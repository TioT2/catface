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
 * @param[in]  begin allowed parsing range begin
 * @param[in]  end   allowed parsing range end
 * @param[out] dst   parsing destination (nullable)
 * 
 * @return pointer to character after last parsed.
 */
static const char * cfAsmParseHexadecmialInteger( const char *begin, const char *end, uint64_t *dst ) {
    uint64_t result = 0;

    while (begin != end) {
        const char ch = *begin;
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

        begin++;
        result = result * 16 + digit;
    }

    if (dst != NULL)
        *dst = result;

    return begin;
} // cfAsmParseHexadecimalInteger

/**
 * @brief hexadecimal number parsing function
 * 
 * @param[in]  begin allowed parsing range begin
 * @param[in]  end   allowed parsing range end
 * @param[out] dst   parsing destination (nullable)
 * 
 * @return pointer to character after last parsed.
 */
static const char * cfAsmParseDecimalInteger( const char *begin, const char *end, uint64_t *dst ) {
    uint64_t result = 0;

    while (begin != end) {
        const char ch = *begin;
        uint64_t digit;

        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else {
            break;
        }

        result = result * 10 + digit;
        begin++;
    }

    if (dst != NULL)
        *dst = result;

    return begin;
} // cfAsmParseDecimalInteger

typedef struct __CfAsmDecimal {
    uint64_t integer;          ///< integer part
    bool     fractionalIsSome; ///< fracitonal part parsing started
    double   fractional;       ///< fractional part
    bool     exponentIsSome;   ///< exponential part parsing started
    int64_t  exponent;         ///< exponential part
} CfAsmDecimal;

