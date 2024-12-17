/**
 * @brief compiler wrapper utiltiy implementation file
 */

#ifndef COMPILER_H_
#define COMPILER_H_

// four stages of compilation))
#include <cf_ast.h>
#include <cf_tir.h>
#include <cf_codegen.h>
#include <cf_linker.h>

/// @brief compiler 'library' - ultimate project library collection
typedef struct Compiler_ Compiler;

/**
 * @brief compiler constructor
 * 
 * @return created compiler pointer
 */
Compiler * compilerCtor( void );

/**
 * @brief compiler destructor
 * 
 * @return created compiler pointer
 */
void compilerDtor( Compiler *const self );

/// @brief CF source file adding stauts
typedef enum CompmilrAddCfFileStatus_ {
    COMPILER_ADD_CF_FILE_STATUS_OK,             ///< 
    COMPILER_ADD_CF_FILE_STATUS_INTERNAL_ERROR, ///< 
    COMPILER_ADD_CF_FILE_STATUS_LEXER_ERROR,    ///< 
    COMPILER_ADD_CF_FILE_STATUS_AST_ERROR,      ///< 
    COMPILER_ADD_CF_FILE_STATUS_TIR_ERROR,      ///< 
    COMPILER_ADD_CF_FILE_STATUS_CODEGEN_ERROR,  ///< 
} CompilerAddCfFileStatus;

/// @brief add CF file result
typedef struct CompilrAddCfFileResult_ {
    CompilerAddCfFileStatus status; ///< 

    union {
        CfLexerTokenizeTextResult lexerError;   ///< lexer error
        CfAstParseResult          astError;     ///< AST building error
        CfTirBuildingResult       tirError;     ///< TIR building error
        CfCodegenResult           codegenError; ///< code generation error
    };
} CompilerAddCfFileResult;

/**
 * @brief add fiel to compilation process
 * 
 * @param[in] self       compiler pointer
 * @param[in] sourceName source file name
 * @param[in] source     source file on CF language to add to compilation process
 * 
 * @return file addition result
 */
CompilerAddCfFileResult compilerAddCfFile( Compiler *const self, const char *sourceName, const char *source );

/// @brief building status
typedef enum CompilerBuildStatus {
    COMPILER_BUILD_STATUS_OK,             ///< building succeeded
    COMPILER_BUILD_STATUS_INTERNAL_ERROR, ///< interal compiler error occured
    COMPILER_BUILD_STATUS_LINK_ERROR,     ///< link error
    COMPILER_BUILD_STATUS_NO_INPUT_FILES, ///< no input files to link into single executable.
} CompilerBuildStatus;

/// @brief compile building result
typedef struct CompilerBuildResult_ {
    CompilerBuildStatus status; ///< linking status

    union {
        CfExecutable ok;

        struct {
            CfLinkStatus  status; ///< status (MUST NOT be _OK)
            CfLinkDetails error;  ///< error itself
        } linkError;
    };
} CompilerBuildResult;

/**
 * @brief link files into single executable and return it
 * 
 * @param[in] self compiler pointer
 * 
 * @return build result
 */
CompilerBuildResult compilerBuildExecutable( Compiler *const self );

#endif // !defined(COMPILER_H_)