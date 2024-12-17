/**
 * @brief AST internal declaration file
 */

#ifndef CF_AST_INTERNAL_H_
#define CF_AST_INTERNAL_H_

#include <setjmp.h>

#include "cf_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief AST structure declaration
struct CfAst_ {
    CfArena           * dataArena;      ///< AST allocation holder
    CfAstDeclaration  * declArray;      ///< declaration array (extends beyond structure memory for declCount - 1 elements)
    size_t              declArrayLen;   ///< declaration array length
}; // struct CfAst_

/// @brief AST parsing context
typedef struct CfAstParser_ {
    CfArena          * tempArena;   ///< arena to allocate temporary data in (such as intermediate arrays)
    CfArena          * dataArena;   ///< arena to allocate actual AST data in

    jmp_buf            errorBuffer; ///< buffer to jump to if some error occured
    CfAstParseResult   parseResult; ///< AST parsing result (used in case if error committed)
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
 * @brief allocate memory at data arena
 * 
 * @param[in] self parser pointer
 * @param[in] size required size
 * 
 * @return allocated data pointer (non-null)
 * 
 * @note in allocation fail case, finishes with INTERNAL_ERROR
 */
void * cfAstParserAllocData( CfAstParser *const self, size_t size );

/**
 * @brief type from AST parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * @param[out]    typeDst      token type parsing destination
 * 
 * @return true if parsed, false if not
 */
bool cfAstParseType( CfAstParser *const self, const CfLexerToken **tokenListPtr, CfAstType *typeDst );

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
const CfLexerToken * cfAstParseToken(
    CfAstParser       *const self,
    const CfLexerToken **      tokenListPtr,
    CfLexerTokenType           expectedType,
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
CfAstBlock * cfAstParseBlock( CfAstParser *const self, const CfLexerToken **tokenListPtr );

/**
 * @brief function from token list parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr list of tokens to parse function from (non-null)
 * 
 * @return parsed function (throws error if failed.)
 * 
 * @note Token list **must** start from CF_LEXER_TOKEN_TYPE_FN token.
 */
CfAstFunction cfAstParseFunction( CfAstParser *const self, const CfLexerToken **tokenListPtr );

/**
 * @brief expression parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * 
 * @return parsed expression pointer (may be null in case if *tokenListPtr array does not starts from valid expression)
 */
CfAstExpression * cfAstParseExpr( CfAstParser *const self, const CfLexerToken **tokenListPtr );

/**
 * @brief variable declaration parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr token list pointer
 * 
 * @return parsed variable declaration
 */
CfAstVariable cfAstParseVariable( CfAstParser *const self, const CfLexerToken **tokenListPtr );

/**
 * @brief declaration parsing function
 * 
 * @param[in]     self         parser pointer
 * @param[in,out] tokenListPtr list of tokens to parse declaration from (non-null, no comment tokens in)
 * @param[out]    dst          parsing destination (non-null)
 * 
 * @return true if parsed, false if end reached.
 */
bool cfAstParseDecl( CfAstParser *const self, const CfLexerToken **tokenListPtr, CfAstDeclaration *dst );

/**
 * @brief parser (as 'self') main function
 * 
 * @param[in]  self            parser pointer
 * @param[in]  tokenList       token list to parse
 * @param[out] declArryDst     declaration array (non-null)
 * @param[out] declArrayLenDst declcaration array length destination (non-null)
 */
void cfAstParserStart(
    CfAstParser        *const self,
    const CfLexerToken *      tokenList,
    CfAstDeclaration   **     declArrayDst,
    size_t             *      declArrayLenDst
);

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_INTERNAL_H_)

// cf_ast_internal.h
