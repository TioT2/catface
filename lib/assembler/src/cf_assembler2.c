/**
 * @brief new assembler implemetnation module
 */

#include <assert.h>
#include <setjmp.h>

#include "cf_assembler.h"

/// @brief assembler representation structure
typedef struct __CfAssembler {
    CfStr  textRest;                ///< rest of text to be processed
    CfStr  lineRest;                ///< rest of line to be processed
    size_t lineIndex;               ///< index of line

    CfAssemblyDetails details;      ///< details
    CfAssemblyStatus  status;       ///< assembling status (**must not** be accessed directly)
    jmp_buf           finishBuffer; ///< finishing buffer
} CfAssembler;

/// @brief token type (actually, token tag)
typedef enum __CfAssemblerTokenType {
    CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET,  ///< '['
    CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET, ///< ']'
    CF_ASSEMBLER_TOKEN_TYPE_PLUS,                 ///< '+'
    CF_ASSEMBLER_TOKEN_TYPE_COLON,                ///< ':'
    CF_ASSEMBLER_TOKEN_TYPE_IDENT,                ///< ident
    CF_ASSEMBLER_TOKEN_TYPE_FLOATING,             ///< floating point number
    CF_ASSEMBLER_TOKEN_TYPE_INTEGER,              ///< integer number
} CfAssemblerTokenType;

/// @brief assembler single token representation tagged union
typedef struct __CfAssemblerToken {
    CfAssemblerTokenType type; ///< token type

    union {
        CfStr   ident;    ///< ident
        double  floating; ///< floating point number
        int64_t integer;  ///< integer number
    };
} CfAssemblerToken;

/**
 * @brief execution finishing function
 * 
 * @param[in,out] self   assembler pointer
 * @param[in]     status assembling status
 * 
 * @note this function will not return.
 * 
 * @note this function sets 'status' and 'details.line' fields automatically,
 * so user should fill status-specific fields of detail structure only.
 */
void cfAssemblerFinish( CfAssembler *const self, const CfAssemblyStatus status ) {
    assert(self != NULL);

    self->status = status;
    self->details.line = self->lineIndex;

    longjmp(self->finishBuffer, true);
} // cfAssemblerFinish

/**
 * @brief next line parsing function
 * 
 * @param[in,out] self assembler pointer
 * 
 * @note throws ok if next line is not present
 * (it's assumed that all instructions are located at exactly one line)
 */
void cfAssemblerNextLine( CfAssembler *const self ) {
    CfStr slice;

    do {
        if (self->textRest.begin >= self->textRest.end)
            cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_OK);

        // find line end
        const char *lineEnd = self->textRest.begin;
        while (lineEnd < self->textRest.end && *lineEnd != '\n')
            lineEnd++;

        // find comment start
        const char *commentStart = self->textRest.begin;
        while (commentStart < lineEnd && *commentStart != ';')
            commentStart++;

        slice.begin = self->textRest.begin;
        slice.end = commentStart;

        // trim leading spaces
        while (slice.begin < slice.end && *slice.begin == ' ' || *slice.begin == '\t')
            slice.begin++;

        // trim trailing spaces
        while (slice.begin < slice.end && *slice.end == ' ' || *slice.end == '\t')
            slice.end--;

        self->textRest.begin = lineEnd + 1;
        self->lineIndex++;
    } while (slice.end == slice.begin);

    self->lineRest = slice;
} // cfAssemblerNextLine

/**
 * @brief check if character can belong to ident
 * 
 * @param[in] ch character to check
 * 
 * @return true if can, false otherwise
 */
static bool cfAssemblerIsIdentCharacter( const char ch ) {
    return
        (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        (ch == '_')
    ;
} // cfAssemblerIsIdentCharacter

/**
 * @brief next token from line parsing function
 * 
 * @param[in,out] self assembler reference
 * @param[out]    dst  parsing destination
 * 
 * @return true if parsed, false otherwise
 */
bool cfAssemblerNextToken( CfAssembler *const self, CfAssemblerToken *const dst ) {
    if (self->lineRest.begin >= self->lineRest.end || *self->lineRest.begin == ';')
        return false;

    const char first = *self->lineRest.begin;

    // handle single-char tokens
    {
        bool parsed = true;
        CfAssemblerTokenType tokenType;

        switch (first) {
        default:
        case '[': tokenType = CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET;  break;
        case ']': tokenType = CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET; break;
        case ':': tokenType = CF_ASSEMBLER_TOKEN_TYPE_COLON;                break;
        case '+': tokenType = CF_ASSEMBLER_TOKEN_TYPE_PLUS;                 break;
            parsed = false;
        }

        if (parsed) {
            self->lineRest.begin++;
            dst->type = tokenType;
            return true;
        }
    }

    // parse number
    if (first >= '0' && first <= '9') {
        if (self->lineRest.begin + 1 < self->lineRest.end && self->lineRest.begin[1] == 'x') {
            // parse hexadecimal
            uint64_t res;
            self->lineRest = cfStrParseHexadecmialInteger(
                (CfStr){self->lineRest.begin + 2, self->lineRest.end},
                &res
            );

            dst->type = CF_ASSEMBLER_TOKEN_TYPE_INTEGER;
            dst->integer = res;
        } else {
            // parse decimal
            CfParsedDecimal res;

            self->lineRest = cfStrParseDecimal(self->lineRest, &res);
            if (res.exponentStarted || res.fractionalStarted) {
                dst->type = CF_ASSEMBLER_TOKEN_TYPE_FLOATING;
                dst->floating = cfParsedDecimalCompose(&res);
            } else {
                dst->type = CF_ASSEMBLER_TOKEN_TYPE_INTEGER;
                dst->integer = res.integer;
            }
        }

        return true;
    }

    if (first >= 'a' && first <= 'z' || first >= 'A' && first <= 'Z' || first == '_') {
        CfStr ident = {self->lineRest.begin, self->lineRest.begin};

        while (ident.end < self->lineRest.end && cfAssemblerIsIdentCharacter(*ident.end))
            ident.end++;

        dst->type = CF_ASSEMBLER_TOKEN_TYPE_IDENT;
        dst->ident = ident;
        self->lineRest.begin = ident.end;

        return true;
    }

    // throw error if unknown token occured

    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_TOKEN);
    return false;
} // cfAssemblerNextToken

/**
 * @brief assembling starting function
 * 
 * @param[in,out] self assembler pointer
 */
void cfAssemblerRun( CfAssembler *const self ) {
    for (;;) {
        // parse next line
        cfAssemblerNextLine(self);

        // token parsing function
        CfAssemblerToken opcode;

        // unknown token
        if (!cfAssemblerNextToken(self, &opcode)) {
            continue;
        }
    }
} // cfAssemblerRun

CfAssemblyStatus cfAssemble2( CfStr text, CfStr sourceName, CfObject *dst, CfAssemblyDetails *details ) {
    CfAssembler assembler = {0};
    // setup finish buffer

    assembler.textRest = text;

    int jmp = setjmp(assembler.finishBuffer);
    if (jmp) {
        return assembler.status;
    }

    // start parsing
    cfAssemblerRun(&assembler);

    // panic?
    assert(false);
    return CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
} // cfAssemble2

// cf_assembler2.c
