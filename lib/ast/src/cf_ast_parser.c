/**
 * @brief AST parser implementation file
 */

#include <assert.h>
#include <setjmp.h>

#include <cf_deque.h>

#include "cf_ast_parser.h"

void cfAstParserFinish( CfAstParser *const self, CfAstParseResult parseResult ) {
    self->parseResult = parseResult;
    longjmp(self->errorBuffer, 1);
} // cfAstParserFinish

void cfAstParserAssert( CfAstParser *const self, bool condition ) {
    if (!condition)
        cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });
} // cfAstParserAssert

void cfAstParseTokenList(
    CfAstParser *const self,
    CfStr              fileContents,
    CfAstToken **      tokenArrayDst,
    size_t      *      tokenArrayLenDst
) {
    assert(tokenArrayDst != NULL);
    assert(tokenArrayLenDst != NULL);

    CfDeque tokenList = cfDequeCtor(sizeof(CfAstToken), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);

    cfAstParserAssert(self, tokenList != NULL);

    CfAstSpan restSpan = { 0, cfStrLength(fileContents) };

    bool endParsed = false;

    while (!endParsed) {
        CfAstTokenParsingResult result = cfAstTokenParse(fileContents, restSpan);

        switch (result.status) {
        case CF_AST_TOKEN_PARSING_STATUS_OK: {
            endParsed = (result.ok.token.type == CF_AST_TOKEN_TYPE_END);

            // ignore comments
            if (result.ok.token.type != CF_AST_TOKEN_TYPE_COMMENT)
                cfAstParserAssert(self, cfDequePushBack(tokenList, &result.ok.token));

            restSpan = result.ok.rest;
            break;
        }

        case CF_AST_TOKEN_PARSING_STATUS_UNEXPECTED_SYMBOL: {
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_UNEXPECTED_SYMBOL,
                .unexpectedSymbol = {
                    .symbol = result.unexpectedSymbol,
                    .offset = restSpan.begin,
                },
            });
            break;
        }
        }
    }

    // allocate 'final' token array
    CfAstToken *tokenArray = (CfAstToken *)cfArenaAlloc(self->tempArena, sizeof(CfAstToken) * cfDequeLength(tokenList));

    cfAstParserAssert(self, tokenArray != NULL);

    cfDequeWrite(tokenList, tokenArray);

    *tokenArrayDst = tokenArray;
    *tokenArrayLenDst = cfDequeLength(tokenList);
} // cfAstParseTokenList

