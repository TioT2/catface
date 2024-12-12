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

    // try to parse value (identifier, literal or expression in '()')
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

    case CF_AST_TOKEN_TYPE_IDENTIFIER:
        *resultExpr = (CfAstExpr) {
            .type       = CF_AST_EXPR_TYPE_IDENTIFIER,
            .span       = tokenList->span,
            .identifier = tokenList->identifier,
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
 * @brief comparison expression parsing function
 * 
 * @param[in]
 */
static CfAstExpr * cfAstParseExprComparison(
    CfAstParser      *const self,
    const CfAstToken **     tokenListPtr
) {
    const CfAstToken *tokenList = *tokenListPtr;

    CfAstExpr *root = cfAstParseExprSum(self, &tokenList);
    if (root == NULL)
        return NULL;

    for (;;) {
        // try to parse parse binary operator (addition/substraction)
        CfAstBinaryOperator op = CF_AST_BINARY_OPERATOR_ADD;
        const CfAstToken *opToken = tokenList;

        switch (opToken->type) {
        case CF_AST_TOKEN_TYPE_EQUAL_EQUAL            : op = CF_AST_BINARY_OPERATOR_EQ; break;
        case CF_AST_TOKEN_TYPE_EXCLAMATION_EQUAL      : op = CF_AST_BINARY_OPERATOR_NE; break;
        case CF_AST_TOKEN_TYPE_ANGULAR_BR_OPEN        : op = CF_AST_BINARY_OPERATOR_LT; break;
        case CF_AST_TOKEN_TYPE_ANGULAR_BR_OPEN_EQUAL  : op = CF_AST_BINARY_OPERATOR_LE; break;
        case CF_AST_TOKEN_TYPE_ANGULAR_BR_CLOSE       : op = CF_AST_BINARY_OPERATOR_GT; break;
        case CF_AST_TOKEN_TYPE_ANGULAR_BR_CLOSE_EQUAL : op = CF_AST_BINARY_OPERATOR_GE; break;

        default:
            opToken = NULL;
        }

        if (opToken == NULL)
            break;

        tokenList++;

        CfAstExpr *rhs = cfAstParseExprSum(self, &tokenList);
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
} // cfAstParseExprComparison

/**
 * @brief assignment operator parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * @param[out]    opDst        operand parsing destination
 * 
 * @return true if parsed, false if not.
 */
static bool cfAstParseAssignmentOperator(
    CfAstParser             *const self,
    const CfAstToken       **      tokenListPtr,
    CfAstAssignmentOperator *      opDst
) {
    switch ((*tokenListPtr)->type) {
    case CF_AST_TOKEN_TYPE_EQUAL             : *opDst = CF_AST_ASSIGNMENT_OPERATOR_NONE; break;
    case CF_AST_TOKEN_TYPE_PLUS_EQUAL        : *opDst = CF_AST_ASSIGNMENT_OPERATOR_ADD;  break;
    case CF_AST_TOKEN_TYPE_MINUS_EQUAL       : *opDst = CF_AST_ASSIGNMENT_OPERATOR_SUB;  break;
    case CF_AST_TOKEN_TYPE_ASTERISK_EQUAL    : *opDst = CF_AST_ASSIGNMENT_OPERATOR_MUL;  break;
    case CF_AST_TOKEN_TYPE_SLASH_EQUAL       : *opDst = CF_AST_ASSIGNMENT_OPERATOR_DIV;  break;
    default:
        return false;
    }

    (*tokenListPtr)++;
    return true;
} // cfAstParseAssignmentOperator

/**
 * @brief assignment expression parsing function
 * 
 * @param[in] self         parser pointer
 * @param[in] tokenListPtr token list pointer
 * 
 * @return parsed assignment (NULL if parsing failed)
 */
static CfAstExpr * cfAstParseExprAssignment(
    CfAstParser       *const self,
    const CfAstToken **      tokenListPtr
) {
    const CfAstToken *tokenList = *tokenListPtr;

    size_t spanBegin = tokenList->span.begin;

    const CfAstToken *destToken = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_IDENTIFIER, false);
    CfAstAssignmentOperator op = CF_AST_ASSIGNMENT_OPERATOR_NONE;

    if (destToken == NULL || !cfAstParseAssignmentOperator(self, &tokenList, &op))
        return NULL;
    CfStr destination = destToken->identifier;

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
            .op          = op,
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
    if ((result = cfAstParseExprComparison(self, tokenListPtr)) != NULL)
        return result;
    return NULL;
} // cfAstParseExpr

// cf_ast_parser_expression.c
