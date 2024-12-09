/**
 * @brief AST parser declaration file
 */

#ifndef CF_AST_PARSER_H_
#define CF_AST_PARSER_H_

#include "cf_ast_internal.h"
#include "cf_ast_lexer.h"

#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

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
void cfAstParserFinish( CfAstParser *const self, CfAstParseResult parseResult );

/**
 * @brief kind-of assertion function
 * 
 * @param[in] self      parser pointer
 * @param[in] condition just some condition
 * 
 * @note this function finishes parsing with INTERNAL_ERROR if condition is false
 */
void cfAstParserAssert( CfAstParser *const self, bool condition );

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
);

/**
 * @brief type from AST parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * @param[out]    typeDst      token type parsing destination
 * 
 * @return true if parsed, false if not
 */
bool cfAstParseType( CfAstParser *const self, const CfAstToken **tokenListPtr, CfAstType *typeDst );

/**
 * @brief token with certain type parsing function
 * 
 * @param[in]     self              parser pointer
 * @param[in,out] tokenListPtr      token list pointer
 * @param[in]     expectedTokenType expected type
 * @param[in]     required          if true, function will throw error, if false - return NULL
 * 
 * @return token pointer (non-null if required == true)
 */
const CfAstToken * cfAstParseToken(
    CfAstParser       *const self,
    const CfAstToken **      tokenListPtr,
    CfAstTokenType           expectedType,
    bool                     required
);

/**
 * @brief block parsing function
 * 
 * @param[in] self             self pointer
 * @param[in,out] tokenListPtr token list pointer
 * 
 * @return parsed block pointer (NULL if parsing failed)
 */
CfAstBlock * cfAstParseBlock( CfAstParser *const self, const CfAstToken **tokenListPtr );

/**
 * @brief function from token list parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr list of tokens to parse function from (non-null)
 * 
 * @return parsed function (throws error if failed.)
 * 
 * @note Token list **must** start from CF_AST_TOKEN_TYPE_FN token.
 */
CfAstFunction cfAstParseFunction( CfAstParser *const self, const CfAstToken **tokenListPtr );

/**
 * @brief expression parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * 
 * @return parsed expression pointer (may be null)
 */
CfAstExpr * cfAstParseExpr( CfAstParser *const self, const CfAstToken **tokenListPtr );

/**
 * @brief variable declaration parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * 
 * @return parsed variable declaration
 */
CfAstVariable cfAstParseVariable( CfAstParser *const self, const CfAstToken **tokenListPtr );

/**
 * @brief declaration parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr list of tokens to parse declaration from (non-null, no comment tokens in)
 * @param[out]    dst          parsing destination (non-null)
 * 
 * @return true if parsed, false if end reached.
 */
bool cfAstParseDecl( CfAstParser *const self, const CfAstToken **tokenListPtr, CfAstDecl *dst );

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
);

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_PARSER_H_)

// cf_ast_parser.h
