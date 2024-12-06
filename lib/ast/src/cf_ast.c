/**
 * @brief AST main implementation file
 */

#include <assert.h>

#include <cf_arena.h>

#include "cf_ast.h"

/// @brief AST main structure
typedef struct __CfAstImpl {
    CfArena      mem;            ///< AST allocation holder
    const char * sourceFileName; ///< source file name
    size_t       declCount;      ///< count
    CfAstDecl    decls[1];       ///< declaration array (extends beyond structure memory for declCount - 1 elements)
} CfAstImpl;

CfStr cfAstSpanCutStr( CfAstSpan span, CfStr str ) {
    CfStr result = {
        .begin = str.begin + span.begin,
        .end = str.begin + span.end,
    };

    return (CfStr) {
        .begin = result.begin < str.end
            ? result.begin
            : str.end,

        .end = result.end < str.end
            ? result.end
            : str.end,
    };
} // cfAstSpanCutStr

void cfAstDtor( CfAst ast ) {
    // AST allocation is located in corresponding arena
    if (ast != NULL)
        cfArenaDtor(ast->mem);
} // cfAstDtor

const CfAstDecl * cfAstGetDecls( const CfAst ast ) {
    assert(ast != NULL);
    return ast->decls;
} // cfAstGetDecls

size_t cfAstGetDeclCount( const CfAst ast ) {
    assert(ast != NULL);
    return ast->declCount;
} // cfAstGetDeclCount

const char * cfAstGetSourceFileName( const CfAst ast ) {
    assert(ast != NULL);
    return ast->sourceFileName;
} // cfAstGetSourceFileName

// cf_ast.c
