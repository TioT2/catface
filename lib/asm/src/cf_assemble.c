#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "cf_asm.h"
#include "cf_stack.h"
#include "cf_string.h"

/**
 * @brief hexadecimal number parsing function
 * 
 * @param[in] begin allowed parsing range begin
 * @param[in] end   allowed parsing range end
 * 
 * @return parsed number if input string starts from hex digit and 0 if not.
 */
static uint64_t cfAsmParseHexadecmial( const char *begin, const char *end ) {
    uint64_t result = 0;

    while (begin != end) {
        const char ch = *begin++;
        uint64_t digit;

        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else if (ch >= 'A' && ch <= 'F') {
            digit = ch - 'A' + 10;
        } else if (ch >= 'a' && ch <= 'f') {
            digit = ch - 'a' + 10;
        } else {
            break;
        }

        result = result * 16 + digit;
    }

    return result;
} // cfAsmParseHexadecimal

typedef struct __CfAsmDecimal {
    uint64_t integer;          ///< integer part
    bool     fractionalIsSome; ///< fracitonal part parsing started
    double   fractional;       ///< fractional part
    bool     exponentIsSome;   ///< exponential part parsing started
    int64_t  exponent;         ///< exponential part
} CfAsmDecimal;

static CfAsmDecimal cfAsmParseDecimal( const char *begin, const char *end ) {
    CfAsmDecimal result = {0};

    // parse integer
    while (begin != end && *begin >= '0' && *begin <= '9') {
        result.integer *= 10;
        result.integer += *begin - '0';
        begin++;
    }

    // parse floating part
    if (begin != end && *begin == '.') {
        begin++;
        result.fractionalIsSome = true;

        double exp = 0.1;

        while (begin != end && *begin >= '0' && *begin <= '9') {
            result.fractional += exp * double(*begin - '0');
            exp *= 0.1;
            begin++;
        }
    }

    // parse exponential part
    if (begin != end && *begin == 'e') {
        int64_t sign = 1;
        result.exponentIsSome = true;

        if (begin != end) {
            if (*begin == '-')
                begin++, sign = -1;
            else if (*begin == '+')
                begin++;
        }

        while (begin != end && *begin >= '0' && *begin <= '9') {
            result.exponent *= 10;
            result.exponent += *begin - '0';
            begin++;
        }
        result.exponent *= sign;
    }

    return result;
} // cfAsmParseDecimal

/**
 * @brief R64 literal parsing function
 * 
 * @param[in] begin parsed string begin
 * @param[in] end   parsing string end
 * 
 * @note the strange combination of arugments is required because
 * function usage suggests that input string may be not null-terminated.
 * 
 * @return parsed literal. in case if string empty, returns 0...
 */
static uint64_t cfAsmParseR64( const char *begin, const char *end ) {
    assert(begin != NULL);
    assert(end != NULL);

    uint64_t result;

    if (cfStrStartsWith(begin, "0x"))
        result = cfAsmParseHexadecmial(begin + 2, end);
    else {
        CfAsmDecimal decimal = cfAsmParseDecimal(begin, end);

        if (decimal.fractionalIsSome || decimal.exponentIsSome)
            *(double *)&result = (decimal.integer + decimal.fractional) * powl(10.0, decimal.exponent);
        else
            result = decimal.integer;
    }

    return result;
} // cfAsmParseR64

