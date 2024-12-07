/**
 * @brief AST parser implementation file
 */

#include <assert.h>
#include <setjmp.h>

#include <cf_deque.h>

#include "cf_ast_internal.h"

/// @brief AST parsing context
typedef struct __CfAstParser {
    CfArena tempArena; ///< arena to allocate temporary data in (such as intermediate arrays)
    CfArena dataArena; ///< arena to allocate actual AST data in

    jmp_buf          errorBuffer; ///< buffer to jump to if some error occured
    CfAstParseResult parseResult; ///< AST parsing result (used in case if error committed)
} CfAstParser;

/**
 * @brief Immediate AST parsing stopping function
 *
 * @param[in] self        parser pointer
 * @param[in] parseResult result to throw exception with (result MUST BE not ok)
 */
void cfAstParserFinish( CfAstParser *const self, CfAstParseResult parseResult ) {
    self->parseResult = parseResult;
    longjmp(self->errorBuffer, 1);
} // cfAstParserFinish

/**
 * @brief kind-of assertion function
 * 
 * @param[in] self      parser pointer
 * @param[in] condition just some condition
 * 
 * @note this function finishes parsing with INTERNAL_ERROR if condition is false
 */
void cfAstParserAssert( CfAstParser *const self, bool condition ) {
    if (!condition)
        cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });
} // cfAstParserAssert

/**
 * @brief token list parsing function
 * 
 * @param[in]  self             parser pointer
 * @param[in]  fileContents     text to tokenize
 * @param[out] tokenArrayDst    resulting token array start pointer destination (non-null)
 * @param[out] tokenArrayLenDst resulting token array length destination (non-null)
 * 
 * @note
 * - Token array is allocated from temporary arena.
 * 
 * - Token array **is** terminated by token with CF_AST_TOKEN_TYPE_END type
 */
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

/**
 * @brief type from AST parsing function
 * 
 * @param[in] self         parser pointer
 * @param[in] tokenListPtr token list pointer
 * @param[in] typeDst      token type parsing destination
 * 
 * @return true if parsed, false if not
 */
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
bool cfAstParseFunctionParam( CfAstParser *const self, const CfAstToken **tokenListPtr, CfAstFunctionParam *paramDst ) {
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

/**
 * @brief token with certain type parsing function
 * 
 * @param[in] self              parser pointer
 * @param[in] tokenListPtr      token list pointer
 * @param[in] expectedTokenType expected type
 * @param[in] required          if true, function will throw error, if false - return NULL
 * 
 * @return token pointer (non-null if required == true)
 */
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
 * @brief block parsing function
 * 
 * @param[in] self self pointer
 * 
 * @return parsed block pointer (NULL if parsing failed)
 */
CfAstBlock * cfAstParseBlock( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    const CfAstToken *tokenList = *tokenListPtr;

    if (cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_CURLY_BR_OPEN, false) == NULL)
        return NULL;

    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_CURLY_BR_CLOSE, true);

    CfAstBlock *block = (CfAstBlock *)cfArenaAlloc(self->dataArena, sizeof(CfAstBlock));
    cfAstParserAssert(self, block != NULL);

    *tokenListPtr = tokenList;

    return block;
} // cfAstParseBlock

/**
 * @brief function from token list parsing function
 * 
 * @param[in] self         parser pointer
 * @param[in] tokenListPtr list of tokens to parse function from (non-null)
 * 
 * @return parsed function (throws error if failed.)
 * 
 * @note Token list **must** start from CF_AST_TOKEN_TYPE_FN token.
 */
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
    CfAstType returnType = {};
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

/**
 * @brief expression parsing function
 * 
 * @param[in] self      parser pointer
 * @param[in] tokenList token list
 * 
 * @return parsed expression pointer (non-null)
 * 
 * @note This function assumes, that expression is required
 */
CfAstExpr * cfAstParseExpr( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    CfAstExpr *result = (CfAstExpr *)cfArenaAlloc(self->dataArena, sizeof(CfAstExpr));
    cfAstParserAssert(self, result != NULL);

    switch ((*tokenListPtr)->type) {
    case CF_AST_TOKEN_TYPE_INTEGER:
        *result = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_INTEGER,
            .span = (*tokenListPtr)->span,
            .integer = (*tokenListPtr)->integer,
        };
        break;

    case CF_AST_TOKEN_TYPE_FLOATING:
        *result = (CfAstExpr) {
            .type = CF_AST_EXPR_TYPE_FLOATING,
            .span = (*tokenListPtr)->span,
            .floating = (*tokenListPtr)->floating,
        };
        break;

    default:
        cfAstParserFinish(self, (CfAstParseResult) {
            .status = CF_AST_PARSE_STATUS_EXPR_VALUE_REQUIRED,
            .exprValueRequired = **tokenListPtr,
        });
        return NULL;
    }

    (*tokenListPtr)++;
    return result;
} // cfAstParseExpr

