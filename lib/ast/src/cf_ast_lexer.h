/**
 * @brief lexer declaration file
 */

#ifndef CF_AST_LEXER_H_
#define CF_AST_LEXER_H_

#include "cf_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief token type (union tag)
typedef enum __CFAstTokenType {
    CF_AST_TOKEN_TYPE_INTEGER,         ///< integer constant
    CF_AST_TOKEN_TYPE_FLOATING,        ///< floating-point constant
    CF_AST_TOKEN_TYPE_IDENT,           ///< ident

    CF_AST_TOKEN_TYPE_FN,              ///< "fn"   keyword
    CF_AST_TOKEN_TYPE_LET,             ///< "let"  keyword
    CF_AST_TOKEN_TYPE_I32,             ///< "i32"  keyword
    CF_AST_TOKEN_TYPE_U32,             ///< "u32"  keyword
    CF_AST_TOKEN_TYPE_F32,             ///< "f32"  keyword
    CF_AST_TOKEN_TYPE_VOID,            ///< "void" keyword

    CF_AST_TOKEN_TYPE_COLON,           ///< ':' symbol
    CF_AST_TOKEN_TYPE_CURLY_BR_OPEN,   ///< '{' symbol
    CF_AST_TOKEN_TYPE_CURLY_BR_CLOSE,  ///< '}' symbol
    CF_AST_TOKEN_TYPE_ROUND_BR_OPEN,   ///< '(' symbol
    CF_AST_TOKEN_TYPE_ROUND_BR_CLOSE,  ///< ')' symbol
    CF_AST_TOKEN_TYPE_SQUARE_BR_OPEN,  ///< '[' symbol
    CF_AST_TOKEN_TYPE_SQUARE_BR_CLOSE, ///< ']' symbol

    CF_AST_TOKEN_TYPE_COMMENT,         ///< comment
} CfAstTokenType;

/// @brief token representation structure (tagged union, actually)
typedef struct __CfAstToken {
    CfAstTokenType type; ///< token kind
    CfAstSpan      span; ///< span this token occupies

    union {
        CfStr    ident;    ///< ident
        uint64_t integer;  ///< integer constant
        double   floating; ///< floating-point constant
        CfStr    comment;  ///< comment token
    };
} CfAstToken;

/// @brief token parsing status
typedef enum __CfAstTokenParsingStatus {
    CF_AST_TOKEN_PARSING_STATUS_OK,                ///< parsing succeeded
    CF_AST_TOKEN_PARSING_STATUS_END,               ///< parsing actually finished
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

        char unexpectedSymbol; ///< symbol that theoretically can't be part of any ident
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
CfAstTokenParsingResult cfAstNextToken( CfStr str, CfAstSpan span );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_LEXER_H_)