static const char * cfAsmParseDecimal( const char *begin, const char *end, CfAsmDecimal *dst ) {
    CfAsmDecimal result = {0};
    const char *newBegin;

    newBegin = cfAsmParseDecimalInteger(begin, end, &result.integer);
    begin = newBegin;

    // parse floating part
    if (begin != end && *begin == '.') {
        begin++;
        result.fractionalIsSome = true;

        uint64_t frac;
        // parse fractional
        newBegin = cfAsmParseDecimalInteger(begin, end, &frac);
        result.fractional = frac * powl(10.0, -double(newBegin - begin));
        begin = newBegin;
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

        uint64_t unsignedExponent;
        newBegin = cfAsmParseDecimalInteger(begin, end, &unsignedExponent);
        result.exponent = int64_t(unsignedExponent) * sign;
        begin = newBegin;
    }

    if (dst != NULL)
        *dst = result;

    return begin;
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
static uint32_t cfAsmParseR32( const char *begin, const char *end ) {
    assert(begin != NULL);
    assert(end != NULL);

    uint32_t result;

    // remove heading spaces
    while (begin < end && (*begin == ' ' || *begin == '\t'))
        begin++;

    if (cfStrStartsWith(begin, "0x")) {
        uint64_t result64;
        cfAsmParseHexadecmialInteger(begin + 2, end, &result64);
        result = result64;
    } else {
        CfAsmDecimal decimal;
        cfAsmParseDecimal(begin, end, &decimal);

        if (decimal.exponentIsSome || decimal.fractionalIsSome)
            *(float *)&result = (decimal.integer + decimal.fractional) * powl(10.0, decimal.exponent);
        else
            result = decimal.integer;
    }

    return result;
} // cfAsmParseR32

static uint32_t cfAsmParseInteger( const char *begin, const char *end ) {
    uint64_t result;

    // remove heading spaces
    while (begin < end && (*begin == ' ' || *begin == '\t'))
        begin++;

    if (cfStrStartsWith(begin, "0x"))
        cfAsmParseHexadecmialInteger(begin, end, &result);
    else
        cfAsmParseDecimalInteger(begin, end, &result);
    return result;
} // cfAsmParseInteger

typedef struct __CfAsmLineIterator {
    const char *textCurr;
    const char *textEnd;
} CfAsmLineIterator;

bool cfAsmLineIteratorNext(
    CfAsmLineIterator *self,
    const char **dstBegin,
    const char **dstEnd
) {
    const char *begin;
    const char *end;

    do {
        if (self->textCurr >= self->textEnd)
            return false;

        // find line end
        const char *lineEnd = self->textCurr;
        while (lineEnd < self->textEnd && *lineEnd != '\n')
            lineEnd++;

        // find comment start
        const char *commentStart = self->textCurr;
        while (commentStart < lineEnd && *commentStart != ';')
            commentStart++;

        begin = self->textCurr;
        end = commentStart;

        // trim leading spaces
        while (begin < end && *begin == ' ' || *begin == '\t')
            begin++;

        // trim trailing spaces
        while (begin < end && *end == ' ' || *end == '\t')
            end--;

        self->textCurr = lineEnd + 1;
    } while (end == begin);

    *dstBegin = begin;
    *dstEnd   = end;

    return true;
}

CfAssemblyStatus cfAssemble( const char *text, size_t textLen, CfModule *dst, CfAssemblyDetails *details ) {
    assert(text != NULL);
    assert(dst != NULL);

    CfAsmLineIterator lineIter = {
        .textCurr = text,
        .textEnd = text + textLen,
    };
    const char *lineBegin = NULL;
    const char *lineEnd = NULL;

    uint16_t frameSize = 0;

    // parse declarations
    bool codeLineAlreadyGot = false;
    while (cfAsmLineIteratorNext(&lineIter, &lineBegin, &lineEnd)) {
        // then some line isn't valid declaration
        if (*lineBegin != '.') {
            codeLineAlreadyGot = true;
            break;
        }

        // parse declaration
        if (cfStrStartsWith(lineBegin + 1, "frame_size")) {
            // skip spaces
            const char *intBegin = lineBegin + 11;
            while (intBegin < lineEnd && (*intBegin == ' ' || *intBegin == '\t'))
                intBegin++;

            // parse size
            frameSize = cfAsmParseInteger(intBegin, lineEnd);
        } else {
            // unknown declaration occured
            if (details != NULL) {
                details->unknownDeclaration.lineBegin = lineBegin;
                details->unknownDeclaration.lineEnd = lineEnd;
            }
            return CF_ASSEMBLY_STATUS_UNKNOWN_DECLARATION;
        }
    }

    CfStack stack = cfStackCtor(sizeof(uint8_t));

    if (stack == CF_STACK_NULL)
        return CF_ASSEMBLY_STATUS_INTERNAL_ERROR;

    // parse code
    while (codeLineAlreadyGot || cfAsmLineIteratorNext(&lineIter, &lineBegin, &lineEnd)) {
        codeLineAlreadyGot = false;

        uint8_t dataBuffer[16];
        size_t dataElementCount = 1;

        // then try to parse expression
               if (cfStrStartsWith(lineBegin, "add")) {
            dataBuffer[0] = CF_OPCODE_ADD;
        } else if (cfStrStartsWith(lineBegin, "sub")) {
            dataBuffer[0] = CF_OPCODE_SUB;
        } else if (cfStrStartsWith(lineBegin, "shl")) {
            dataBuffer[0] = CF_OPCODE_SHL;
        } else if (cfStrStartsWith(lineBegin, "imul")) {
            dataBuffer[0] = CF_OPCODE_IMUL;
        } else if (cfStrStartsWith(lineBegin, "mul")) {
            dataBuffer[0] = CF_OPCODE_MUL;
        } else if (cfStrStartsWith(lineBegin, "idiv")) {
            dataBuffer[0] = CF_OPCODE_IDIV;
        } else if (cfStrStartsWith(lineBegin, "div")) {
            dataBuffer[0] = CF_OPCODE_DIV;
        } else if (cfStrStartsWith(lineBegin, "shr")) {
            dataBuffer[0] = CF_OPCODE_SHR;
        } else if (cfStrStartsWith(lineBegin, "sar")) {
            dataBuffer[0] = CF_OPCODE_SAR;
        } else if (cfStrStartsWith(lineBegin, "ftoi")) {
            dataBuffer[0] = CF_OPCODE_FTOI;
        } else if (cfStrStartsWith(lineBegin, "fadd")) {
            dataBuffer[0] = CF_OPCODE_FADD;
        } else if (cfStrStartsWith(lineBegin, "fsub")) {
            dataBuffer[0] = CF_OPCODE_FSUB;
        } else if (cfStrStartsWith(lineBegin, "fmul")) {
            dataBuffer[0] = CF_OPCODE_FMUL;
        } else if (cfStrStartsWith(lineBegin, "fdiv")) {
            dataBuffer[0] = CF_OPCODE_FDIV;
        } else if (cfStrStartsWith(lineBegin, "itof")) {
            dataBuffer[0] = CF_OPCODE_ITOF;
        } else if (cfStrStartsWith(lineBegin, "push")) {
            // try to parse register
            lineBegin += strlen("push");
            while (lineBegin < lineEnd && *lineBegin == ' ' || *lineBegin == '\t')
                lineBegin++;
            if (lineBegin + 1 < lineEnd && *lineBegin >= 'a' && *lineBegin <= 'd' && lineBegin[1] == 'x') {
                dataBuffer[0] = CF_OPCODE_PUSH_R;
                dataBuffer[1] = *lineBegin - 'a';
                dataElementCount = 2;
            } else {
                dataBuffer[0] = CF_OPCODE_PUSH;
                *(uint32_t *)(dataBuffer + 1) = cfAsmParseR32(lineBegin, lineEnd);
                dataElementCount = 5;
            }

        } else if (cfStrStartsWith(lineBegin, "pop")) {

            // try to parse register
            lineBegin += strlen("pop");
            while (lineBegin < lineEnd && *lineBegin == ' ' || *lineBegin == '\t')
                lineBegin++;

            if (lineBegin + 1 < lineEnd && *lineBegin >= 'a' && *lineBegin <= 'd' && lineBegin[1] == 'x') {
                dataBuffer[0] = CF_OPCODE_POP_R;
                dataBuffer[1] = *lineBegin - 'a';
                dataElementCount = 2;
            } else {
                dataBuffer[0] = CF_OPCODE_POP;
            }
        } else if (cfStrStartsWith(lineBegin, "syscall")) {
            dataBuffer[0] = CF_OPCODE_SYSCALL;
            *(uint32_t *)(dataBuffer + 1) = cfAsmParseR32(lineBegin + strlen("syscall"), lineEnd);
            dataElementCount = 5;
        } else if (cfStrStartsWith(lineBegin, "unreachable")) {
            dataBuffer[0] = CF_OPCODE_UNREACHABLE;
        } else {
            if (details != NULL) {
                details->unknownInstruction.lineBegin = lineBegin;
                details->unknownInstruction.lineEnd   = lineEnd;
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
    }

    uint8_t *code = NULL;
    if (!cfStackToArray(stack, (void **)&code)) {
        cfStackDtor(stack);
        return CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
    }
    dst->code = code;
    dst->codeLength = cfStackGetSize(stack) * sizeof(uint8_t);
    dst->frameSize = frameSize;

    cfStackDtor(stack);
    return CF_ASSEMBLY_STATUS_OK;
} // cfAssemble

// cf_asm.cpp
