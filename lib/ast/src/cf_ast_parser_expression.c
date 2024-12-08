/**
 * @brief expression parser implemetnation file
 */

#include <cf_deque.h>

#include "cf_ast_parser.h"

/**
 * @brief prefix-value-postfix combination parsing function
 * 
 * @param[in] self         parser pointer
 * @param[in] tokenListPtr parsed token list pointer
 * 
 * @return parsed expression (NULL if parsing failed)
 */
static CfAstExpr * cfAstParseExprValue( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    const CfAstToken *tokenList = *tokenListPtr;

    // try to parse value (ident, literal or expression in '()')
    CfAstExpr *resultExpr = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
    cfAstParserAssert(self, resultExpr != NULL);

    switch (tokenList->type) {
    case CF_AST_TOKEN_TYPE_INTEGER:
        *resultExpr = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_INTEGER,
            .span = tokenList->span,
            .integer = tokenList->integer,
        };
        tokenList++;
        break;

    case CF_AST_TOKEN_TYPE_FLOATING:
        *resultExpr = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_FLOATING,
            .span = tokenList->span,
            .floating = tokenList->floating,
        };
        tokenList++;
        break;

    case CF_AST_TOKEN_TYPE_IDENT:
        *resultExpr = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_IDENT,
            .span = tokenList->span,
            .ident = tokenList->ident,
        };
        tokenList++;
        break;

    case CF_AST_TOKEN_TYPE_ROUND_BR_OPEN: {
        size_t spanStart = tokenList->span.begin;
        tokenList++;

        CfAstExpr *expr = cfAstParseExpr(self, &tokenList);
        cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_ROUND_BR_CLOSE, true);

        if (expr == NULL)
            // commit error if cannot parse expression in brackets
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_EXPR_BRACKET_INTERNALS_MISSING,
                .bracketInternalsMissing = { spanStart, tokenList->span.begin }
            });

        resultExpr = expr;

        break;
    }

    default:
        return NULL;
    }


    // try to parse function call
    if (tokenList->type == CF_AST_TOKEN_TYPE_ROUND_BR_OPEN) {
        tokenList++;

        // create parameter list
        CfDeque paramDeque = cfDequeCtor(sizeof(CfAstExpr *), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);
        cfAstParserAssert(self, paramDeque != NULL);

        // parse arguments
        for (;;) {
            // try to parse parameter expression
            CfAstExpr *param = cfAstParseExpr(self, &tokenList);
            if (param == NULL)
                break;

            // insert expression to queue
            cfAstParserAssert(self, cfDequePushBack(paramDeque, &param));

            // try to parse comma
            if (cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_COMMA, false) == NULL)
                break;
        }

        // move params from deque to array
        size_t paramArrayLength = cfDequeLength(paramDeque);
        CfAstExpr **paramArray = (CfAstExpr **)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr *) * paramArrayLength);
        cfAstParserAssert(self, paramArray != NULL);
        cfDequeWrite(paramDeque, paramArray);

        // parse closing bracket
        cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_ROUND_BR_CLOSE, true);

        // apply call expression
        CfAstExpr *callExpr = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
        cfAstParserAssert(self, callExpr != NULL);
        *callExpr = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_CALL,
            .call = {
                .callee           = resultExpr,
                .paramArrayLength = paramArrayLength,
                .paramArray       = paramArray,
            },
        };

        resultExpr = callExpr;
    }

    *tokenListPtr = tokenList;
    return resultExpr;
} // cfAstParseExprValue

CfAstExpr * cfAstParseExpr( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    return cfAstParseExprValue(self, tokenListPtr);
} // cfAstParseExpr

// cf_ast_parser_expression.c
