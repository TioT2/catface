/**
 * @brief lexer main implementation file
 */

#include <string.h>
#include <ctype.h>
#include <math.h>

#include "cf_lexer.h"

/**
 * @brief keyword from identifier parsing function
 * 
 * @param[in]  identifier identifier to parse keyword from
 * @param[out] kwDst kryword destination (non-null)
 * 
 * @return true if keyword parsed, false if not.
 */
static bool cfLexerTokenKeywordFromIdent( CfStr identifier, CfLexerTokenType *kwDst ) {
    struct {
        const char      * text;
        CfLexerTokenType  type;
    } table[] = {
        {"fn",    CF_LEXER_TOKEN_TYPE_FN    },
        {"let",   CF_LEXER_TOKEN_TYPE_LET   },
        {"i32",   CF_LEXER_TOKEN_TYPE_I32   },
        {"u32",   CF_LEXER_TOKEN_TYPE_U32   },
        {"f32",   CF_LEXER_TOKEN_TYPE_F32   },
        {"void",  CF_LEXER_TOKEN_TYPE_VOID  },
        {"if",    CF_LEXER_TOKEN_TYPE_IF    },
        {"else",  CF_LEXER_TOKEN_TYPE_ELSE  },
        {"while", CF_LEXER_TOKEN_TYPE_WHILE },
    };

    for (size_t i = 0, n = sizeof(table) / sizeof(table[0]); i < n; i++) {
        if (cfStrIsSame(identifier, CF_STR(table[i].text))) {
            *kwDst = table[i].type;
            return true;
        }
    }

    return false;
} // cfLexerTokenKeywordFromIdent

/**
 * @brief hexadecimal digit into number parsing function
 *
 * @param[in] ch hexadecimal digit ( [0-9a-zA-Z] )
 *
 * @return number corresponding to the digit. if number isn't hexadecimal digit, output is undefined.
 */
static uint32_t cfLexerTokenParseHexDigit( char ch ) {
    return
          ch <= '9' ? ch - '0'
        : ch <= 'Z' ? ch - 'A'
        : ch <= 'z' ? ch - 'a'
        : 0;
} // cfLexerTokenParseHexDigit

