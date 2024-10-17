#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "cf_asm.h"
#include "cf_stack.h"
#include "cf_string.h"

bool cfAsmNextLine(
    CfStringSlice *self,
    CfStringSlice *dst
) {
    CfStringSlice slice;

    do {
        if (self->begin >= self->end)
            return false;

        // find line end
        const char *lineEnd = self->begin;
        while (lineEnd < self->end && *lineEnd != '\n')
            lineEnd++;

        // find comment start
        const char *commentStart = self->begin;
        while (commentStart < lineEnd && *commentStart != ';')
            commentStart++;

        slice.begin = self->begin;
        slice.end = commentStart;

        // trim leading spaces
        while (slice.begin < slice.end && *slice.begin == ' ' || *slice.begin == '\t')
            slice.begin++;

        // trim trailing spaces
        while (slice.begin < slice.end && *slice.end == ' ' || *slice.end == '\t')
            slice.end--;

        self->begin = lineEnd + 1;
    } while (slice.end == slice.begin);

    *dst = slice;

    return true;
} // cfAsmNextLine

/// @brief fixup representation structure
typedef struct __CfAsmFixup {
    uint32_t *codePtr;     ///< pointer to code section to insert label to
    const char *labelName; ///< name of label to insert
} CfAsmFixup;

/**
 * @brief check if character can belong to ident
 */
