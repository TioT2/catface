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
 * - Token array IS terminated by token with type = CF_AST_TOKEN_TYPE_END
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

    // check if tokenList is constructed
    if (tokenList == NULL)
        cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });

    CfAstSpan restSpan = { 0, cfStrLength(fileContents) };

    bool endParsed = false;

    while (!endParsed) {
        CfAstTokenParsingResult result = cfAstTokenParse(fileContents, restSpan);

        switch (result.status) {
        case CF_AST_TOKEN_PARSING_STATUS_OK: {
            endParsed = (result.ok.token.type = CF_AST_TOKEN_TYPE_END);

            // ignore comments
            if (result.ok.token.type != CF_AST_TOKEN_TYPE_COMMENT)
                if (!cfDequePushBack(tokenList, &result.ok.token))
                    cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });

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

    if (tokenArray == NULL)
        cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });

    cfDequeWrite(tokenList, tokenArray);

    *tokenArrayDst = tokenArray;
    *tokenArrayLenDst = cfDequeLength(tokenList);
} // cfAstParseTokenList

/**
 * @brief declaration parsing function
 * 
 * @param[in]     self      parser pointer
 * @param[in,out] tokenList list of tokens to parse declaration from
 * @param[out]    dst       parsing destination
 * 
 * @return true if parsed, false if end reached.
 */
bool cfAstParseDecl( CfAstParser *const self, const CfAstToken **tokenList, CfAstDecl *dst ) {
    cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });
    return false;
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

    if (declList == NULL)
        return cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });

    const CfAstToken *restTokens = tokenArray;

    for (;;) {
        CfAstDecl decl = {};
        if (!cfAstParseDecl(self, &restTokens, &decl))
            break;

        if (!cfDequePushBack(declList, &decl))
            cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });
    }

    CfAstDecl *declArray = (CfAstDecl *)cfArenaAlloc(self->dataArena, sizeof(CfAstDecl) * cfDequeLength(declList));

    if (declArray == NULL)
        cfAstParserFinish(self, (CfAstParseResult) { CF_AST_PARSE_STATUS_INTERNAL_ERROR });

    *declArrayDst = declArray;
    *declArrayLenDst = cfDequeLength(declList);
} // cfAstParse

CfAstParseResult cfAstParse( CfStr fileName, CfStr fileContents, CfArena tempArena ) {
    // arena that contains actual AST data
    CfArena dataArena = NULL;
    CfAst ast = NULL;

    if (false
        || (dataArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED)) == NULL
        || ((ast == cfArenaAlloc(dataArena, sizeof(CfAstImpl))))
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