/**
 * @brief variable declaration parsing function
 * 
 * @param[in] self parser pointer
 */
CfAstVariable cfAstParseVariable( CfAstParser *const self, const CfAstToken **tokenListPtr ) {
    const CfAstToken *tokenList = *tokenListPtr;
    size_t spanBegin = tokenList->span.begin;

    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_LET, true);
    CfStr name = cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_IDENT, true)->ident;
    cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_COLON, true);

    CfAstType type = {};
    if (!cfAstParseType(self, &tokenList, &type))
        cfAstParserFinish(self, (CfAstParseResult) {
            .status = CF_AST_PARSE_STATUS_VARIABLE_TYPE_MISSING,
            .variableTypeMissing = tokenList[0],
        });

    CfAstExpr *init = NULL;
    if (cfAstParseToken(self, &tokenList, CF_AST_TOKEN_TYPE_EQUAL, false) != NULL)
        init = cfAstParseExpr(self, &tokenList);

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

/**
 * @brief declaration parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr list of tokens to parse declaration from (non-null, no comment tokens in)
 * @param[out]    dst          parsing destination (non-null)
 * 
 * @return true if parsed, false if end reached.
 */
bool cfAstParseDecl( CfAstParser *const self, const CfAstToken **tokenListPtr, CfAstDecl *dst ) {
    assert(tokenListPtr != NULL);
    assert(dst != NULL);

    const CfAstToken *tokenList = *tokenListPtr;

    // check for token end
    if (tokenList[0].type == CF_AST_TOKEN_TYPE_END)
        return false;

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

    // the only reason
    case CF_AST_TOKEN_TYPE_END:
        *tokenListPtr = tokenList + 1;
        return false;

    default:
        cfAstParserFinish(self, (CfAstParseResult) {
            .status = CF_AST_PARSE_STATUS_NOT_DECLARATION_START,
            .notDeclarationStart = tokenList[0],
        });
        return false; // no difference, actually
    }
} // cfAstParseDecl

/**
 * @brief declaration array parsing function
 * 
 * @param[in]  self            parser pointer
 * @param[in]  fileContents    text to parse
 * @param[out] declArryDst     declaration array (non-null)
 * @param[out] declArrayLenDst declcaration array length destination (non-null)
 */
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

CfAstParseResult cfAstParse( CfStr fileName, CfStr fileContents, CfArena tempArena ) {
    // arena that contains actual AST data
    CfArena dataArena = NULL;
    CfAst ast = NULL;

    if (false
        || (dataArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED)) == NULL
        || (ast = (CfAst)cfArenaAlloc(dataArena, sizeof(CfAstImpl))) == NULL
    ) {
        cfArenaDtor(dataArena);
        return (CfAstParseResult) { .status = CF_AST_PARSE_STATUS_INTERNAL_ERROR };
    }

    bool tempArenaOwned = false;

    // allocate temporary arena if it's not already allocated
    if (tempArena == NULL) {
        tempArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED);
        if (tempArena == NULL) {
            cfArenaDtor(tempArena);
            return (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_INTERNAL_ERROR,
            };
        }
        tempArenaOwned = true;
    }

    CfAstParser parser = {
        .tempArena = tempArena,
        .dataArena = dataArena,
    };

    int isError = setjmp(parser.errorBuffer);

    if (isError) {
        // perform cleanup
        if (tempArenaOwned)
            cfArenaDtor(tempArena);
        cfArenaDtor(dataArena);
        return parser.parseResult;
    }

    CfAstDecl *declArray = NULL;
    size_t declArrayLen = 0;

    cfAstParseDecls(&parser, fileContents, &declArray, &declArrayLen);

    // destroy temp arena in case if it's created in this function
    if (tempArenaOwned)
        cfArenaDtor(tempArena);

    // assemble AST from parts.
    *ast = (CfAstImpl) {
        .mem            = dataArena,
        .sourceName     = fileName,
        .sourceContents = fileContents,
        .declArray      = declArray,
        .declArrayLen   = declArrayLen,
    };

    return (CfAstParseResult) {
        .status = CF_AST_PARSE_STATUS_OK,
        .ok = ast,
    };
} // cfAstParse


// cf_ast_parse.c
