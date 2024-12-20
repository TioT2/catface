/**
 * @brief AST parser implementation file
 */

#include <assert.h>
#include <setjmp.h>

#include <cf_deque.h>

#include "cf_ast_internal.h"

void cfAstParserFinish( CfAstParser *const self, CfAstParseResult parseResult ) {
    self->parseResult = parseResult;
    longjmp(self->errorBuffer, 1);
} // cfAstParserFinish

void cfAstParserAssert( CfAstParser *const self, bool condition ) {
    if (!condition)
        cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });
} // cfAstParserAssert

void * cfAstParserAllocData( CfAstParser *const self, size_t size ) {
    void *data = cfArenaAlloc(self->dataArena, size);
    cfAstParserAssert(self, data != NULL);
    return data;
} // cfAstParserAllocData

bool cfAstParseType( CfAstParser *const self, const CfLexerToken **tokenListPtr, CfAstType *typeDst ) {
    switch ((*tokenListPtr)->type) {
    case CF_LEXER_TOKEN_TYPE_I32  : *typeDst = CF_AST_TYPE_I32;  break;
    case CF_LEXER_TOKEN_TYPE_U32  : *typeDst = CF_AST_TYPE_U32;  break;
    case CF_LEXER_TOKEN_TYPE_F32  : *typeDst = CF_AST_TYPE_F32;  break;
    case CF_LEXER_TOKEN_TYPE_VOID : *typeDst = CF_AST_TYPE_VOID; break;

    default:
        return false;
    }

    (*tokenListPtr)++;
    return true;
} // cfAstParseType

/**
 * @brief function param parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list to parse function param from
 * @param[out]    paramDst     parameter parsing destination
 * 
 * @return true if parsed, false otherwise.
 */
static bool cfAstParseFunctionParam(
    CfAstParser         *const self,
    const CfLexerToken   **      tokenListPtr,
    CfAstFunctionParam  *      paramDst
) {
    const CfLexerToken *tokenList = *tokenListPtr;
    CfAstType type;

    if (false
        || tokenList[0].type != CF_LEXER_TOKEN_TYPE_IDENTIFIER
        || tokenList[1].type != CF_LEXER_TOKEN_TYPE_COLON
        || (tokenList += 2, !cfAstParseType(self, &tokenList, &type))
    )
        return false;

    *paramDst = (CfAstFunctionParam) {
        .name = (*tokenListPtr)[0].identifier,
        .type = type,
        .span = (CfStrSpan) { (*tokenListPtr)[0].span.begin, tokenList->span.begin }
    };

    *tokenListPtr = tokenList;
    return true;
} // cfAstParseFunctionParam

const CfLexerToken * cfAstParseToken(
    CfAstParser       *const self,
    const CfLexerToken **      tokenListPtr,
    CfLexerTokenType           expectedType,
    bool                     required
) {
    if ((*tokenListPtr)->type == expectedType)
        return (*tokenListPtr)++;
    if (required)
        cfAstParserFinish(self, (CfAstParseResult) {
            .status = CF_AST_PARSE_STATUS_UNEXPECTED_TOKEN_TYPE,
            .unexpectedTokenType = {
                .actualToken  = *tokenListPtr,
                .expectedType = expectedType,
            },
        });

    return NULL;
} // cfAstParseToken


/**
 * @brief statement parsing function
 * 
 * @param[in]  self         parser pointer
 * @param[in]  tokenListPtr token list pointer
 * @param[out] stmtDst      statement parsing destination
 * 
 * @return true if parsed, false if not.
 */
