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
static CfAstExpression * cfAstParseExprValue( CfAstParser *const self, const CfLexerToken **tokenListPtr ) {
    const CfLexerToken *tokenList = *tokenListPtr;

    // FIXME Arena memory leaks in 'default' case.
    CfAstExpression *resultExpr = (CfAstExpression *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression));
    cfAstParserAssert(self, resultExpr != NULL);

    // try to parse value (identifier, literal or expression in '()')
    switch (tokenList->type) {
    case CF_LEXER_TOKEN_TYPE_INTEGER:
        *resultExpr = (CfAstExpression) {
            .type = CF_AST_EXPRESSION_TYPE_INTEGER,
            .span = tokenList->span,
            .integer = tokenList->integer,
        };
        tokenList++;
        break;

    case CF_LEXER_TOKEN_TYPE_FLOATING:
        *resultExpr = (CfAstExpression) {
            .type = CF_AST_EXPRESSION_TYPE_FLOATING,
            .span = tokenList->span,
            .floating = tokenList->floating,
        };
        tokenList++;
        break;

    case CF_LEXER_TOKEN_TYPE_IDENTIFIER:
        *resultExpr = (CfAstExpression) {
            .type       = CF_AST_EXPRESSION_TYPE_IDENTIFIER,
            .span       = tokenList->span,
            .identifier = tokenList->identifier,
        };
        tokenList++;
        break;

    case CF_LEXER_TOKEN_TYPE_ROUND_BR_OPEN: {
        uint32_t spanStart = tokenList->span.begin;
        tokenList++;

        CfAstExpression *expr = cfAstParseExpr(self, &tokenList);
        cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_ROUND_BR_CLOSE, true);

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

    // parse postifx operator sequence
    for (;;) {
        // try to parse function call
        if (tokenList->type == CF_LEXER_TOKEN_TYPE_ROUND_BR_OPEN) {
            tokenList++;

            // create parameter list
            CfDeque paramDeque = cfDequeCtor(sizeof(CfAstExpression *), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);
            cfAstParserAssert(self, paramDeque != NULL);

            // parse arguments
            for (;;) {
                // try to parse parameter expression
                CfAstExpression *param = cfAstParseExpr(self, &tokenList);
                if (param == NULL)
                    break;

                // insert expression to queue
                cfAstParserAssert(self, cfDequePushBack(paramDeque, &param));

                // try to parse comma
                if (cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_COMMA, false) == NULL)
                    break;
            }

            // move params from deque to array
            size_t paramArrayLength = cfDequeLength(paramDeque);
            CfAstExpression **paramArray = (CfAstExpression **)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression *) * paramArrayLength);
            cfAstParserAssert(self, paramArray != NULL);
            cfDequeWrite(paramDeque, paramArray);

            // parse closing bracket
            uint32_t callSpanEnd = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_ROUND_BR_CLOSE, true)->span.end;

            // apply call expression
            CfAstExpression *callExpr = (CfAstExpression *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression));
            cfAstParserAssert(self, callExpr != NULL);
            *callExpr = (CfAstExpression) {
                .type = CF_AST_EXPRESSION_TYPE_CALL,
                .span = (CfStrSpan) { resultExpr->span.begin, callSpanEnd },
                .call = {
                    .callee           = resultExpr,
                    .argumentArrayLength = paramArrayLength,
                    .argumentArray       = paramArray,
                },
            };

            resultExpr = callExpr;

            continue;
        }

        // try to parse conversion
        if (tokenList->type == CF_LEXER_TOKEN_TYPE_AS) {
            tokenList++;

            // parse type
            CfAstType type = CF_AST_TYPE_VOID;
            cfAstParseType(self, &tokenList, &type);

            // apply call expression
            CfAstExpression *convExpr = (CfAstExpression *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression));
            cfAstParserAssert(self, convExpr != NULL);
            *convExpr = (CfAstExpression) {
                .type = CF_AST_EXPRESSION_TYPE_CONVERSION,
                .span = (CfStrSpan) { resultExpr->span.begin, tokenList->span.begin },
                .conversion = {
                    .expr = resultExpr,
                    .type = type,
                },
            };

            resultExpr = convExpr;

            continue;
        }
        break;
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
static CfAstExpression * cfAstParseExprProduct(
    CfAstParser       *const self,
    const CfLexerToken **      tokenListPtr
) {
    const CfLexerToken *tokenList = *tokenListPtr;

    CfAstExpression *root = cfAstParseExprValue(self, &tokenList);
    if (root == NULL)
        return NULL;

    for (;;) {
        // try to parse parse binary operator (multiplication/division)
        CfAstBinaryOperator op = CF_AST_BINARY_OPERATOR_MUL;
        const CfLexerToken *opToken = NULL;

        if ((opToken = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_ASTERISK, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_MUL;
        } else if ((opToken = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_SLASH, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_DIV;
        } else {
            break;
        }

        CfAstExpression *rhs = cfAstParseExprValue(self, &tokenList);
        if (rhs == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_EXPR_RHS_MISSING,
                .rhsMissing = (CfStrSpan) { root->span.begin, opToken->span.end },
            });

        CfAstExpression *newRoot = (CfAstExpression *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression));
        cfAstParserAssert(self, newRoot != NULL);

        *newRoot = (CfAstExpression) {
            .type = CF_AST_EXPRESSION_TYPE_BINARY_OPERATOR,
            .span = (CfStrSpan) { root->span.begin, rhs->span.end },
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
static CfAstExpression * cfAstParseExprSum(
    CfAstParser       *const self,
    const CfLexerToken **      tokenListPtr
) {
    const CfLexerToken *tokenList = *tokenListPtr;

    CfAstExpression *root = cfAstParseExprProduct(self, &tokenList);
    if (root == NULL)
        return NULL;

    for (;;) {
        // try to parse parse binary operator (addition/substraction)
        CfAstBinaryOperator op = CF_AST_BINARY_OPERATOR_ADD;
        const CfLexerToken *opToken = NULL;

        if ((opToken = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_PLUS, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_ADD;
        } else if ((opToken = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_MINUS, false)) != NULL) {
            op = CF_AST_BINARY_OPERATOR_SUB;
        } else {
            break;
        }

        CfAstExpression *rhs = cfAstParseExprProduct(self, &tokenList);
        if (rhs == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_EXPR_RHS_MISSING,
                .rhsMissing = (CfStrSpan) { root->span.begin, opToken->span.end },
            });

        CfAstExpression *newRoot = (CfAstExpression *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression));
        cfAstParserAssert(self, newRoot != NULL);

        *newRoot = (CfAstExpression) {
            .type = CF_AST_EXPRESSION_TYPE_BINARY_OPERATOR,
            .span = (CfStrSpan) { root->span.begin, rhs->span.end },
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
static CfAstExpression * cfAstParseExprComparison(
    CfAstParser      *const self,
    const CfLexerToken **     tokenListPtr
) {
    const CfLexerToken *tokenList = *tokenListPtr;

    CfAstExpression *root = cfAstParseExprSum(self, &tokenList);
    if (root == NULL)
        return NULL;

    for (;;) {
        // try to parse parse binary operator (addition/substraction)
        CfAstBinaryOperator op = CF_AST_BINARY_OPERATOR_ADD;
        const CfLexerToken *opToken = tokenList;

        switch (opToken->type) {
        case CF_LEXER_TOKEN_TYPE_EQUAL_EQUAL            : op = CF_AST_BINARY_OPERATOR_EQ; break;
        case CF_LEXER_TOKEN_TYPE_EXCLAMATION_EQUAL      : op = CF_AST_BINARY_OPERATOR_NE; break;
        case CF_LEXER_TOKEN_TYPE_ANGULAR_BR_OPEN        : op = CF_AST_BINARY_OPERATOR_LT; break;
        case CF_LEXER_TOKEN_TYPE_ANGULAR_BR_OPEN_EQUAL  : op = CF_AST_BINARY_OPERATOR_LE; break;
        case CF_LEXER_TOKEN_TYPE_ANGULAR_BR_CLOSE       : op = CF_AST_BINARY_OPERATOR_GT; break;
        case CF_LEXER_TOKEN_TYPE_ANGULAR_BR_CLOSE_EQUAL : op = CF_AST_BINARY_OPERATOR_GE; break;

        default:
            opToken = NULL;
        }

        if (opToken == NULL)
            break;

        tokenList++;

        CfAstExpression *rhs = cfAstParseExprSum(self, &tokenList);
        if (rhs == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_EXPR_RHS_MISSING,
                .rhsMissing = (CfStrSpan) { root->span.begin, opToken->span.end },
            });

        CfAstExpression *newRoot = (CfAstExpression *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression));
        cfAstParserAssert(self, newRoot != NULL);

        *newRoot = (CfAstExpression) {
            .type = CF_AST_EXPRESSION_TYPE_BINARY_OPERATOR,
            .span = (CfStrSpan) { root->span.begin, rhs->span.end },
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
    const CfLexerToken       **      tokenListPtr,
    CfAstAssignmentOperator *      opDst
) {
    switch ((*tokenListPtr)->type) {
    case CF_LEXER_TOKEN_TYPE_EQUAL             : *opDst = CF_AST_ASSIGNMENT_OPERATOR_NONE; break;
    case CF_LEXER_TOKEN_TYPE_PLUS_EQUAL        : *opDst = CF_AST_ASSIGNMENT_OPERATOR_ADD;  break;
    case CF_LEXER_TOKEN_TYPE_MINUS_EQUAL       : *opDst = CF_AST_ASSIGNMENT_OPERATOR_SUB;  break;
    case CF_LEXER_TOKEN_TYPE_ASTERISK_EQUAL    : *opDst = CF_AST_ASSIGNMENT_OPERATOR_MUL;  break;
    case CF_LEXER_TOKEN_TYPE_SLASH_EQUAL       : *opDst = CF_AST_ASSIGNMENT_OPERATOR_DIV;  break;
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
static CfAstExpression * cfAstParseExprAssignment(
    CfAstParser       *const self,
    const CfLexerToken **      tokenListPtr
) {
    const CfLexerToken *tokenList = *tokenListPtr;

    uint32_t spanBegin = tokenList->span.begin;

    const CfLexerToken *destToken = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_IDENTIFIER, false);
    CfAstAssignmentOperator op = CF_AST_ASSIGNMENT_OPERATOR_NONE;

    if (destToken == NULL || !cfAstParseAssignmentOperator(self, &tokenList, &op))
        return NULL;
    CfStr destination = destToken->identifier;

    // parse expression
    CfAstExpression *value = cfAstParseExpr(self, &tokenList);

    if (value == NULL)
        cfAstParserFinish(self, (CfAstParseResult) {
            .status                 = CF_AST_PARSE_STATUS_EXPR_ASSIGNMENT_VALUE_MISSING,
            .assignmentValueMissing = (CfStrSpan) { spanBegin, tokenList->span.begin },
        });

    CfAstExpression *resultExpr = (CfAstExpression *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpression));
    cfAstParserAssert(self, resultExpr != NULL);

    *resultExpr = (CfAstExpression) {
        .type = CF_AST_EXPRESSION_TYPE_ASSIGNMENT,
        .span = (CfStrSpan) { spanBegin, tokenList->span.begin },
        .assignment = {
            .op          = op,
            .destination = destination,
            .value       = value,
        },
    };

    *tokenListPtr = tokenList;
    return resultExpr;
} // cfAstParseExprAssignment

CfAstExpression * cfAstParseExpr( CfAstParser *const self, const CfLexerToken **tokenListPtr ) {
    CfAstExpression *result = NULL;

    if ((result = cfAstParseExprAssignment(self, tokenListPtr)) != NULL)
        return result;
    if ((result = cfAstParseExprComparison(self, tokenListPtr)) != NULL)
        return result;
    return NULL;
} // cfAstParseExpr

// cf_ast_parser_expression.c