bool cfAstParseType( CfAstParser *const self, const CfAstToken **tokenListPtr, CfAstType *typeDst ) {
    switch ((*tokenListPtr)->type) {
    case CF_AST_TOKEN_TYPE_I32  : *typeDst = CF_AST_TYPE_I32;  break;
    case CF_AST_TOKEN_TYPE_U32  : *typeDst = CF_AST_TYPE_U32;  break;
    case CF_AST_TOKEN_TYPE_F32  : *typeDst = CF_AST_TYPE_F32;  break;
    case CF_AST_TOKEN_TYPE_VOID : *typeDst = CF_AST_TYPE_VOID; break;

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
    const CfAstToken   **      tokenListPtr,
    CfAstFunctionParam  *      paramDst
) {
    const CfAstToken *tokenList = *tokenListPtr;
    CfAstType type;

    if (false
        || tokenList[0].type != CF_AST_TOKEN_TYPE_IDENT
        || tokenList[1].type != CF_AST_TOKEN_TYPE_COLON
        || (tokenList += 2, !cfAstParseType(self, &tokenList, &type))
    )
        return false;

    *paramDst = (CfAstFunctionParam) {
        .name = (*tokenListPtr)[0].ident,
        .type = type,
        .span = (CfAstSpan) { (*tokenListPtr)[0].span.begin, tokenList->span.begin }
    };

    *tokenListPtr = tokenList;
    return true;
} // cfAstParseFunctionParam

const CfAstToken * cfAstParseToken(
    CfAstParser       *const self,
    const CfAstToken **      tokenListPtr,
    CfAstTokenType           expectedType,
    bool                     required
) {
    if ((*tokenListPtr)->type == expectedType)
        return (*tokenListPtr)++;
    if (required)
        cfAstParserFinish(self, (CfAstParseResult) {
            .status = CF_AST_PARSE_STATUS_UNEXPECTED_TOKEN_TYPE,
            .unexpectedTokenType = {
                .actualToken = **tokenListPtr,
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
static bool cfAstParseStmt( CfAstParser *const self, const CfAstToken **tokenListPtr, CfAstStmt *stmtDst ) {
    const CfAstToken *tokenList = *tokenListPtr;

    CfAstBlock *block = NULL;
    CfAstExpr *expr = NULL;
    CfAstDecl decl = {};

    // try to parse new block
    if ((block = cfAstParseBlock(self, &tokenList)) != NULL) {
        stmtDst->type = CF_AST_STMT_TYPE_BLOCK;
        *stmtDst = (CfAstStmt) {
            .type = CF_AST_STMT_TYPE_BLOCK,
            .span = block->span,
            .block = block,
        };
    } else if (cfAstParseDecl(self, &tokenList, &decl)) {
        *stmtDst = (CfAstStmt) {
            .type = CF_AST_STMT_TYPE_DECL,
            .span = decl.span,
            .decl = decl,
        };
    } else if ((expr = cfAstParseExpr(self, &tokenList)) != NULL) {
        // parse semicolon after expression
        cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_SEMICOLON, true);

        *stmtDst = (CfAstStmt) {
            .type = CF_AST_STMT_TYPE_EXPR,
            .span = expr->span,
            .expr = expr,
        };
    } else {
        return false;
    }

    *tokenListPtr = tokenList;
    return true;
} // cfAstParseStmt

CfAstBlock * cfAstParseBlock( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    const CfAstToken *tokenList = *tokenListPtr;

    if (cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_CURLY_BR_OPEN, false) == NULL)
        return NULL;

    // parse statements
    CfDeque stmtDeque = cfDequeCtor(sizeof(CfAstStmt), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);

    for (;;) {
        CfAstStmt stmt = {};
        if (!cfAstParseStmt(self, &tokenList, &stmt))
            break;
        cfAstParserAssert(self, cfDequePushBack(stmtDeque, &stmt));
    }

    CfAstBlock *block = (CfAstBlock *)cfArenaAlloc(
        self->dataArena,
        sizeof(CfAstBlock) + sizeof(CfAstStmt) * cfDequeLength(stmtDeque)
    );
    cfAstParserAssert(self, block != NULL);

    block->stmtCount = cfDequeLength(stmtDeque);
    cfDequeWrite(stmtDeque, block->stmts);

    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_CURLY_BR_CLOSE, true);

    *tokenListPtr = tokenList;

    return block;
} // cfAstParseBlock

CfAstFunction cfAstParseFunction( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    const CfAstToken *tokenList = *tokenListPtr;
    size_t signatureSpanBegin = tokenList[0].span.begin;

    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_FN, true);
    CfStr name = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_IDENT, true)->ident;
    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_ROUND_BR_OPEN, true);

    // create param list
    CfDeque paramDeque = cfDequeCtor(sizeof(CfAstFunctionParam), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);

    for (;;) {
        CfAstFunctionParam param = {};

        // parse function parameter
        if (!cfAstParseFunctionParam(self, &tokenList, &param))
            break;

        // insert param into parameter list
        cfAstParserAssert(self, cfDequePushBack(paramDeque, &param));

        // (try to) parse comma. in case if comma isn't parsed, stop parsing parameters
        if (cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_COMMA, false) == NULL)
            break;
    }

    // parse closing bracket
    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_ROUND_BR_CLOSE, true);

    // parameter array
    size_t paramArrayLength = cfDequeLength(paramDeque);
    CfAstFunctionParam *paramArray = (CfAstFunctionParam *)cfArenaAlloc(
        self->dataArena,
        sizeof(CfAstFunctionParam) * paramArrayLength
    );
    cfAstParserAssert(self, paramArray != NULL);
    cfDequeWrite(paramDeque, paramArray);

    // parse return type
    CfAstType returnType = CF_AST_TYPE_VOID;
    if (!cfAstParseType(self, &tokenList, &returnType))
        returnType = CF_AST_TYPE_VOID;

    size_t signatureSpanEnd = tokenList[0].span.begin;

    CfAstBlock *impl = cfAstParseBlock(self, &tokenList);

    // parse block
    if (impl == NULL)
        cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_SEMICOLON, true);

    size_t spanEnd = tokenList[0].span.end;

    *tokenListPtr = tokenList;

    return (CfAstFunction) {
        .name          = name,
        .params        = paramArray,
        .paramCount    = paramArrayLength,
        .returnType    = returnType,
        .signatureSpan = (CfAstSpan) { signatureSpanBegin, signatureSpanEnd },
        .span          = (CfAstSpan) { signatureSpanBegin, spanEnd },
        .impl          = impl,
    };
} // cfAstParseFunction

