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

    char line[256] = {0};
    const size_t lineLengthMax = sizeof(line);

    // display framesize
    if (module->frameSize != 0) {
        size_t len = sprintf(line, ".frame_size     0x%08X\n", module->frameSize);
        line[len] = '\n';

        if (cfStackPushArrayReversed(&outStack, line, len + 1) != CF_STACK_OK) {
            cfStackDtor(outStack);
            return CF_DISASSEMBLY_STATUS_INTERNAL_ERROR;
        }
    }

    while (bytecode < bytecodeEnd) {
        uint8_t opcode = *bytecode++;

        switch (opcode) {
        case CF_OPCODE_UNREACHABLE    : {
            strcpy(line, "unreachable");
            break;
        }
        case CF_OPCODE_SYSCALL        : {
            if (bytecodeEnd - bytecode < 4) {
                cfStackDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }

            sprintf(line, "syscall %d", *(const uint32_t *)bytecode);
            bytecode += 4;
            break;
        }
        case CF_OPCODE_ADD        : {
            strcpy(line, "add");
            break;
        }
        case CF_OPCODE_SUB        : {
            strcpy(line, "sub");
            break;
        }
        case CF_OPCODE_SHL        : {
            strcpy(line, "shl");
            break;
        }
        case CF_OPCODE_IMUL      : {
            strcpy(line, "imul");
            break;
        }
        case CF_OPCODE_MUL      : {
            strcpy(line, "mul");
            break;
        }
        case CF_OPCODE_IDIV      : {
            strcpy(line, "idiv");
            break;
        }
        case CF_OPCODE_DIV      : {
            strcpy(line, "div");
            break;
        }
        case CF_OPCODE_SHR      : {
            strcpy(line, "shr");
            break;
        }
        case CF_OPCODE_SAR      : {
            strcpy(line, "sar");
            break;
        }
        case CF_OPCODE_FTOI: {
            strcpy(line, "ftoi");
            break;
        }
        case CF_OPCODE_FADD: {
            strcpy(line, "fadd");
            break;
        }
        case CF_OPCODE_FSUB: {
            strcpy(line, "fsub");
            break;
        }
        case CF_OPCODE_FMUL: {
            strcpy(line, "fmul");
            break;
        }
        case CF_OPCODE_FDIV: {
            strcpy(line, "fdiv");
            break;
        }
        case CF_OPCODE_ITOF: {
            strcpy(line, "itof");
            break;
        }
        case CF_OPCODE_PUSH: {
            // then read constant
            if (bytecodeEnd - bytecode < 4) {
                cfStackDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            uint64_t r32 = *(const uint32_t *)bytecode;
            bytecode += 4;

            snprintf(line, sizeof(line), "push 0x%08lX", r32);
            break;
        }

        case CF_OPCODE_POP: {
            strcpy(line, "pop");
            break;
        }

        case CF_OPCODE_PUSH_R: {
            if (bytecodeEnd - bytecode < 1) {
                cfStackDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            uint8_t reg = *bytecode++;
            sprintf(line, "push %cx", 'a' + reg);
            break;
        }

        case CF_OPCODE_POP_R: {
            if (bytecodeEnd - bytecode < 1) {
                cfStackDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            uint8_t reg = *bytecode++;
            sprintf(line, "pop  %cx", 'a' + reg);
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
