#include "cf_asm.h"
#include "cf_stack.h"

#include <assert.h>
#include <string.h>

CfDisassemblyStatus cfDisassemble( const CfModule *module, char **dest, CfDisassemblyDetails *details ) {
    assert(module != NULL);
    assert(dest != NULL);

    CfStack outStack = cfStackCtor(sizeof(char));

    const uint8_t *bytecode = (const uint8_t *)module->code;
    const uint8_t *bytecodeEnd = bytecode + module->codeLength;

    char line[64] = {0};
    const size_t lineLengthMax = sizeof(line);

    while (bytecode + 2 <= bytecodeEnd) {
        uint16_t opcode = *bytecode;
        bytecode += 2;

        switch (opcode) {
        case CF_OPCODE_UNREACHABLE : {
            strcpy(line, "unreachable");
            break;
        }
        case CF_OPCODE_I64_ADD     : {
            strcpy(line, "i64_add");
            break;
        }
        case CF_OPCODE_I64_SUB     : {
            strcpy(line, "i64_sub");
            break;
        }
        case CF_OPCODE_I64_MUL_S   : {
            strcpy(line, "i64_mul_s");
            break;
        }
        case CF_OPCODE_I64_DIV_S   : {
            strcpy(line, "i64_div_s");
            break;
        }
        case CF_OPCODE_I64_MUL_U   : {
            strcpy(line, "i64_mul_u");
            break;
        }
        case CF_OPCODE_I64_DIV_U   : {
            strcpy(line, "i64_div_u");
            break;
        }
        case CF_OPCODE_R64_PUSH    : {
            strcpy(line, "r64_push    0x");
            // then read constant
            if (bytecodeEnd - bytecode < 8) {
                cfStackDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            uint64_t r64 = *(const uint64_t *)bytecode;
            bytecode += 8;

            for (size_t i = 0; i < 16; i++) {
                uint64_t hexDigit = (r64 >> ((15 - i) * 4)) & 0xF;
                char digit;

                if (hexDigit < 10) digit = '0' + hexDigit;
                else               digit = 'A' + hexDigit - 10;

                line[14 + i] = digit;
            }
            line[30] = '\0';

            break;
        }
        case CF_OPCODE_R64_POP     : {
            strcpy(line, "r64_pop");
            break;
        }
        case CF_OPCODE_SYSCALL     : {
            strcpy(line, "syscall");
            break;
        }

        default: {
            if (details != NULL)
                details->unknownOpcode.opcode = opcode;
            cfStackDtor(outStack);
            return CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE;
        }
        }

        const size_t currentSize = strlen(line);
        line[currentSize] = '\n';
        if (CF_STACK_OK != cfStackPushArrayReversed(&outStack, line, currentSize + 1)) {
            cfStackDtor(outStack);
            return CF_DISASSEMBLY_STATUS_INTERNAL_ERROR;
        }
    }

    // then append '0' and transform stack to array
    const char zero = '\0';
    if (CF_STACK_OK != cfStackPush(&outStack, &zero)) {
        cfStackDtor(outStack);
        return CF_DISASSEMBLY_STATUS_INTERNAL_ERROR;
    }

    if (!cfStackToArray(outStack, (void **)dest)) {
        cfStackDtor(outStack);
        return CF_DISASSEMBLY_STATUS_INTERNAL_ERROR;
    }

    cfStackDtor(outStack);
    return CF_DISASSEMBLY_STATUS_OK;
} // cfDisassemble

// cf_disassemble.cpp
