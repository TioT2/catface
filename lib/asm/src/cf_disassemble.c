#include "cf_asm.h"
#include "cf_darr.h"

#include <assert.h>
#include <string.h>

/**
 * @brief register by index getting function
 * 
 * @param[in] reg register index (< CF_REGISTER_COUNT)
 * 
 * @return register name
 */
static const char * cfAsmGetRegisterName( uint8_t reg ) {
    assert(reg < CF_REGISTER_COUNT);

    switch (reg) {
    case 0: return "cz";
    case 1: return "fl";
    case 2: return "ax";
    case 3: return "bx";
    case 4: return "cx";
    case 5: return "dx";
    case 6: return "ex";
    case 7: return "fx";
    }

    return "";
} // cfAsmGetRegisterName

CfDisassemblyStatus cfDisassemble( const CfModule *module, char **dest, CfDisassemblyDetails *details ) {
    assert(module != NULL);
    assert(dest != NULL);

    CfDarr outStack = cfDarrCtor(sizeof(char));

    const uint8_t *bytecode = (const uint8_t *)module->code;
    const uint8_t *bytecodeEnd = bytecode + module->codeLength;

    char line[256] = {0};
    const size_t lineLengthMax = sizeof(line);

    while (bytecode < bytecodeEnd) {
        uint8_t opcode = *bytecode++;

        switch (opcode) {
        case CF_OPCODE_UNREACHABLE    : {
            strcpy(line, "unreachable");
            break;
        }
        case CF_OPCODE_SYSCALL        : {
            if (bytecodeEnd - bytecode < 4) {
                cfDarrDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }

            sprintf(line, "syscall %d", *(const uint32_t *)bytecode);
            bytecode += 4;
            break;
        }

        case CF_OPCODE_HALT: {
            strcpy(line, "halt");
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

        case CF_OPCODE_JL:
        case CF_OPCODE_JLE:
        case CF_OPCODE_JG:
        case CF_OPCODE_JGE:
        case CF_OPCODE_JE:
        case CF_OPCODE_JNE:
        case CF_OPCODE_JMP: {
            if (bytecodeEnd - bytecode < 4) {
                cfDarrDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }

            uint32_t r32 = *(const uint32_t *)bytecode;
            bytecode += 4;

            const char *name = "??? ";
            switch (opcode) {
            case CF_OPCODE_JL  : name = "jl  "; break;
            case CF_OPCODE_JLE : name = "jle "; break;
            case CF_OPCODE_JG  : name = "jg  "; break;
            case CF_OPCODE_JGE : name = "jge "; break;
            case CF_OPCODE_JE  : name = "je  "; break;
            case CF_OPCODE_JNE : name = "jne "; break;
            case CF_OPCODE_JMP : name = "jmp "; break;
            }

            snprintf(line, sizeof(line), "%s 0x%08X", name, r32);
            break;
        }

        case CF_OPCODE_CMP: {
            strcpy(line, "cmp");
            break;
        }

        case CF_OPCODE_ICMP: {
            strcpy(line, "icmp");
            break;
        }

        case CF_OPCODE_FCMP: {
            strcpy(line, "fcmp");
            break;
        }

        case CF_OPCODE_PUSH: {
            // then read constant
            if (bytecodeEnd - bytecode < 4) {
                cfDarrDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            uint32_t r32 = *(const uint32_t *)bytecode;
            bytecode += 4;

            snprintf(line, sizeof(line), "push 0x%08X", r32);
            break;
        }

        case CF_OPCODE_POP: {
            strcpy(line, "pop");
            break;
        }

        case CF_OPCODE_PUSH_R: {
            if (bytecodeEnd - bytecode < 1) {
                cfDarrDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            uint8_t reg = *bytecode++;
            sprintf(line, "push %s", cfAsmGetRegisterName(reg));
            break;
        }

        case CF_OPCODE_POP_R: {
            if (bytecodeEnd - bytecode < 1) {
                cfDarrDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            uint8_t reg = *bytecode++;
            sprintf(line, "pop  %s", cfAsmGetRegisterName(reg));
            break;
        }

        default: {
            if (details != NULL)
                details->unknownOpcode.opcode = opcode;
            cfDarrDtor(outStack);
            return CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE;
        }
        }

        const size_t currentSize = strlen(line);
        line[currentSize] = '\n';
        if (CF_DARR_OK != cfDarrPushArray(&outStack, line, currentSize + 1)) {
            cfDarrDtor(outStack);
            return CF_DISASSEMBLY_STATUS_INTERNAL_ERROR;
        }
    }

    // then append '0' and transform stack to array
    const char zero = '\0';
    if (CF_DARR_OK != cfDarrPush(&outStack, &zero)) {
        cfDarrDtor(outStack);
        return CF_DISASSEMBLY_STATUS_INTERNAL_ERROR;
    }

    if (CF_DARR_OK != cfDarrIntoData(outStack, (void **)dest)) {
        cfDarrDtor(outStack);
        return CF_DISASSEMBLY_STATUS_INTERNAL_ERROR;
    }

    cfDarrDtor(outStack);
    return CF_DISASSEMBLY_STATUS_OK;
} // cfDisassemble

// cf_disassemble.cpp
