/**
 * @brief lexer implementation file
 */

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "cf_ast_lexer.h"

/**
 * @brief keyword from ident parsing function
 * 
 * @param[in]  ident ident to parse keyword from
 * @param[out] kwDst kryword destination (non-null)
 * 
 * @return true if keyword parsed, false if not.
 */
static bool cfAstTokenKeywordFromIdent( CfStr ident, CfAstTokenType *kwDst ) {
    struct {
        const char    * text;
        CfAstTokenType  type;
    } table[] = {
        {"fn",   CF_AST_TOKEN_TYPE_FN   },
        {"let",  CF_AST_TOKEN_TYPE_LET  },
        {"i32",  CF_AST_TOKEN_TYPE_I32  },
        {"u32",  CF_AST_TOKEN_TYPE_U32  },
        {"f32",  CF_AST_TOKEN_TYPE_F32  },
        {"void", CF_AST_TOKEN_TYPE_VOID },
    };

    for (size_t i = 0, n = sizeof(table) / sizeof(table[0]); i < n; i++) {
        if (cfStrIsSame(ident, CF_STR(table[i].text))) {
            *kwDst = table[i].type;
            return true;
        }
    }

    return false;
} // cfAstTokenKeywordFromIdent

/**
 * @brief hexadecimal digit into number parsing function
 *
 * @param[in] ch hexadecimal digit ( [0-9a-zA-Z] )
 *
 * @return number corresponding to the digit. if number isn't hexadecimal digit, output is undefined.
 */
static uint32_t cfAstTokenParseHexDigit( char ch ) {
    return
          ch <= '9' ? ch - '0'
        : ch <= 'Z' ? ch - 'A'
        : ch <= 'z' ? ch - 'a'
        : 0;
} // cfAstTokenParseHexDigit

