/**
 * @brief AST main implementation file
 */

#include <assert.h>

#include "cf_ast_internal.h"

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
    return ast->declArray;
} // cfAstGetDecls

size_t cfAstGetDeclCount( const CfAst ast ) {
    assert(ast != NULL);
    return ast->declArrayLen;
} // cfAstGetDeclCount

CfStr cfAstGetSourceFileName( const CfAst ast ) {
    assert(ast != NULL);
    return ast->sourceName;
} // cfAstGetSourceFileName

// cf_ast.c