static bool cfAsmIsIdentCharacter( const char ch ) {
    return
        (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        (ch == '_')
    ;
} // cfAsmIsIdentCharacter

typedef enum __CfAsmTokenType {
    CF_ASM_TOKEN_TYPE_IDENT,         ///< ident
    CF_ASM_TOKEN_TYPE_INTEGER,       ///< number
    CF_ASM_TOKEN_TYPE_COLON,         ///< colon
    CF_ASM_TOKEN_TYPE_FLOATING,      ///< floating
    CF_ASM_TOKEN_TYPE_LEFT_BRACKET,  ///< left bracket
    CF_ASM_TOKEN_TYPE_RIGHT_BRACKET, ///< right bracket
} CfAsmTokenType;

/// @brief token tagged union
typedef struct __CfAsmToken {
    CfAsmTokenType type; ///< type

    union {
        CfStringSlice ident; ///< ident slice
        uint64_t integer;    ///< integer number
        double floating;     ///< floating point number
    };
} CfAsmToken;

bool cfAsmNextToken( CfStringSlice *line, CfAsmToken *dst ) {
    if (line->begin < line->end && *line->begin == ';')
        return false;

    while (line->begin < line->end && *line->begin == ' ' || *line->begin == '\t')
        line->begin++;

    if (line->begin >= line->end)
        return false;
    char first = *line->begin;

    if (first == '[') {
        line->begin++;
        dst->type = CF_ASM_TOKEN_TYPE_LEFT_BRACKET;
        return true;
    }

    if (first == ']') {
        line->begin++;
        dst->type = CF_ASM_TOKEN_TYPE_RIGHT_BRACKET;
        return true;
    }

    if (first == ':') {
        line->begin++;
        dst->type = CF_ASM_TOKEN_TYPE_COLON;
        return true;
    }

    // parse number
    if (first >= '0' && first <= '9') {
        if (line->begin + 1 < line->end && line->begin[1] == 'x') {
            // parse hexadecimal
            uint64_t res;
            *line = cfSliceParseHexadecmialInteger(
                (CfStringSlice){line->begin + 2, line->end},
                &res
            );

            dst->type = CF_ASM_TOKEN_TYPE_INTEGER;
            dst->integer = res;
            return true;
        } else {
            // parse decimal (or floating)
            CfParsedDecimal res;

            *line = cfSliceParseDecimal(*line, &res);
            if (res.exponentStarted || res.fractionalStarted) {
                dst->type = CF_ASM_TOKEN_TYPE_FLOATING;
                dst->floating = (res.integer + res.fractional) * powl(10.0, res.exponent);
            } else {
                dst->type = CF_ASM_TOKEN_TYPE_INTEGER;
                dst->integer = res.integer;
            }

            return true;
        }
    }

    if (first >= 'a' && first <= 'z' || first >= 'A' && first <= 'Z' || first == '_') {
        CfStringSlice ident = {line->begin, line->begin};

        while (ident.end < line->end && cfAsmIsIdentCharacter(*ident.end))
            ident.end++;

        dst->floating = CF_ASM_TOKEN_TYPE_IDENT;
        dst->ident = ident;

        line->begin = ident.end;
        return true;
    }

    // unknown token
    return false;
} // cfAsmNextToken

/**
 * @brief register from string slice parsing function
 * 
 * @param[in] slice slice to parse register from
 * 
 * @return register index + 1 if success, 0 otherwise
 * (yes, it's quite strange solution, but in this case this ### is somehow reliable.)
 */
static uint32_t cfAsmParseRegister( CfStringSlice slice ) {
    if (slice.begin + 2 != slice.end)
        return 0;

    uint16_t bs = *(const uint16_t *)slice.begin;

    if (bs == *(const uint16_t *)"ax") return 1;
    if (bs == *(const uint16_t *)"bx") return 2;
    if (bs == *(const uint16_t *)"cx") return 3;
    if (bs == *(const uint16_t *)"dx") return 4;

    return 0;
} // cfAsmParseRegister

CfAssemblyStatus cfAssemble( CfStringSlice text, CfModule *dst, CfAssemblyDetails *details ) {
    assert(dst != NULL);

    CfStringSlice line;
    CfStack stack = CF_STACK_NULL;
    CfStack fixupStack = CF_STACK_NULL;
    CfAssemblyStatus resultStatus = CF_ASSEMBLY_STATUS_OK;

    stack = cfStackCtor(sizeof(uint8_t));
    fixupStack = cfStackCtor(sizeof(CfAsmFixup));
    resultStatus = CF_ASSEMBLY_STATUS_OK;

    if (stack == CF_STACK_NULL || fixupStack == CF_STACK_NULL) {
        resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
        goto cfAssemble__end;
    }

    // parse code
    while (cfAsmNextLine(&text, &line)) {
        uint8_t dataBuffer[16];
        size_t dataElementCount = 1;

        CfAsmToken token = {};
        CfStringSlice tokenSlice = line;

        // There's no starting token in line
        if (!cfAsmNextToken(&tokenSlice, &token))
            continue;

        if (token.type != CF_ASM_TOKEN_TYPE_IDENT) {
            if (details != NULL)
                details->unknownInstruction = line;
            resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
            goto cfAssemble__end;
        }

        // then try to parse opcode
               if (cfSliceStartsWith(token.ident, "add")) {
            dataBuffer[0] = CF_OPCODE_ADD;
        } else if (cfSliceStartsWith(token.ident, "sub")) {
            dataBuffer[0] = CF_OPCODE_SUB;
        } else if (cfSliceStartsWith(token.ident, "shl")) {
            dataBuffer[0] = CF_OPCODE_SHL;
        } else if (cfSliceStartsWith(token.ident, "imul")) {
            dataBuffer[0] = CF_OPCODE_IMUL;
        } else if (cfSliceStartsWith(token.ident, "mul")) {
            dataBuffer[0] = CF_OPCODE_MUL;
        } else if (cfSliceStartsWith(token.ident, "idiv")) {
            dataBuffer[0] = CF_OPCODE_IDIV;
        } else if (cfSliceStartsWith(token.ident, "div")) {
            dataBuffer[0] = CF_OPCODE_DIV;
        } else if (cfSliceStartsWith(token.ident, "shr")) {
            dataBuffer[0] = CF_OPCODE_SHR;
        } else if (cfSliceStartsWith(token.ident, "sar")) {
            dataBuffer[0] = CF_OPCODE_SAR;
        } else if (cfSliceStartsWith(token.ident, "ftoi")) {
            dataBuffer[0] = CF_OPCODE_FTOI;
        } else if (cfSliceStartsWith(token.ident, "fadd")) {
            dataBuffer[0] = CF_OPCODE_FADD;
        } else if (cfSliceStartsWith(token.ident, "fsub")) {
            dataBuffer[0] = CF_OPCODE_FSUB;
        } else if (cfSliceStartsWith(token.ident, "fmul")) {
            dataBuffer[0] = CF_OPCODE_FMUL;
        } else if (cfSliceStartsWith(token.ident, "fdiv")) {
            dataBuffer[0] = CF_OPCODE_FDIV;
        } else if (cfSliceStartsWith(token.ident, "itof")) {
            dataBuffer[0] = CF_OPCODE_ITOF;
        } else if (cfSliceStartsWith(token.ident, "push")) {
            if (
                !cfAsmNextToken(&tokenSlice, &token) ||
                (true
                    && token.type != CF_ASM_TOKEN_TYPE_INTEGER
                    && token.type != CF_ASM_TOKEN_TYPE_FLOATING
                    && token.type != CF_ASM_TOKEN_TYPE_IDENT
                )
            ) {
                if (details != NULL)
                    details->unknownInstruction = line;
                resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
                goto cfAssemble__end;
            }

            if (token.type == CF_ASM_TOKEN_TYPE_IDENT) {
                // use ident
                dataBuffer[0] = CF_OPCODE_PUSH_R;

                uint32_t reg = cfAsmParseRegister(token.ident);

                if (reg == 0) {
                    if (details != NULL)
                        details->unknownRegister = token.ident;
                    resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER;
                    goto cfAssemble__end;
                }

                dataBuffer[1] = reg - 1;
                dataElementCount = 2;
            } else {
                dataBuffer[0] = CF_OPCODE_PUSH;
                // parse some numeric token

                if (token.type == CF_ASM_TOKEN_TYPE_INTEGER)
                    *(uint32_t *)(dataBuffer + 1) = token.integer;
                else
                    *(float *)(dataBuffer + 1) = token.floating;
                dataElementCount = 5;
            }

        } else if (cfSliceStartsWith(token.ident, "pop")) {
            if (cfAsmNextToken(&tokenSlice, &token)) {
                if (token.type != CF_ASM_TOKEN_TYPE_IDENT) {
                    if (details != NULL)
                        details->unknownInstruction = line;
                    resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
                    goto cfAssemble__end;
                }

                dataBuffer[0] = CF_OPCODE_POP_R;

                uint32_t reg = cfAsmParseRegister(token.ident);

                if (reg == 0) {
                    if (details != NULL)
                        details->unknownRegister = line;
                    resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER;
                    goto cfAssemble__end;
                }

                dataBuffer[1] = reg - 1;
                dataElementCount = 2;
            } else {
                dataBuffer[0] = CF_OPCODE_POP;
            }

        } else
        if (cfSliceStartsWith(token.ident, "syscall")) {
            dataBuffer[0] = CF_OPCODE_SYSCALL;
            if (
                !cfAsmNextToken(&tokenSlice, &token) ||
                token.type != CF_ASM_TOKEN_TYPE_INTEGER
            ) {
                if (details != NULL)
                    details->unknownInstruction = line;
                resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
                goto cfAssemble__end;
            }

            *(uint32_t *)(dataBuffer + 1) = token.integer;
            dataElementCount = 5;
        } else if (cfSliceStartsWith(token.ident, "unreachable")) {
            dataBuffer[0] = CF_OPCODE_UNREACHABLE;
        } else {
            if (details != NULL) {
                details->unknownInstruction = line;
            }

            resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
            goto cfAssemble__end;
        }

        // append element to stack
        CfStackStatus status = cfStackPushArrayReversed(&stack, &dataBuffer, dataElementCount);
        if (status != CF_STACK_OK) {
            resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
            goto cfAssemble__end;
        }
    }

    {
        uint8_t *code = NULL;
        if (!cfStackToArray(stack, (void **)&code)) {
            resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
            goto cfAssemble__end;
        }
        dst->code = code;
        dst->codeLength = cfStackGetSize(stack) * sizeof(uint8_t);
    }

cfAssemble__end:
    cfStackDtor(stack);
    cfStackDtor(fixupStack);
    return resultStatus;
} // cfAssemble

// cf_asm.cpp
