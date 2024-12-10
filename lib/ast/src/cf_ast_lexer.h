/**
 * @brief lexer declaration file
 */

#ifndef CF_AST_LEXER_H_
#define CF_AST_LEXER_H_

#include "cf_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// quite good example of brain Rust

/// @brief token parsing status
typedef enum __CfAstTokenParsingStatus {
    CF_AST_TOKEN_PARSING_STATUS_OK,                ///< parsing succeeded
    CF_AST_TOKEN_PARSING_STATUS_UNEXPECTED_SYMBOL, ///< symbol that can't be part of any token
} CfAstTokenParsingStatus;

/// @brief token parsing result tagged union
typedef struct __CfTokenParsingResult {
    CfAstTokenParsingStatus status; ///< operation status

    union {
        struct {
            CfAstSpan  rest;  ///< span that holds rest of text to parse
            CfAstToken token; ///< parsed token
        } ok;

        char unexpectedSymbol; ///< symbol that theoretically can't be part of any identifier
    };
} CfAstTokenParsingResult;

/**
 * @brief next token parsing function
 * 
 * @param[in]  str  string to parse token from
 * @param[out] span region of this string to parse token from
 * 
 * @return token parsing result
 */
CfAstTokenParsingResult cfAstTokenParse( CfStr str, CfAstSpan span );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_LEXER_H_)
