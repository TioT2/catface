/**
 * @brief lexer library header
 */

#ifndef CF_LEXER_H_
#define CF_LEXER_H_

#include <cf_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief token type (union tag)
typedef enum CfLexerTokenType_ {
    CF_LEXER_TOKEN_TYPE_INTEGER,         ///< integer constant
    CF_LEXER_TOKEN_TYPE_FLOATING,        ///< floating-point constant
    CF_LEXER_TOKEN_TYPE_IDENTIFIER,      ///< identifier

    CF_LEXER_TOKEN_TYPE_FN,              ///< "fn"    keyword
    CF_LEXER_TOKEN_TYPE_LET,             ///< "let"   keyword
    CF_LEXER_TOKEN_TYPE_I32,             ///< "i32"   keyword
    CF_LEXER_TOKEN_TYPE_U32,             ///< "u32"   keyword
    CF_LEXER_TOKEN_TYPE_F32,             ///< "f32"   keyword
    CF_LEXER_TOKEN_TYPE_VOID,            ///< "void"  keyword
    CF_LEXER_TOKEN_TYPE_IF,              ///< "if"    keyword
    CF_LEXER_TOKEN_TYPE_ELSE,            ///< "else"  keyword
    CF_LEXER_TOKEN_TYPE_WHILE,           ///< "while" keyword

    CF_LEXER_TOKEN_TYPE_ANGULAR_BR_OPEN_EQUAL,  ///< "<=" character combination
    CF_LEXER_TOKEN_TYPE_ANGULAR_BR_CLOSE_EQUAL, ///< ">=" character combination
    CF_LEXER_TOKEN_TYPE_EQUAL_EQUAL,            ///< "==" character combination
    CF_LEXER_TOKEN_TYPE_EXCLAMATION_EQUAL,      ///< "!=" character combination
    CF_LEXER_TOKEN_TYPE_PLUS_EQUAL,             ///< "+=" character combination
    CF_LEXER_TOKEN_TYPE_MINUS_EQUAL,            ///< "-=" character combination
    CF_LEXER_TOKEN_TYPE_ASTERISK_EQUAL,         ///< "*=" character combination
    CF_LEXER_TOKEN_TYPE_SLASH_EQUAL,            ///< "/=" character combination

    CF_LEXER_TOKEN_TYPE_ANGULAR_BR_OPEN,        ///< '<' symbol
    CF_LEXER_TOKEN_TYPE_ANGULAR_BR_CLOSE,       ///< '>' symbol

    CF_LEXER_TOKEN_TYPE_COLON,           ///< ':' symbol
    CF_LEXER_TOKEN_TYPE_SEMICOLON,       ///< ';' symbol
    CF_LEXER_TOKEN_TYPE_COMMA,           ///< ',' symbol
    CF_LEXER_TOKEN_TYPE_EQUAL,           ///< '=' symbol
    CF_LEXER_TOKEN_TYPE_PLUS,            ///< '+' symbol
    CF_LEXER_TOKEN_TYPE_MINUS,           ///< '-' symbol
    CF_LEXER_TOKEN_TYPE_ASTERISK,        ///< '*' symbol
    CF_LEXER_TOKEN_TYPE_SLASH,           ///< '/' symbol
    CF_LEXER_TOKEN_TYPE_CURLY_BR_OPEN,   ///< '{' symbol
    CF_LEXER_TOKEN_TYPE_CURLY_BR_CLOSE,  ///< '}' symbol
    CF_LEXER_TOKEN_TYPE_ROUND_BR_OPEN,   ///< '(' symbol
    CF_LEXER_TOKEN_TYPE_ROUND_BR_CLOSE,  ///< ')' symbol
    CF_LEXER_TOKEN_TYPE_SQUARE_BR_OPEN,  ///< '[' symbol
    CF_LEXER_TOKEN_TYPE_SQUARE_BR_CLOSE, ///< ']' symbol

    CF_LEXER_TOKEN_TYPE_COMMENT,         ///< comment
    CF_LEXER_TOKEN_TYPE_END,             ///< text ending token
} CfLexerTokenType;

/// @brief token representation structure (tagged union, actually)
typedef struct CfLexerToken_ {
    CfLexerTokenType type; ///< token kind
    CfStrSpan        span; ///< span this token occupies

    union {
        CfStr    identifier; ///< identifier
        uint64_t integer;    ///< integer constant
        double   floating;   ///< floating-point constant
        CfStr    comment;    ///< comment token
    };
} CfLexerToken;

/**
 * @brief token from string parsing function
 * 
 * @param[in] str  string to parse token from
 * @param[in] span span to parse token from
 * 
 * @return true if parsed, false if not
 */
bool cfLexerParseToken( CfStr str, CfStrSpan span, CfLexerToken *tokenDst );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_LEXER_H_)

// cf_lexer.h