bool cfLexerParseToken( CfStr source, CfStrSpan span, CfLexerToken *tokenDst ) {
    CfStr str = cfStrSubstr(source, span);

    while (str.begin < str.end && isspace(*str.begin))
        str.begin++;

    // check if there is at least one symbol to parse available
    if (str.begin == str.end) {
        *tokenDst = (CfLexerToken) { .type = CF_LEXER_TOKEN_TYPE_END };
        return true;
    }

    // offset from current str begin to span begin
    size_t strOffset = str.begin - (source.begin + span.begin);

    // parse comment
    if (cfStrStartsWith(str, "//")) {
        CfStr comment = { .begin = str.begin + 2, .end = str.begin + 2 };

        while (comment.end < str.end && *comment.end != '\n')
            comment.end++;

        size_t length = cfStrLength(comment);

        *tokenDst = (CfLexerToken) {
            .type = CF_LEXER_TOKEN_TYPE_COMMENT,
            .span = (CfStrSpan) {
                .begin = (uint32_t)(comment.begin - source.begin),
                .end   = (uint32_t)(comment.end   - source.begin),
            }
        };
        return true;
    }

    // only number may start from digit, actually
    if (isdigit(*str.begin)) {
        const char *start = str.begin;

        uint32_t base =
              cfStrStartsWith(str, "0x") ? 16
            : cfStrStartsWith(str, "0o") ? 8
            : cfStrStartsWith(str, "0b") ? 2
            : 10;
        uint32_t offset = base != 10 ? 2 : 0;

        bool isFloat = false;

        str.begin += offset;

        uint64_t integer = 0;
        int64_t exponent = 0;
        double fractional = 0.0;

        while (isxdigit(*str.begin)) {
            uint32_t digit = cfLexerTokenParseHexDigit(*str.begin);

            if (digit >= base)
                break;

            integer = integer * base + digit;
            str.begin++;
        }

        // if number is decimal, try to parse float
        if (base == 10) {
            if (str.begin < str.end && *str.begin == '.') {
                str.begin++;
                isFloat = true;

                double exp = 1.0;

                while (str.begin < str.end && isdigit(*str.begin)) {
                    exp *= 0.1;
                    fractional += exp * (double)(*str.begin - '0');
                    str.begin++;
                }
            }

            if (str.begin < str.end && *str.begin == 'e') {
                str.begin++;
                isFloat = true;
                int64_t expSign = 1;

                if (str.begin < str.end && (*str.begin == '-' || *str.begin == '+')) {
                    expSign = (*str.begin == '-') ? -1 : 1;
                    str.begin++;
                }

                while (str.begin < str.end && isdigit(*str.begin)) {
                    exponent = exponent * 10 + (*str.begin - '0');
                    str.begin++;
                }
                exponent *= expSign;
            }
        }

        CfStrSpan tokenSpan = (CfStrSpan) { (uint32_t)(start - source.begin), (uint32_t)(str.begin - source.begin) };

        *tokenDst = isFloat
            ? (CfLexerToken) {
                .type = CF_LEXER_TOKEN_TYPE_FLOATING,
                .span = tokenSpan,
                .floating = ((double)integer + fractional) * (double)pow(10.0, (double)exponent)
            }
            : (CfLexerToken) {
                .type = CF_LEXER_TOKEN_TYPE_INTEGER,
                .span = tokenSpan,
                .integer = integer,
            };
        return true;
    }

    // try to match double-character tokens
    if (str.end - str.begin >= 2) {
        bool found = true;

        // switch can't be used here because compile-time string literal
        // to uint16_t conversion isn't possible within C language.
        static const struct {
            const char       * pattern; ///< pattern to match
            CfLexerTokenType   type;    ///< corresponding token type
        } tokens[] = {
            {"<=", CF_LEXER_TOKEN_TYPE_ANGULAR_BR_OPEN_EQUAL  },
            {">=", CF_LEXER_TOKEN_TYPE_ANGULAR_BR_CLOSE_EQUAL },
            {"==", CF_LEXER_TOKEN_TYPE_EQUAL_EQUAL            },
            {"!=", CF_LEXER_TOKEN_TYPE_EXCLAMATION_EQUAL      },
            {"+=", CF_LEXER_TOKEN_TYPE_PLUS_EQUAL             },
            {"-=", CF_LEXER_TOKEN_TYPE_MINUS_EQUAL            },
            {"*=", CF_LEXER_TOKEN_TYPE_ASTERISK_EQUAL         },
            {"/=", CF_LEXER_TOKEN_TYPE_SLASH_EQUAL            },
        };
        uint16_t strPattern = *(const uint16_t *)str.begin;
        uint32_t tokSpanBegin = str.begin - source.begin;

        for (uint32_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++)
            if (*(const uint16_t *)tokens[i].pattern == strPattern) {
                *tokenDst = (CfLexerToken) {
                    .type = tokens[i].type,
                    .span = (CfStrSpan) { tokSpanBegin, tokSpanBegin + 2 },
                };
                return true;
            }
    }

    // try to match single-character tokens
    bool found = true;
    CfLexerTokenType type = CF_LEXER_TOKEN_TYPE_END;

    // match single-character token
    switch (*str.begin) {
    case '<': type = CF_LEXER_TOKEN_TYPE_ANGULAR_BR_OPEN  ; break;
    case '>': type = CF_LEXER_TOKEN_TYPE_ANGULAR_BR_CLOSE ; break;
    case ':': type = CF_LEXER_TOKEN_TYPE_COLON            ; break;
    case ';': type = CF_LEXER_TOKEN_TYPE_SEMICOLON        ; break;
    case ',': type = CF_LEXER_TOKEN_TYPE_COMMA            ; break;
    case '=': type = CF_LEXER_TOKEN_TYPE_EQUAL            ; break;
    case '+': type = CF_LEXER_TOKEN_TYPE_PLUS             ; break;
    case '-': type = CF_LEXER_TOKEN_TYPE_MINUS            ; break;
    case '*': type = CF_LEXER_TOKEN_TYPE_ASTERISK         ; break;
    case '/': type = CF_LEXER_TOKEN_TYPE_SLASH            ; break;
    case '{': type = CF_LEXER_TOKEN_TYPE_CURLY_BR_OPEN    ; break;
    case '}': type = CF_LEXER_TOKEN_TYPE_CURLY_BR_CLOSE   ; break;
    case '(': type = CF_LEXER_TOKEN_TYPE_ROUND_BR_OPEN    ; break;
    case ')': type = CF_LEXER_TOKEN_TYPE_ROUND_BR_CLOSE   ; break;
    case '[': type = CF_LEXER_TOKEN_TYPE_SQUARE_BR_OPEN   ; break;
    case ']': type = CF_LEXER_TOKEN_TYPE_SQUARE_BR_CLOSE  ; break;
    default:
        found = false;
    }

    if (found) {
        uint32_t spanBegin = str.begin - source.begin;

        *tokenDst = (CfLexerToken) {
            .type = type,
            .span = (CfStrSpan) { spanBegin, spanBegin + 1 },
        };
        return true;
    }

    // try to parse identifier
    if (isalpha(*str.begin) || *str.begin == '_') {
        CfStr identifier = { str.begin, str.begin };

        while (identifier.end < str.end && isalnum(*identifier.end) || *identifier.end == '_')
            identifier.end++;

        // try to separate keyword from identifier
        CfLexerTokenType ty = CF_LEXER_TOKEN_TYPE_END;

        *tokenDst = cfLexerTokenKeywordFromIdent(identifier, &ty)
            ? (CfLexerToken) { .type = ty, }
            : (CfLexerToken) { .type = CF_LEXER_TOKEN_TYPE_IDENTIFIER, .identifier = identifier, };

        tokenDst->span = (CfStrSpan) {
            (uint32_t)(identifier.begin - source.begin),
            (uint32_t)(identifier.end   - source.begin)
        };
        return true;
    }

    return false;
} // cfLexerParseToken

// cf_lexer.c
