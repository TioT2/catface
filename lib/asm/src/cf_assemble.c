#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cf_asm.h"
#include "cf_stack.h"
#include "cf_string.h"

/**
 * @brief decimal number parsing function
 * 
 * @param[in] begin allowed parsing range begin
 * @param[in] end   allowed parsing range end
 * 
 * @return parsed number if input string starts from digit and 0 if not.
 */
static uint64_t cfAsmParseDecimal( const char *begin, const char *end ) {
    uint64_t result = 0;

    while (begin != end && *begin >= '0' && *begin <= '9') {
        result *= 10;
        result += *begin - '0';
        begin++;
    }

    return result;
} // cfAsmParseDecimal

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
    else
        result = cfAsmParseDecimal(begin, end);

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
        } else if (cfStrStartsWith(lineBegin, "i64_mul_s")) {
            dataBuffer[0] = CF_OPCODE_I64_MUL_S;
        } else if (cfStrStartsWith(lineBegin, "i64_mul_u")) {
            dataBuffer[0] = CF_OPCODE_I64_MUL_U;
        } else if (cfStrStartsWith(lineBegin, "i64_div_s")) {
            dataBuffer[0] = CF_OPCODE_I64_DIV_S;
        } else if (cfStrStartsWith(lineBegin, "i64_div_u")) {
            dataBuffer[0] = CF_OPCODE_I64_DIV_U;
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