CfAssemblyStatus cfAssemble( const char *text, size_t codeLen, CfModule *dst, CfAssemblyDetails *details ) {
    assert(text != NULL);
    assert(dst != NULL);

    const char *const textEnd = text + codeLen;

    const char *lineBegin = text;
    const char *lineEnd = text;
    CfStack stack = cfStackCtor(sizeof(uint16_t));

    while (lineBegin < textEnd) {
        lineEnd = lineBegin;
        while (lineEnd < textEnd && *lineEnd != '\n')
            lineEnd++;

        const char *commentStart = lineBegin;
        while (commentStart < lineEnd && *commentStart != ';')
            commentStart++;

        while (lineBegin < commentStart && (*lineBegin == '\t' || *lineBegin == ' '))
            lineBegin++;

        if (lineBegin == commentStart) {
            lineBegin = lineEnd + 1;
            continue;
        }

        // This solution is quite bad, because
        uint16_t dataBuffer[16];
        size_t dataElementCount = 1;

        // then try to parse expression
               if (cfStrStartsWith(lineBegin, "i64_add")) {
            dataBuffer[0] = CF_OPCODE_I64_ADD;
        } else if (cfStrStartsWith(lineBegin, "i64_sub")) {
            dataBuffer[0] = CF_OPCODE_I64_SUB;
        } else if (cfStrStartsWith(lineBegin, "i64_shl")) {
            dataBuffer[0] = CF_OPCODE_I64_SHL;
        } else if (cfStrStartsWith(lineBegin, "i64_mul_s")) {
            dataBuffer[0] = CF_OPCODE_I64_MUL_S;
        } else if (cfStrStartsWith(lineBegin, "i64_mul_u")) {
            dataBuffer[0] = CF_OPCODE_I64_MUL_U;
        } else if (cfStrStartsWith(lineBegin, "i64_div_s")) {
            dataBuffer[0] = CF_OPCODE_I64_DIV_S;
        } else if (cfStrStartsWith(lineBegin, "i64_div_u")) {
            dataBuffer[0] = CF_OPCODE_I64_DIV_U;
        } else if (cfStrStartsWith(lineBegin, "i64_shr_s")) {
            dataBuffer[0] = CF_OPCODE_I64_SHR_S;
        } else if (cfStrStartsWith(lineBegin, "i64_shr_u")) {
            dataBuffer[0] = CF_OPCODE_I64_SHR_U;
        } else if (cfStrStartsWith(lineBegin, "i64_from_f64_s")) {
            dataBuffer[0] = CF_OPCODE_I64_FROM_F64_S;
        } else if (cfStrStartsWith(lineBegin, "i64_from_f64_u")) {
            dataBuffer[0] = CF_OPCODE_I64_FROM_F64_U;
        } else if (cfStrStartsWith(lineBegin, "f64_add")) {
            dataBuffer[0] = CF_OPCODE_F64_ADD;
        } else if (cfStrStartsWith(lineBegin, "f64_sub")) {
            dataBuffer[0] = CF_OPCODE_F64_SUB;
        } else if (cfStrStartsWith(lineBegin, "f64_mul")) {
            dataBuffer[0] = CF_OPCODE_F64_MUL;
        } else if (cfStrStartsWith(lineBegin, "f64_div")) {
            dataBuffer[0] = CF_OPCODE_F64_DIV;
        } else if (cfStrStartsWith(lineBegin, "f64_from_i64_s")) {
            dataBuffer[0] = CF_OPCODE_F64_FROM_I64_S;
        } else if (cfStrStartsWith(lineBegin, "f64_from_i64_u")) {
            dataBuffer[0] = CF_OPCODE_F64_FROM_I64_U;
        } else if (cfStrStartsWith(lineBegin, "r64_push")) {
            // read r64 constant
            dataBuffer[0] = CF_OPCODE_R64_PUSH;

            // remove spaces from start
            const char *constBegin = lineBegin + strlen("r64_push") + 1;
            while (constBegin <= lineEnd && *constBegin == ' ' || *constBegin == '\t')
                constBegin++;

            // read 64-bit constant
            uint64_t r64 = cfAsmParseR64(constBegin, lineEnd);

            dataBuffer[1] = (r64 >>  0) & 0xFFFF;
            dataBuffer[2] = (r64 >> 16) & 0xFFFF;
            dataBuffer[3] = (r64 >> 32) & 0xFFFF;
            dataBuffer[4] = (r64 >> 48) & 0xFFFF;

            dataElementCount = 5;
        } else if (cfStrStartsWith(lineBegin, "r64_pop")) {
            dataBuffer[0] = CF_OPCODE_R64_POP;
        } else if (cfStrStartsWith(lineBegin, "syscall")) {
            dataBuffer[0] = CF_OPCODE_SYSCALL;
        } else if (cfStrStartsWith(lineBegin, "unreachable")) {
            dataBuffer[0] = CF_OPCODE_UNREACHABLE;
        } else {
            if (details != NULL) {
                details->unknownInstruction.lineBegin = lineBegin;
                details->unknownInstruction.lineEnd   = commentStart;
            }

            cfStackDtor(stack);
            return CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
        }

        // append element to stack
        CfStackStatus status = cfStackPushArrayReversed(&stack, &dataBuffer, dataElementCount);
        if (status != CF_STACK_OK) {
            cfStackDtor(stack);
            return CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
        }

        // do necessary calculations
        lineBegin = lineEnd + 1;
    }

    uint16_t *code = NULL;
    if (!cfStackToArray(stack, (void **)&code)) {
        cfStackDtor(stack);
        return CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
    }
    dst->code = code;
    dst->codeLength = cfStackGetSize(stack) * sizeof(uint16_t);

    cfStackDtor(stack);
    return CF_ASSEMBLY_STATUS_OK;
} // cfAssemble

// cf_asm.cpp
