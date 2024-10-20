#include "cf_disassembler.h"
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

/**
 * @brief push/pop info formatting function
 * 
 * @param[out] dst    formatting destination
 * @param[in]  dstLen destinatino buffer length
 * @param[in]  info   pushPop info
 * @param[in]  imm    immediate value (any value acceptable if info immediate reading flag is not set)
 */
static void cfAsmFormatPushPopInfo(
    char                *const dst,
    const size_t               dstLen,
    const CfPushPopInfo        info,
    const uint32_t             imm
) {
    if (info.isMemoryAccess) {
        if (info.doReadImmediate) {
            snprintf(dst, dstLen, "[%s + 0x%08X]", cfAsmGetRegisterName(info.registerIndex), imm);
            return;
        }
        snprintf(dst, dstLen, "[%s]", cfAsmGetRegisterName(info.registerIndex));
        return;
    }
    if (info.doReadImmediate) {
        snprintf(dst, dstLen, "%s + 0x%08X", cfAsmGetRegisterName(info.registerIndex), imm);
        return;
    }
    snprintf(dst, dstLen, "%s", cfAsmGetRegisterName(info.registerIndex));
} // cfAsmFormatPushPopInfo

CfDisassemblyStatus cfDisassemble( const CfExecutable *exec, char **dest, CfDisassemblyDetails *details ) {
    assert(exec != NULL);
    assert(dest != NULL);

    CfDarr outStack = cfDarrCtor(sizeof(char));

    const uint8_t *bytecode = (const uint8_t *)exec->code;
    const uint8_t *bytecodeEnd = bytecode + exec->codeLength;

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
        case CF_OPCODE_VSM: {
            strcpy(line, "vsm");
            break;
        }
        case CF_OPCODE_VRS: {
            strcpy(line, "vrs");
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
        case CF_OPCODE_JMP:
        case CF_OPCODE_CALL: {
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
            case CF_OPCODE_CALL: name = "call"; break;
            }

            snprintf(line, lineLengthMax, "%s 0x%08X", name, r32);
            break;
        }

        case CF_OPCODE_RET: {
            strcpy(line, "ret");
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

        case CF_OPCODE_POP:
        case CF_OPCODE_PUSH: {
            if (bytecodeEnd - bytecode < 1) {
                cfDarrDtor(outStack);
                return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
            }
            CfPushPopInfo info = *(const CfPushPopInfo *)bytecode;
            bytecode += sizeof(CfPushPopInfo);
            uint32_t imm = 0;

            if (info.doReadImmediate) {
                // read immediate, actually
                if (bytecodeEnd - bytecode < 4) {
                    cfDarrDtor(outStack);
                    return CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END;
                }
                imm = *(const uint32_t *)bytecode;
                bytecode += sizeof(uint32_t);
            }

            const char *name = opcode == CF_OPCODE_PUSH ? "push " : "pop  ";

            strncpy(line, name, lineLengthMax);
            cfAsmFormatPushPopInfo(
                line + strlen(name),
                lineLengthMax - strlen(name),
                info,
                imm
            );
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


const char * cfDisassemblyStatusStr( const CfDisassemblyStatus status ) {
    switch (status) {
    case CF_DISASSEMBLY_STATUS_OK                  : return "ok";
    case CF_DISASSEMBLY_STATUS_INTERNAL_ERROR      : return "internal error";
    case CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE      : return "unknown opcode";
    case CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END : return "unexpected code end";

    default                                        : return "<invalid>";
    }
} // cfDisassemblyStatusStr


void cfDisassemblyDetailsDump(
    FILE *const                  out,
    const CfDisassemblyStatus    status,
    const CfDisassemblyDetails * details
) {
    assert(out != NULL);
    assert(details != NULL);

    const char *str = cfDisassemblyStatusStr(status);

    if (status == CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE) {
        fprintf(out, "unknown opcode: 0x%4X", (int)details->unknownOpcode.opcode);
    } else {
        fprintf(out, "%s", str);
    }
} // cfDisassemblyDetailsDump

// cf_disassemble.c
