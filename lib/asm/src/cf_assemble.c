#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cf_asm.h"
#include "cf_stack.h"
#include "cf_string.h"

/**
 * @brief R64 literal parsing function
 * 
 * @param[in] begin parsed string begin
 * @param[in] end   parsing string end
 * 
 * @note the strange combination of arugments is required because
 * function usage suggests that input string may be not null-terminated.
 * 
 * @todo create normal error handling and split this function in many more compact ones.
 * 
 * @return parsed literal. in case if string empty, returns 0...
 */
static uint64_t cfAsmParseR64( const char *begin, const char *end ) {
    assert(begin != NULL);
    assert(end != NULL);

    uint64_t result = 0;

    // at least now only decimal constants are supported.
    while (begin != end && (*begin == ' ' || *begin == '\t'))
        begin++;

    while (begin != end && *begin >= '0' && *begin <= '9') {
        result *= 10;
        result += *begin - '0';
        begin++;
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

            // read 64 bit constant
            uint64_t r64 = cfAsmParseR64(lineBegin + strlen("r64_push") + 1, lineEnd);

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