CfAstTokenParsingResult cfAstTokenParse( CfStr source, CfAstSpan span ) {
    CfStr str = cfAstSpanCutStr(span, source);

    while (str.begin < str.end && isspace(*str.begin))
        str.begin++;

    // check if there is at least one symbol to parse available
    if (str.begin == str.end)
        return (CfAstTokenParsingResult) {
            .status = CF_AST_TOKEN_PARSING_STATUS_OK,
            .ok = {
                .rest = span,
                .token = (CfAstToken) {
                    .type = CF_AST_TOKEN_TYPE_END,
                    .span = span,
                },
            },
        };

    // offset from current str begin to span begin
    size_t strOffset = str.begin - (source.begin + span.begin);

    // parse comment
    if (cfStrStartsWith(str, "//")) {
        CfStr comment = { .begin = str.begin + 2, .end = str.begin + 2 };

        while (comment.end < str.end && *comment.end != '\n')
            comment.end++;

        size_t length = cfStrLength(comment);

        return (CfAstTokenParsingResult) {
            .status = CF_AST_TOKEN_PARSING_STATUS_OK,
            .ok = {
                .rest = (CfAstSpan) {
                    .begin = (size_t)(comment.end - source.begin),
                    .end = span.end
                },
                .token = (CfAstToken) {
                    .type = CF_AST_TOKEN_TYPE_COMMENT,
                    .span = (CfAstSpan) {
                        .begin = (size_t)(comment.begin - source.begin),
                        .end = (size_t)(comment.end - source.begin),
                    }
                }
            },
        };
    }

    // only number may start from digit, actually
    if (isdigit(*str.begin)) {
        const char *start = str.begin;

        uint32_t base =
              cfStrStartsWith(str, "0x") ? 16
            : cfStrStartsWith(str, "0o") ? 8
            : cfStrStartsWith(str, "0b") ? 1
            : 10;

        uint32_t offset = str.end - str.begin > 1 && !isdigit(str.begin[1])
            ? 2
            : 0
        ;

        bool isFloat = false;

        str.begin += offset;

        uint64_t integer = 0;
        uint64_t exponent = 0;
        double fractional = 0.0;

        while (isxdigit(*str.begin)) {
            uint32_t digit = cfAstTokenParseHexDigit(*str.begin);

            if (digit >= base)
                break;

            integer = integer * base + digit;
            str.begin++;
        }

        // if number is decimal, try to parse float
        if (base == 10) {
            if (*str.begin == '.') {
                isFloat = true;

                double exp = 1.0;

                while (isdigit(*str.begin)) {
                    exp *= 0.1;
                    fractional += exp * (double)(*str.begin - '0');
                }
            }

            if (*str.begin == 'e') {
                isFloat = true;

                while (isdigit(*str.begin)) {
                    exponent = exponent * 10 + (*str.begin - '0');
                }
            }
        }

        CfAstSpan tokenSpan = (CfAstSpan) { (size_t)(start - source.begin), (size_t)(str.begin - source.begin) };

        return (CfAstTokenParsingResult) {
            .status = CF_AST_TOKEN_PARSING_STATUS_OK,
            .ok = {
                .rest = (CfAstSpan) { (size_t)(str.begin - source.begin), span.end },
                .token = isFloat
                    ? (CfAstToken) {
                        .type = CF_AST_TOKEN_TYPE_FLOATING,
                        .span = tokenSpan,
                        .floating = ((double)integer + fractional) * (double)pow(10.0, (double)exponent)
                    }
                    : (CfAstToken) {
                        .type = CF_AST_TOKEN_TYPE_INTEGER,
                        .span = tokenSpan,
                        .integer = integer,
                    }
            }
        };
    }

    bool found = true;
    CfAstTokenType type = CF_AST_TOKEN_TYPE_VOID;

    // match single-character token
    switch (*str.begin) {
    case ':': type = CF_AST_TOKEN_TYPE_COLON           ; break;
    case '{': type = CF_AST_TOKEN_TYPE_CURLY_BR_OPEN   ; break;
    case '}': type = CF_AST_TOKEN_TYPE_CURLY_BR_CLOSE  ; break;
    case '(': type = CF_AST_TOKEN_TYPE_ROUND_BR_OPEN   ; break;
    case ')': type = CF_AST_TOKEN_TYPE_ROUND_BR_CLOSE  ; break;
    case '[': type = CF_AST_TOKEN_TYPE_SQUARE_BR_OPEN  ; break;
    case ']': type = CF_AST_TOKEN_TYPE_SQUARE_BR_CLOSE ; break;
    default:
        found = false;
    }

    if (found)
        return (CfAstTokenParsingResult) {
            .status = CF_AST_TOKEN_PARSING_STATUS_OK,
            .ok = {
                .rest = (CfAstSpan) { span.begin + 1, span.end },
                .token = (CfAstToken) {
                    .type = type,
                    .span = (CfAstSpan) { span.begin, span.begin + 1 },
                }
            },
        };

    // try to parse ident
    if (isalpha(*str.begin) || *str.begin == '_') {
        CfStr ident = { str.begin, str.begin };

        while (ident.end < str.end && isalnum(*ident.end) || *ident.end == '_')
            ident.end++;

        // try to separate keyword from ident
        CfAstTokenType ty = CF_AST_TOKEN_TYPE_VOID;

        CfAstToken token = cfAstTokenKeywordFromIdent(ident, &ty)
            ? (CfAstToken) { .type = ty, }
            : (CfAstToken) { .type = CF_AST_TOKEN_TYPE_IDENT, .ident = ident, };

        token.span = (CfAstSpan) { (size_t)(ident.begin - source.begin), (size_t)(ident.end - source.begin) };

        return (CfAstTokenParsingResult) {
            .status = CF_AST_TOKEN_PARSING_STATUS_OK,
            .ok = {
                .rest = (CfAstSpan) { (size_t)(ident.end - source.begin), span.end },
                .token = token,
            },
        };
    }

    // unknown character, actually
    return (CfAstTokenParsingResult) {
        .status = CF_AST_TOKEN_PARSING_STATUS_UNEXPECTED_SYMBOL,
        .unexpectedSymbol = *str.begin,
    };
} // cfAstTokenParse

// cf_ast_lexer.c