CfAstVariable cfAstParseVariable( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    const CfAstToken *tokenList = *tokenListPtr;
    size_t spanBegin = tokenList->span.begin;

    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_LET, true);
    CfStr name = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_IDENT, true)->ident;
    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_COLON, true);

    CfAstType type = CF_AST_TYPE_VOID;
    if (!cfAstParseType(self, &tokenList, &type))
        cfAstParserFinish(self, (CfAstParseResult) {
            .status = CF_AST_PARSE_STATUS_VARIABLE_TYPE_MISSING,
            .variableTypeMissing = tokenList[0],
        });

    CfAstExpr *init = NULL;
    if (cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_EQUAL, false) != NULL) {
        init = cfAstParseExpr(self, &tokenList);

        if (init == NULL)
            cfAstParserFinish(self, (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_VARIABLE_INIT_MISSING,
                .variableInitMissing = *tokenList,
            });
    }

    // semicolon required
    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_SEMICOLON, true);

    size_t spanEnd = tokenList->span.begin;

    *tokenListPtr = tokenList;

    return (CfAstVariable) {
        .name = name,
        .type = type,
        .init = init,
        .span = (CfAstSpan) { spanBegin, spanEnd },
    };
} // cfAstParseVariable

bool cfAstParseDecl( CfAstParser *const self, const CfAstToken **tokenListPtr, CfAstDecl *dst ) {
    assert(tokenListPtr != NULL);
    assert(dst != NULL);

    const CfAstToken *tokenList = *tokenListPtr;

    // it's possible to find out declaraion type by first token
    switch (tokenList[0].type) {
    case CF_AST_TOKEN_TYPE_FN: {
        CfAstFunction function = cfAstParseFunction(self, &tokenList);

        *dst = (CfAstDecl) {
            .type = CF_AST_DECL_TYPE_FN,
            .span = function.span,
            .fn = function,
        };

        *tokenListPtr = tokenList;
        return true;
    }

    case CF_AST_TOKEN_TYPE_LET: {
        CfAstVariable variable = cfAstParseVariable(self, &tokenList);

        *dst = (CfAstDecl) {
            .type = CF_AST_DECL_TYPE_LET,
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

void cfAstParseDecls(
    CfAstParser  *const self,
    CfStr               fileContents,
    CfAstDecl   **      declArrayDst,
    size_t       *      declArrayLenDst
) {
    CfAstToken *tokenArray = NULL;
    size_t tokenArrayLength = 0;

    // tokenize file contents
    cfAstParseTokenList(self, fileContents, &tokenArray, &tokenArrayLength);

    // parse declcarations from tokens
    CfDeque declList = cfDequeCtor(sizeof(CfAstDecl), CF_DEQUE_CHUNK_SIZE_UNDEFINED, self->tempArena);

    cfAstParserAssert(self, declList != NULL);

    const CfAstToken *restTokens = tokenArray;

    for (;;) {
        CfAstDecl decl = {};
        if (!cfAstParseDecl(self, &restTokens, &decl))
            break;
        cfAstParserAssert(self, cfDequePushBack(declList, &decl));
    }

    cfAstParseToken(self, &restTokens, CF_AST_TOKEN_TYPE_END, true);

    CfAstDecl *declArray = (CfAstDecl *)cfArenaAlloc(self->dataArena, sizeof(CfAstDecl) * cfDequeLength(declList));
    cfAstParserAssert(self, declArray != NULL);
    cfDequeWrite(declList, declArray);

    *declArrayDst = declArray;
    *declArrayLenDst = cfDequeLength(declList);
} // cfAstParse


// cf_ast_parse.c
