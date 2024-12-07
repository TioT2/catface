/**
 * @brief AST parser implementation file
 */

#include <assert.h>
#include <setjmp.h>

#include "cf_ast_internal.h"

/// @brief AST parsing context
typedef struct __CfAstParser {
    CfArena tempArena; ///< arena to allocate temporary data in (such as intermediate arrays)
    CfArena dataArena; ///< arena to allocate actual AST data in

    jmp_buf          errorBuffer; ///< buffer to jump to if some error occured
    CfAstParseResult parseResult; ///< AST parsing result (used in case if error committed)
} CfAstParser;

/**
 * @brief AST parsing forced stopping function
 *
 * @param[in] self        parser pointer
 * @param[in] parseResult result to throw exception with (result MUST BE not ok)
 */
void cfAstParserFinish( CfAstParser *const self, CfAstParseResult parseResult ) {
    self->parseResult = parseResult;
    longjmp(self->errorBuffer, 1);
} // cfAstParserFinish

/**
 * @brief declaration array parsing function
 * 
 * @param[in]  self            parser pointer
 * @param[in]  fileContents    text to parse
 * @param[out] declArryDst     declaration array (non-null)
 * @param[out] declArrayLenDst declcaration array length destination (non-null)
 */
void cfAstParseDecls(
    CfAstParser  *const self,
    CfStr               fileContents,
    CfAstDecl   **      declArrayDst,
    size_t       *      declArrayLenDst
) {
    // TMP solution)))
    cfAstParserFinish(self, (CfAstParseResult) { .status = CF_AST_PARSE_STATUS_INTERNAL_ERROR });
} // cfAstPArse

CfAstParseResult cfAstParse( CfStr fileName, CfStr fileContents, CfArena tempArena ) {
    // arena that contains actual AST data
    CfArena dataArena = NULL;
    CfAst ast = NULL;

    if (false
        || (dataArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED)) == NULL
        || ((ast == cfArenaAlloc(dataArena, sizeof(CfAstImpl))))
    ) {
        cfArenaDtor(dataArena);
        return (CfAstParseResult) { .status = CF_AST_PARSE_STATUS_INTERNAL_ERROR };
    }

    bool tempArenaOwned = false;

    // allocate temporary arena if it's not already allocated
    if (tempArena == NULL) {
        tempArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED);
        if (tempArena == NULL) {
            cfArenaDtor(tempArena);
            return (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_INTERNAL_ERROR,
            };
        }
        tempArenaOwned = true;
    }

    CfAstParser parser = {
        .tempArena = tempArena,
        .dataArena = dataArena,
    };

    int isError = setjmp(parser.errorBuffer);

    if (isError) {
        // perform cleanup
        if (tempArenaOwned)
            cfArenaDtor(tempArena);
        cfArenaDtor(dataArena);
        return parser.parseResult;
    }

    CfAstDecl *declArray = NULL;
    size_t declArrayLen = 0;

    cfAstParseDecls(&parser, fileContents, &declArray, &declArrayLen);

    // destroy temp arena in case if it's created in this function
    if (tempArenaOwned)
        cfArenaDtor(tempArena);

    // initialize AST structure
    *ast = (CfAstImpl) {
        .mem            = dataArena,
        .sourceName     = fileName,
        .sourceContents = fileContents,
        .declArray      = declArray,
        .declArrayLen   = declArrayLen,
    };

    return (CfAstParseResult) {
        .status = CF_AST_PARSE_STATUS_OK,
        .ok = ast,
    };
} // cfAstParse


// cf_ast_parse.c
