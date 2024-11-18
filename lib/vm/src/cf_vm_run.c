/**
 * @brief VM execution implementation file.
 */

#include <math.h>
#include <string.h>

#include "cf_vm_internal.h"

void cfVmRun( CfVm *const self ) {

// macro definition, because there's no way to abstract type
#define GENERIC_BINARY_OPERATION(ty, operation) \
    {                                           \
        ty lhs, rhs;                            \
        cfVmPopOperand(self, &rhs);             \
        cfVmPopOperand(self, &lhs);             \
        lhs = lhs operation rhs;                \
        cfVmPushOperand(self, &lhs);            \
        break;                                  \
    }

#define GENERIC_COMPARISON(ty)                     \
    {                                              \
        ty lhs = (ty)0, rhs = (ty)0;               \
        cfVmPopOperand(self, &rhs);                \
        cfVmPopOperand(self, &lhs);                \
        self->registers.fl.cmpIsEq = (lhs == rhs); \
        self->registers.fl.cmpIsLt = (lhs  < rhs); \
        break;                                     \
    }
#define GENERIC_UNARY_OPERATION(ty, name, op) \
    {                                         \
        ty name = (ty)0;                      \
        cfVmPopOperand(self, &name);          \
        name = op;                            \
        cfVmPushOperand(self, &name);         \
        break;                                \
    }

#define GENERIC_CONVERSION(src, dst) \
    {                                \
        union {                      \
            src s;                   \
            dst d;                   \
        } sd;                        \
        cfVmPopOperand(self, &sd);   \
        sd.d = (dst)sd.s;            \
        cfVmPushOperand(self, &sd);  \
        break;                       \
    }

    // start infinite execution loop
    for (;;) {
        uint8_t opcode = 0;

        cfVmRead(self, &opcode, 1);

        switch ((CfOpcode)opcode) {
        case CF_OPCODE_UNREACHABLE: {
            cfVmTerminate(self, CF_TERM_REASON_UNREACHABLE);
        }

        case CF_OPCODE_SYSCALL: {
            uint32_t index;
            cfVmRead(self, &index, sizeof(index));

            /// TODO: remove this sh*tcode then import tables will be added.
            switch (index) {
            // readFloat64
            case 0: {
                float value = 0.304780;
                value = self->sandbox->readFloat64(self->sandbox->userContext);
                cfVmPushOperand(self, &value);
                break;
            }

            // writeFloat64
            case 1: {
                float argument;
                cfVmPopOperand(self, &argument);
                self->sandbox->writeFloat64(self->sandbox->userContext, argument);
                break;
            }

            default: {
                self->termInfo.unknownSystemCall = index;
                cfVmTerminate(self, CF_TERM_REASON_UNKNOWN_SYSTEM_CALL);
            }
            }
            break;
        }

        case CF_OPCODE_HALT: {
            cfVmTerminate(self, CF_TERM_REASON_HALT);
            break;
        }

        // set of generic binary operations
        case CF_OPCODE_ADD : GENERIC_BINARY_OPERATION(uint32_t,  +)
        case CF_OPCODE_SUB : GENERIC_BINARY_OPERATION(uint32_t,  -)
        case CF_OPCODE_SHL : GENERIC_BINARY_OPERATION(uint32_t, <<)
        case CF_OPCODE_SHR : GENERIC_BINARY_OPERATION(uint32_t, >>)
        case CF_OPCODE_SAR : GENERIC_BINARY_OPERATION( int32_t, >>)
        case CF_OPCODE_AND : GENERIC_BINARY_OPERATION(uint32_t,  &)
        case CF_OPCODE_OR  : GENERIC_BINARY_OPERATION(uint32_t,  |)
        case CF_OPCODE_XOR : GENERIC_BINARY_OPERATION(uint32_t,  ^)
        case CF_OPCODE_IMUL: GENERIC_BINARY_OPERATION( int32_t,  *)
        case CF_OPCODE_MUL : GENERIC_BINARY_OPERATION(uint32_t,  *)
        case CF_OPCODE_IDIV: GENERIC_BINARY_OPERATION( int32_t,  /)
        case CF_OPCODE_DIV : GENERIC_BINARY_OPERATION(uint32_t,  /)
        case CF_OPCODE_FADD: GENERIC_BINARY_OPERATION(   float,  +)
        case CF_OPCODE_FSUB: GENERIC_BINARY_OPERATION(   float,  -)
        case CF_OPCODE_FMUL: GENERIC_BINARY_OPERATION(   float,  *)
        case CF_OPCODE_FDIV: GENERIC_BINARY_OPERATION(   float,  /)

        // set of generic comparisons
        case CF_OPCODE_CMP : GENERIC_COMPARISON(uint32_t)
        case CF_OPCODE_ICMP: GENERIC_COMPARISON( int32_t)
        case CF_OPCODE_FCMP: GENERIC_COMPARISON(   float)

        // set of generic conversions
        case CF_OPCODE_FTOI: GENERIC_CONVERSION(float, int32_t)
        case CF_OPCODE_ITOF: GENERIC_CONVERSION(int32_t, float)

        case CF_OPCODE_FSIN : GENERIC_UNARY_OPERATION(float, f, sinf(f));
        case CF_OPCODE_FCOS : GENERIC_UNARY_OPERATION(float, f, cosf(f));
        case CF_OPCODE_FNEG : GENERIC_UNARY_OPERATION(float, f, -f);
        case CF_OPCODE_FSQRT: GENERIC_UNARY_OPERATION(float, f, sqrtf(f));

        case CF_OPCODE_JMP: cfVmGenericConditionalJump(self,
            true
        ); break;

        case CF_OPCODE_JLE: cfVmGenericConditionalJump(self,
            self->registers.fl.cmpIsLt || self->registers.fl.cmpIsEq
        ); break;

        case CF_OPCODE_JL : cfVmGenericConditionalJump(self,
            self->registers.fl.cmpIsLt
        ); break;

        case CF_OPCODE_JGE: cfVmGenericConditionalJump(self,
            !self->registers.fl.cmpIsLt
        ); break;

        case CF_OPCODE_JG : cfVmGenericConditionalJump(self,
            !self->registers.fl.cmpIsLt && !self->registers.fl.cmpIsEq
        ); break;

        case CF_OPCODE_JE : cfVmGenericConditionalJump(self,
            self->registers.fl.cmpIsEq
        ); break;

        case CF_OPCODE_JNE: cfVmGenericConditionalJump(self,
            !self->registers.fl.cmpIsEq
        ); break;

        case CF_OPCODE_CALL: {
            uint32_t point;
            cfVmRead(self, &point, sizeof(point));
            cfVmPushIC(self);
            cfVmJump(self, point);
            break;
        }

        case CF_OPCODE_RET: {
            cfVmPopIC(self);
            break;
        }

        case CF_OPCODE_VSM: {
            // read videoMode bits from stack
            uint32_t newVideoMode;
            cfVmPopOperand(self, &newVideoMode);

            // validate video mode flag bit combination
            self->termInfo.invalidVideoMode.storageFormatBits = newVideoMode;
            self->termInfo.invalidVideoMode.updateModeBits = newVideoMode >> 1;

            // validate video mode
            switch ((CfVideoStorageFormat)(newVideoMode & 0x7)) {
            case CF_VIDEO_STORAGE_FORMAT_TEXT:
            case CF_VIDEO_STORAGE_FORMAT_COLORED_TEXT:
            case CF_VIDEO_STORAGE_FORMAT_COLOR_PALETTE:
            case CF_VIDEO_STORAGE_FORMAT_TRUE_COLOR:
                break;
            default:
                cfVmTerminate(self, CF_TERM_REASON_INVALID_VIDEO_MODE);
            }

            switch ((CfVideoUpdateMode)((newVideoMode >> 3) & 0x1)) {
            case CF_VIDEO_UPDATE_MODE_IMMEDIATE:
            case CF_VIDEO_UPDATE_MODE_MANUAL:
                break;
            default:
                cfVmTerminate(self, CF_TERM_REASON_INVALID_VIDEO_MODE);
            }

            // actually, set video mode
            cfVmSetVideoMode(
                self,
                (CfVideoStorageFormat)(newVideoMode & 0x7),
                (CfVideoUpdateMode)((newVideoMode >> 3) & 0x1)
            );
            break;
        }

        case CF_OPCODE_VRS: {
            // refresh screen
            if (!self->sandbox->refreshScreen(self->sandbox->userContext))
                cfVmTerminate(self, CF_TERM_REASON_SANDBOX_ERROR);
            break;
        }

        case CF_OPCODE_TIME: {
            float time;
            if (!self->sandbox->getExecutionTime(self->sandbox->userContext, &time))
                cfVmTerminate(self, CF_TERM_REASON_SANDBOX_ERROR);
            cfVmPushOperand(self, &time);
            break;
        }

        case CF_OPCODE_MEOW: {
            uint32_t meowCount;
            cfVmPopOperand(self, &meowCount);

            // this MEOW instruction implementation is deprecated.

            // for (uint32_t i = 0; i < meowCount; i++)
            //     printf("MEOW!\n");
            break;
        }

        case CF_OPCODE_MGS: {
            cfVmPushOperand(self, &self->ramSize);
            break;
        }

        case CF_OPCODE_IGKS: {
            uint32_t keyInt = CF_KEY_NULL;
            uint32_t stateInt = 0;

            cfVmPopOperand(self, &keyInt);

            const CfKey key = cfKeyFromUint32(keyInt);

            if (key != CF_KEY_NULL) {
                bool state = false;

                if (!self->sandbox->getKeyState(self->sandbox->userContext, key, &state))
                    cfVmTerminate(self, CF_TERM_REASON_SANDBOX_ERROR);

                stateInt = state;
            }

            cfVmPushOperand(self, &stateInt);
            break;
        }

        case CF_OPCODE_IWKD: {
            union {
                uint32_t integer; ///< integer part
                CfKey    key;     ///< key part
            } ik = { .integer = 0 };

            if (!self->sandbox->waitKeyDown(self->sandbox->userContext, &ik.key))
                cfVmTerminate(self, CF_TERM_REASON_SANDBOX_ERROR);
            cfVmPushOperand(self, &ik.integer);
            break;
        }

        case CF_OPCODE_PUSH: {
            CfPushPopInfo info;
            cfVmRead(self, &info, sizeof(info));

            uint32_t value = 0;
            if (info.doReadImmediate)
                cfVmRead(self, &value, sizeof(value));
            value += cfVmReadRegister(self, info.registerIndex);

            if (info.isMemoryAccess)
                memcpy(&value, cfVmGetMemoryPointer(self, value), 4);

            cfVmPushOperand(self, &value);
            break;
        }

        case CF_OPCODE_POP: {
            CfPushPopInfo info;
            cfVmRead(self, &info, sizeof(info));

            uint32_t value;
            cfVmPopOperand(self, &value);

            if (info.isMemoryAccess) {
                uint32_t imm = 0;
                if (info.doReadImmediate)
                    cfVmRead(self, &imm, sizeof(imm));
                uint32_t addr = cfVmReadRegister(self, info.registerIndex) + imm;

                void *ptr = cfVmGetMemoryPointer(self, addr);

                memcpy(ptr, &value, sizeof(value));
            } else {
                // throw error if trying to write to immediate
                if (info.doReadImmediate) {
                    self->termInfo.invalidPopInfo = info;
                    cfVmTerminate(self, CF_TERM_REASON_INVALID_POP_INFO);
                }

                // write to register
                cfVmWriteRegister(self, info.registerIndex, value);
            }
            break;
        }

        default: {
            self->termInfo.unknownOpcode = opcode;
            cfVmTerminate(self, CF_TERM_REASON_UNKNOWN_OPCODE);
        }
        }
    }

#undef GENERIC_JUMP
#undef GENERIC_COMPARISON
#undef GENERIC_BINARY_OPERATION
#undef GENERIC_CONVERSION
} // cfVmRun

// cf_vm_run.c