static bool cfAstParseStmt( CfAstParser *const self, const CfLexerToken **tokenListPtr, CfAstStatement *stmtDst ) {
    const CfLexerToken *tokenList = *tokenListPtr;

    CfAstBlock *block = NULL;
    CfAstExpression *expr = NULL;
    CfAstDeclaration decl = {};

    uint32_t spanBegin = tokenList->span.begin;

    // try to parse new block
    if (cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_IF, false) != NULL) {
        // parse condition and corresponding block
        CfAstExpression *cond = cfAstParseExpr(self, &tokenList);

        if (cond == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_IF_CONDITION_MISSING,
                .ifConditionMissing = (CfStrSpan) { spanBegin, tokenList->span.begin },
            });

        CfAstBlock *blockThen = cfAstParseBlock(self, &tokenList);

        if (blockThen == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_IF_BLOCK_MISSING,
                .ifBlockMissing = (CfStrSpan) { spanBegin, tokenList->span.begin },
            });

        CfAstBlock *blockElse = NULL;
        const CfLexerToken *elseToken = NULL;
        if ((elseToken = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_ELSE, false)) != NULL) {
            // parse else block
            blockElse = cfAstParseBlock(self, &tokenList);

            if (blockElse == NULL)
                cfAstParserFinish(self, (CfAstParseResult) {
                    .status = CF_AST_PARSE_STATUS_ELSE_BLOCK_MISSING,
                    .elseBlockMissing = elseToken->span,
                });
        }

        *stmtDst = (CfAstStatement) {
            .type = CF_AST_STATEMENT_TYPE_IF,
            .if_ = {
                .condition = cond,
                .blockThen  = blockThen,
                .blockElse  = blockElse,
            },
        };

    } else if (cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_WHILE, false) != NULL) {
        // parse condition
        CfAstExpression *condition = cfAstParseExpr(self, &tokenList);
        if (condition == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_WHILE_CONDITION_MISSING,
                .whileConditionMissing = (CfStrSpan) { spanBegin, tokenList->span.begin }
            });

        // parse code
        CfAstBlock *code = cfAstParseBlock(self, &tokenList);
        if (code == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_WHILE_BLOCK_MISSING,
                .whileBlockMissing = (CfStrSpan) { spanBegin, tokenList->span.begin }
            });

        *stmtDst = (CfAstStatement) {
            .type = CF_AST_STATEMENT_TYPE_WHILE,
            .span = (CfStrSpan) { spanBegin, tokenList->span.begin },
            .while_ = {
                .condition = condition,
                .code       = code,
            },
        };
    } else if (cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_RETURN, false) != NULL) {
        CfAstExpression *expr = cfAstParseExpr(self, &tokenList);

        cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_SEMICOLON, true);

        *stmtDst = (CfAstStatement) {
            .type = CF_AST_STATEMENT_TYPE_RETURN,
            .span = (CfStrSpan) { spanBegin, tokenList->span.begin },
            .return_ = expr,
        };
    } else if ((block = cfAstParseBlock(self, &tokenList)) != NULL) {
        *stmtDst = (CfAstStatement) {
            .type = CF_AST_STATEMENT_TYPE_BLOCK,
            .span = block->span,
            .block = block,
        };
    } else if (cfAstParseDecl(self, &tokenList, &decl)) {
        *stmtDst = (CfAstStatement) {
            .type        = CF_AST_STATEMENT_TYPE_DECLARATION,
            .span        = decl.span,
            .declaration = decl,
        };
    } else if ((expr = cfAstParseExpr(self, &tokenList)) != NULL) {
        // parse semicolon after expression
        cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_SEMICOLON, true);

        *stmtDst = (CfAstStatement) {
            .type       = CF_AST_STATEMENT_TYPE_EXPRESSION,
            .span       = expr->span,
            .expression = expr,
        };
    } else {
        return false;
    }

    *tokenListPtr = tokenList;
    return true;
} // cfAstParseStmt

CfAstBlock * cfAstParseBlock( CfAstParser *const self, const CfLexerToken **tokenListPtr ) {
    const CfLexerToken *tokenList = *tokenListPtr;

    if (cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_CURLY_BR_OPEN, false) == NULL)
        return NULL;

    // parse statements
    CfDeque *stmtDeque = cfDequeCtor(sizeof(CfAstStatement), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);

    for (;;) {
        CfAstStatement stmt = {};
        if (!cfAstParseStmt(self, &tokenList, &stmt))
            break;
        cfAstParserAssert(self, cfDequePushBack(stmtDeque, &stmt));
    }

    CfAstBlock *block = (CfAstBlock *)cfAstParserAllocData(self,
        sizeof(CfAstBlock) + sizeof(CfAstStatement) * cfDequeLength(stmtDeque)
    );

    block->statementCount = cfDequeLength(stmtDeque);
    cfDequeWrite(stmtDeque, block->statements);

    cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_CURLY_BR_CLOSE, true);

    *tokenListPtr = tokenList;

    return block;
} // cfAstParseBlock

CfAstFunction cfAstParseFunction( CfAstParser *const self, const CfLexerToken **tokenListPtr ) {
    const CfLexerToken *tokenList = *tokenListPtr;
    uint32_t signatureSpanBegin = tokenList[0].span.begin;

    cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_FN, true);
    CfStr name = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_IDENTIFIER, true)->identifier;
    cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_ROUND_BR_OPEN, true);

    // create param list
    CfDeque *paramDeque = cfDequeCtor(sizeof(CfAstFunctionParam), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);

    for (;;) {
        CfAstFunctionParam param = {};

        // parse function parameter
        if (!cfAstParseFunctionParam(self, &tokenList, &param))
            break;

        // insert param into parameter list
        cfAstParserAssert(self, cfDequePushBack(paramDeque, &param));

        // (try to) parse comma. in case if comma isn't parsed, stop parsing parameters
        if (cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_COMMA, false) == NULL)
            break;
    }

    // parse closing bracket
    cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_ROUND_BR_CLOSE, true);

    // parameter array
    size_t paramArrayLength = cfDequeLength(paramDeque);
    CfAstFunctionParam *paramArray = (CfAstFunctionParam *)cfAstParserAllocData(self,
        sizeof(CfAstFunctionParam) * paramArrayLength
    );
    cfDequeWrite(paramDeque, paramArray);

    // parse return type
    CfAstType outputType = CF_AST_TYPE_VOID;
    if (!cfAstParseType(self, &tokenList, &outputType))
        outputType = CF_AST_TYPE_VOID;

    uint32_t signatureSpanEnd = tokenList[0].span.begin;

    CfAstBlock *impl = cfAstParseBlock(self, &tokenList);

    // parse block
    if (impl == NULL)
        cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_SEMICOLON, true);

    uint32_t spanEnd = tokenList[0].span.end;

    *tokenListPtr = tokenList;

    return (CfAstFunction) {
        .name          = name,
        .inputs        = paramArray,
        .inputCount    = paramArrayLength,
        .outputType    = outputType,
        .signatureSpan = (CfStrSpan) { signatureSpanBegin, signatureSpanEnd },
        .span          = (CfStrSpan) { signatureSpanBegin, spanEnd },
        .impl          = impl,
    };
} // cfAstParseFunction

