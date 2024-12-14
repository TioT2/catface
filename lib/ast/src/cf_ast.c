/**
 * @brief AST main implementation file
 */

#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "cf_ast_internal.h"

const char * cfAstDeclarationTypeStr( CfAstDeclarationType declType ) {
    switch (declType) {
    case CF_AST_DECLARATION_TYPE_FN  : return "fn";
    case CF_AST_DECLARATION_TYPE_LET : return "let";
    }

    assert(false && "Invalid 'declType' parameter passed");
    return NULL;
} // cfAstDeclarationTypeStr

const char * cfAstTypeStr( CfAstType type ) {
    switch (type) {
    case CF_AST_TYPE_I32:  return "i32";
    case CF_AST_TYPE_U32:  return "u32";
    case CF_AST_TYPE_F32:  return "f32";
    case CF_AST_TYPE_VOID: return "void";
    }

    assert(false && "Invalid 'type' parameter passed");
    return NULL;
} // cfAstTypeStr

void cfAstDtor( CfAst *ast ) {
    // AST allocation is located in corresponding arena
    if (ast != NULL)
        cfArenaDtor(ast->dataArena);
} // cfAstDtor

const CfAstDeclaration * cfAstGetDeclarations( const CfAst *ast ) {
    assert(ast != NULL);
    return ast->declArray;
} // cfAstGetDeclarations

size_t cfAstGetDeclarationCount( const CfAst *ast ) {
    assert(ast != NULL);
    return ast->declArrayLen;
} // cfAstGetDeclarationCount

CfStr cfAstGetSourceFileName( const CfAst *ast ) {
    assert(ast != NULL);
    return ast->sourceName;
} // cfAstGetSourceFileName

void cfAstSpanDumpJson( FILE *out, CfStrSpan span ) {
    fprintf(out, "[%u, %u]", span.begin, span.end);
} // cfAstSpanDumpJson

/**
 * @brief AST expression in json dumping function
 * 
 * @param[in] out    output file
 * @param[in] expr   expression to dump
 * @param[in] offset expression offset
 */
static void cfAstExprDumpJson( FILE *out, const CfAstExpression *expr, size_t offset ) {
} // 

void cfAstDumpJson( FILE *out, const CfAst *ast ) {
    fprintf(out, "{\n");

    fprintf(out, "    \"sourceName\": \"");
    cfStrWriteShielded(out, ast->sourceName);
    fprintf(out, "\",\n");

    fprintf(out, "    \"sourceContents\": \"");
    cfStrWriteShielded(out, ast->sourceContents);
    fprintf(out, "\",\n");

    fprintf(out, "    \"declarations\": [\n");
    for (size_t i = 0; i < ast->declArrayLen; i++) {
        const CfAstDeclaration *decl = &ast->declArray[i];
        fprintf(out, "%*s{\n", 8, "");

        // display type
        fprintf(out, "%*s\"type\": \"", 12, "");
        cfStrWriteShielded(out, CF_STR(cfAstDeclarationTypeStr(decl->type)));
        fprintf(out, "\",\n");

        fprintf(out, "%*s\"span\": ", 12, "");
        cfAstSpanDumpJson(out, decl->span);
        fprintf(out, ",\n");

        switch (decl->type) {
        case CF_AST_DECLARATION_TYPE_FN: {
            // don't dump function additional content))
            break;
        }

        case CF_AST_DECLARATION_TYPE_LET: {
            fprintf(out, "%*s\"name\": \"", 12, "");
            cfStrWriteShielded(out, decl->let.name);
            fprintf(out, "\",\n");

            fprintf(out, "%*s\"type\": \"", 12, "");
            cfStrWriteShielded(out, CF_STR(cfAstTypeStr(decl->let.type)));
            fprintf(out, "\"\n");

            break;
        }
        }

        fprintf(out, "        }%s\n", i == ast->declArrayLen - 1 ? "" : ",");
    }
    fprintf(out, "    ]\n");

    fprintf(out, "}\n");
} // cfAstDumpJson

// cf_ast.c
