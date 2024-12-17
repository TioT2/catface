/**
 * @brief compiler implementation file
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <cf_deque.h>

#include "compiler.h"

/// @brief compiler file
typedef struct CompilerFile_ {
    size_t     textLength; ///< file length
    char     * text;       ///< file text name (owned)
    char     * name;       ///< file name (owned)
    CfObject   object;     ///< 
} CompilerFile;

/// @brief compiler itself
typedef struct Compiler_ {
    CfArena * dataArena;      ///< arena, used for compiler data allocation
    CfArena * tempArena;      ///< temporary allocation arena
    CfDeque * inputFileDeque; ///< input file deque
} Compiler;

Compiler * compilerCtor( void ) {
    Compiler *compiler = NULL;
    CfArena *dataArena = NULL;
    CfArena *tempArena = NULL;
    CfDeque *inputFileDeque = NULL;

    // 4096 as allocation block is used because of why not

    if (false
        || (dataArena = cfArenaCtor(4096)) == NULL
        || (compiler = (Compiler *)cfArenaAlloc(dataArena, sizeof(Compiler))) == NULL
        || (tempArena = cfArenaCtor(4096)) == NULL
        || (inputFileDeque = cfDequeCtor(sizeof(CompilerFile), CF_DEQUE_CHUNK_SIZE_UNDEFINED, dataArena)) == NULL
    ) {
        cfArenaDtor(tempArena);
        cfArenaDtor(dataArena);
        return NULL;
    }

    *compiler = (Compiler) {
        .dataArena      = dataArena,
        .tempArena      = tempArena,
        .inputFileDeque = inputFileDeque,
    };

    return compiler;
} // compilerCtor

CompilerAddCfFileResult compilerAddCfFile( Compiler *const self, const char *sourceName, const char *source ) {
    size_t sourceLength = strlen(source);
    size_t sourceNameLength = strlen(sourceName);

    char *textCopy = (char *)cfArenaAlloc(self->dataArena, sizeof(char) * (sourceLength + sourceNameLength + 2));
    char *sourceNameCopy = textCopy + sourceLength + 1;

    if (textCopy == NULL)
        return (CompilerAddCfFileResult) { COMPILER_ADD_CF_FILE_STATUS_INTERNAL_ERROR };
    memcpy(textCopy, source, sourceLength);
    memcpy(sourceNameCopy, sourceName, sourceNameLength);


    CompilerFile file = {
        .textLength = sourceLength,
        .text = textCopy,
        .name = sourceNameCopy,
    };

    CfAst *ast = NULL;
    CfTir *tir = NULL;

    // build AST
    CfAstParseResult astParseResult = cfAstParse(
        (CfStr) { file.name, file.name + sourceNameLength },
        (CfStr) { file.text, file.text + sourceLength },
        self->tempArena
    );

    if (astParseResult.status != CF_AST_PARSE_STATUS_OK)
        return (CompilerAddCfFileResult) {
            .status = COMPILER_ADD_CF_FILE_STATUS_AST_ERROR,
            .astError = astParseResult,
        };
    ast = astParseResult.ok;

    // free temp variables
    cfArenaFree(self->tempArena);


    CfTirBuildingResult tirBuildResult = cfTirBuild(
        ast,
        self->tempArena
    );

    if (tirBuildResult.status != CF_TIR_BUILDING_STATUS_OK)
        return (CompilerAddCfFileResult) {
            .status = COMPILER_ADD_CF_FILE_STATUS_TIR_ERROR,
            .tirError = tirBuildResult,
        };
    tir = tirBuildResult.ok;

    cfArenaFree(self->tempArena);

    // run codegenerator
    CfCodegenResult codegenResult = cfCodegen(tir, &file.object, self->tempArena);

    if (codegenResult.status != CF_CODEGEN_STATUS_OK)
        return (CompilerAddCfFileResult) {
            .status = COMPILER_ADD_CF_FILE_STATUS_CODEGEN_ERROR,
            .codegenError = codegenResult
        };

    // object built
    cfAstDtor(ast);
    cfTirDtor(tir);

    // 
    if (!cfDequePushBack(self->inputFileDeque, &file)) {
        cfObjectDtor(&file.object);
        return (CompilerAddCfFileResult) { COMPILER_ADD_CF_FILE_STATUS_INTERNAL_ERROR };
    }

    return (CompilerAddCfFileResult) { COMPILER_ADD_CF_FILE_STATUS_OK };
} // compilerAddCfFile

CompilerBuildResult compilerBuildExecutable( Compiler *const self ) {
    CfDequeCursor fileCursor = {};

    if (!cfDequeFrontCursor(self->inputFileDeque, &fileCursor))
        return (CompilerBuildResult) { COMPILER_BUILD_STATUS_NO_INPUT_FILES };

    CfExecutable result;
    CfLinkDetails linkDetails;
    size_t objectFileCount = cfDequeLength(self->inputFileDeque);
    CfObject *objects = (CfObject *)cfArenaAlloc(self->tempArena, sizeof(CfObject) * objectFileCount);
    if (objects == NULL)
        return (CompilerBuildResult) { COMPILER_BUILD_STATUS_INTERNAL_ERROR };

    {
        size_t i = 0;
        do {
            objects[i++] = ((CompilerFile *)cfDequeCursorGet(&fileCursor))->object;
        } while (cfDequeCursorAdvance(&fileCursor, 1));
    }

    CfLinkStatus status = cfLink(objects, objectFileCount, &result, &linkDetails);

    if (status != CF_LINK_STATUS_OK)
        return (CompilerBuildResult) {
            .status = COMPILER_BUILD_STATUS_LINK_ERROR,
            .linkError = {
                .status = status,
                .error = linkDetails,
            },
        };

    cfArenaFree(self->tempArena);

    return (CompilerBuildResult) {
        .status = COMPILER_BUILD_STATUS_OK,
        .ok = result,
    };
} // compilerBuildExecutable


void compilerDtor( Compiler *const self ) {
    CfDequeCursor inputFileCursor = {};

    if (cfDequeFrontCursor(self->inputFileDeque, &inputFileCursor)) {
        do {
            CompilerFile *file = (CompilerFile *)cfDequeCursorGet(&inputFileCursor);

            cfObjectDtor(&file->object);
        } while (cfDequeCursorAdvance(&inputFileCursor, 1));

        cfDequeCursorDtor(&inputFileCursor);
    }

    cfArenaDtor(self->tempArena);
    cfArenaDtor(self->dataArena);
} // compilerDtor


// compmiler.h