CfAstVariable cfAstParseVariable( CfAstParser *const self, const CfLexerToken **tokenListPtr ) {
    const CfLexerToken *tokenList = *tokenListPtr;
    uint32_t spanBegin = tokenList->span.begin;

    cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_LET, true);
    CfStr name = cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_IDENTIFIER, true)->identifier;
    cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_COLON, true);

    CfAstType type = CF_AST_TYPE_VOID;
    if (!cfAstParseType(self, &tokenList, &type))
        cfAstParserFinish(self, (CfAstParseResult) {
            .status = CF_AST_PARSE_STATUS_VARIABLE_TYPE_MISSING,
            .variableTypeMissing = (CfStrSpan) { spanBegin, tokenList->span.begin },
        });

    CfAstExpression *init = NULL;
    if (cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_EQUAL, false) != NULL) {
        init = cfAstParseExpr(self, &tokenList);

        if (init == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_VARIABLE_INIT_MISSING,
                .variableInitMissing = (CfStrSpan) { spanBegin, tokenList->span.begin },
            });
    }

    // semicolon required
    cfAstParseToken(self, &tokenList, CF_LEXER_TOKEN_TYPE_SEMICOLON, true);

    uint32_t spanEnd = tokenList->span.begin;

    *tokenListPtr = tokenList;

    return (CfAstVariable) {
        .name = name,
        .type = type,
        .init = init,
        .span = (CfStrSpan) { spanBegin, spanEnd },
    };
} // cfAstParseVariable

bool cfAstParseDecl( CfAstParser *const self, const CfLexerToken **tokenListPtr, CfAstDeclaration *dst ) {
    assert(tokenListPtr != NULL);
    assert(dst != NULL);

    const CfLexerToken *tokenList = *tokenListPtr;

    // it's possible to find out declaraion type by first token
    switch (tokenList[0].type) {
    case CF_LEXER_TOKEN_TYPE_FN: {
        CfAstFunction function = cfAstParseFunction(self, &tokenList);

        *dst = (CfAstDeclaration) {
            .type = CF_AST_DECLARATION_TYPE_FN,
            .span = function.span,
            .fn = function,
        };

        *tokenListPtr = tokenList;
        return true;
    }

    case CF_LEXER_TOKEN_TYPE_LET: {
        CfAstVariable variable = cfAstParseVariable(self, &tokenList);

        *dst = (CfAstDeclaration) {
            .type = CF_AST_DECLARATION_TYPE_LET,
            .span = variable.span,
            .let = variable,
        };

        *tokenListPtr = tokenList;
        return true;
    }

    default:
        return false;
    }
} // cfAstParseDecl

void cfAstParserStart(
    CfAstParser        *const self,
    const CfLexerToken *      tokenList,
    CfAstDeclaration   **     declArrayDst,
    size_t             *      declArrayLenDst
) {
    CfDeque *declList = cfDequeCtor(sizeof(CfAstDeclaration), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);

    cfAstParserAssert(self, declList != NULL);

    const CfLexerToken *restTokens = tokenList;

    for (;;) {
        CfAstDeclaration decl = {};
        if (!cfAstParseDecl(self, &restTokens, &decl))
            break;
        cfAstParserAssert(self, cfDequePushBack(declList, &decl));
    }

    cfAstParseToken(self, &restTokens, CF_LEXER_TOKEN_TYPE_END, true);

    CfAstDeclaration *declArray = (CfAstDeclaration *)cfArenaAlloc(self->dataArena, sizeof(CfAstDeclaration) * cfDequeLength(declList));
    cfAstParserAssert(self, declArray != NULL);
    cfDequeWrite(declList, declArray);

    *declArrayDst = declArray;
    *declArrayLenDst = cfDequeLength(declList);
} // cfAstParse


// cf_ast_parse.c
