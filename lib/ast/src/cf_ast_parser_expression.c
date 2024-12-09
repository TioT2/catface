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

    // FIXME Arena memory leaks in 'default' case.
    CfAstExpr *resultExpr = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
    cfAstParserAssert(self, resultExpr != NULL);

    // try to parse value (ident, literal or expression in '()')
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
        size_t callSpanEnd = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_ROUND_BR_CLOSE, true)->span.end;

        // apply call expression
        CfAstExpr *callExpr = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
        cfAstParserAssert(self, callExpr != NULL);
        *callExpr = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_CALL,
            .span = (CfAstSpan) { resultExpr->span.begin, callSpanEnd },
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

/**
 * @brief product parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * 
 * @return parsed expression (NULL if parsing failed)
 */
static CfAstExpr * cfAstParseExprProduct(
    CfAstParser       *const self,
    const CfAstToken **      tokenListPtr
) {
    const CfAstToken *tokenList = *tokenListPtr;

    CfAstExpr *root = cfAstParseExprValue(self, &tokenList);
    if (root == NULL)
        return NULL;

    for (;;) {
        // try to parse parse binary operator (multiplication/division)
        CfAstBinaryOperator op = CF_AST_BINARY_OPERATOR_MUL;
        const CfAstToken *opToken = NULL;

        if ((opToken = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_ASTERISK, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_MUL;
        } else if ((opToken = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_SLASH, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_DIV;
        } else {
            break;
        }

        CfAstExpr *rhs = cfAstParseExprValue(self, &tokenList);
        if (rhs == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_EXPR_RHS_MISSING,
                .rhsMissing = (CfAstSpan) { root->span.begin, opToken->span.end },
            });

        CfAstExpr *newRoot = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
        cfAstParserAssert(self, newRoot != NULL);

        *newRoot = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_BINARY_OPERATOR,
            .span = (CfAstSpan) { root->span.begin, rhs->span.end },
            .binaryOperator = { .op = op, .lhs = root, .rhs = rhs },
        };

        root = newRoot;
    }

    *tokenListPtr = tokenList;
    return root;
} // cfAstParseExprProduct

/**
 * @brief sum expression parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list
 * 
 * @return parsed expression (NULL if parsing failed)
 */
static CfAstExpr * cfAstParseExprSum(
    CfAstParser       *const self,
    const CfAstToken **      tokenListPtr
) {
    const CfAstToken *tokenList = *tokenListPtr;

    CfAstExpr *root = cfAstParseExprProduct(self, &tokenList);
    if (root == NULL)
        return NULL;

    for (;;) {
        // try to parse parse binary operator (addition/substraction)
        CfAstBinaryOperator op = CF_AST_BINARY_OPERATOR_ADD;
        const CfAstToken *opToken = NULL;

        if ((opToken = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_PLUS, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_ADD;
        } else if ((opToken = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_MINUS, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_SUB;
        } else {
            break;
        }

        CfAstExpr *rhs = cfAstParseExprProduct(self, &tokenList);
        if (rhs == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_EXPR_RHS_MISSING,
                .rhsMissing = (CfAstSpan) { root->span.begin, opToken->span.end },
            });

        CfAstExpr *newRoot = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
        cfAstParserAssert(self, newRoot != NULL);

        *newRoot = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_BINARY_OPERATOR,
            .span = (CfAstSpan) { root->span.begin, rhs->span.end },
            .binaryOperator = { .op = op, .lhs = root, .rhs = rhs },
        };

        root = newRoot;
    }

    *tokenListPtr = tokenList;
    return root;
} // cfAstParseExprSum

/**
 * @brief assignment expression parsing function
 */
static CfAstExpr * cfAstParseExprAssignment(
    CfAstParser       *const self,
    const CfAstToken **      tokenListPtr
) {
    const CfAstToken *tokenList = *tokenListPtr;

    size_t spanBegin = tokenList->span.begin;

    const CfAstToken *destToken = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_IDENT, false);

    if (destToken == NULL || cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_EQUAL, false) == NULL)
        return NULL;
    CfStr destination = destToken->ident;

    // parse expression
    CfAstExpr *value = cfAstParseExpr(self, &tokenList);

    if (value == NULL)
        cfAstParserFinish(self, (CfAstParseResult) {
            .status                 = CF_AST_PARSE_STATUS_EXPR_ASSIGNMENT_VALUE_MISSING,
            .assignmentValueMissing = (CfAstSpan) { spanBegin, tokenList->span.begin },
        });

    CfAstExpr *resultExpr = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
    cfAstParserAssert(self, resultExpr != NULL);

    *resultExpr = (CfAstExpr) {
        .type = CF_AST_EXPR_TYPE_ASSIGNMENT,
        .span = (CfAstSpan) { spanBegin, tokenList->span.begin },
        .assignment = {
            .destination = destination,
            .value       = value,
        },
    };

    *tokenListPtr = tokenList;
    return resultExpr;
} // cfAstParseExprAssignment

CfAstExpr * cfAstParseExpr( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    CfAstExpr *result = NULL;

    if ((result = cfAstParseExprAssignment(self, tokenListPtr)) != NULL)
        return result;
    if ((result = cfAstParseExprSum(self, tokenListPtr)) != NULL)
        return result;
    return NULL;
} // cfAstParseExpr

// cf_ast_parser_expression.c
