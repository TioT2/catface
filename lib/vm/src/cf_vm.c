#include <cf_vm.h>
#include <cf_stack.h>

#include <assert.h>

void cfModuleExec( const CfModule *module, const CfSandbox *sandbox ) {
    assert(module != NULL);
    assert(module->code != NULL);
    assert(sandbox != NULL);

#define PUSH(value)                                                    \
    if (CF_STACK_OK != cfStackPush(&stack, &(value))) {                \
        PANIC(                                                         \
            .reason = CF_PANIC_REASON_NO_OPERANDS,                     \
            .offset = OFFSET,                                          \
        );                                                             \
    }                                                                  \

#define POP(value)                                                     \
    if (CF_STACK_OK != cfStackPop(&stack, &(value))) {                 \
        PANIC(                                                         \
            .reason = CF_PANIC_REASON_INTERNAL_ERROR,                  \
            .offset = OFFSET,                                          \
        );                                                             \
    }                                                                  \

#define OFFSET ((size_t)(instructionCounter - (const uint8_t *)module->code))

#define PANIC(...) { panicInfo = (CfPanicInfo){ __VA_ARGS__ }; goto cfModuleExec_HANDLE_PANIC; }

    // this name inspired by x86v7 architecture
    const uint8_t *instructionCounter = (const uint8_t *)module->code;
    const uint8_t *const instructionCounterEnd = (const uint8_t *)module->code + module->codeLength;

    CfStack stack = cfStackCtor(sizeof(uint64_t));
    CfPanicInfo panicInfo;

    if (stack == CF_STACK_NULL)
        PANIC(
            .reason = CF_PANIC_REASON_INTERNAL_ERROR,
            .offset = 0,
        );

    while (instructionCounter + 2 <= instructionCounterEnd) {
        uint16_t opcode = *(const uint16_t *)instructionCounter;

        instructionCounter += 2;

        switch ((CfOpcode)opcode) {

        /***
         * Integer instructions
         ***/
        case CF_OPCODE_I64_ADD     : {
            uint64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs += rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_I64_SUB     : {
            uint64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs -= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_I64_SHL     : {
            uint64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs <<= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_I64_MUL_S   : {
            int64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs *= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_I64_MUL_U   : {
            uint64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs *= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_I64_DIV_S   : {
            int64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs /= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_I64_DIV_U   : {
            uint64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs /= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_I64_SHR_S: {
            int64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs >>= rhs;
            PUSH(lhs);
        }

        case CF_OPCODE_I64_SHR_U: {
            uint64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs >>= rhs;
            PUSH(lhs);
        }

        case CF_OPCODE_I64_FROM_F64_S: {
            double f64;
            int64_t i64;

            POP(f64);
            i64 = f64;
            PUSH(i64);
        }

        case CF_OPCODE_I64_FROM_F64_U: {
            double f64;
            uint64_t i64;

            POP(f64);
            i64 = f64;
            PUSH(i64);
        }

        /***
         * Floating-point instructions
         ***/
        case CF_OPCODE_F64_ADD: {
            double lhs, rhs;

            POP(lhs);
            POP(rhs);
            lhs += rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_F64_SUB: {
            double lhs, rhs;

            POP(lhs);
            POP(rhs);
            lhs -= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_F64_MUL: {
            double lhs, rhs;

            POP(lhs);
            POP(rhs);
            lhs *= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_F64_DIV: {
            double lhs, rhs;

            POP(lhs);
            POP(rhs);
            lhs /= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_F64_FROM_I64_S: {
            int64_t i64;
            double f64;

            POP(i64);
            f64 = i64;
            PUSH(f64);
            break;
        }

        case CF_OPCODE_F64_FROM_I64_U: {
            uint64_t i64;
            double f64;

            POP(i64);
            f64 = i64;
            PUSH(f64);
            break;
        }


        case CF_OPCODE_R64_PUSH    : {
            if (instructionCounter + 8 > instructionCounterEnd)
                PANIC(
                    .reason = CF_PANIC_REASON_UNEXPECTED_CODE_END,
                    .offset = OFFSET,
                );

            uint64_t val = *(const uint64_t *)instructionCounter;
            instructionCounter += 8;
            PUSH(val);
            break;
        }
        case CF_OPCODE_R64_POP     : {
            uint64_t dst;
            POP(dst);
            break;
        }
        case CF_OPCODE_SYSCALL     : {
            uint64_t index;
            POP(index);

            /// TODO: remove this sh*tcode then import tables will be added.
            switch (index) {
            // readFloat64
            case 0: {
                double value = 0.304780;
                if (sandbox->readFloat64 != NULL)
                    value = sandbox->readFloat64(sandbox->userContextPtr);
                PUSH(value);
                break;
            }

            // writeFloat64
            case 1: {
                double argument;
                POP(argument);
                if (sandbox->writeFloat64 != NULL)
                    sandbox->writeFloat64(sandbox->userContextPtr, argument);
                break;
            }

            default: {
                PANIC(
                    .reason = CF_PANIC_REASON_UNKNOWN_SYSTEM_CALL,
                    .offset = OFFSET,

                    .unknownSystemCall = {
                        .index = index,
                    }
                );
            }
            }
            break;
        }
        case CF_OPCODE_UNREACHABLE : {
            PANIC(
                .reason = CF_PANIC_REASON_UNREACHABLE,
                .offset = OFFSET,
            );
        }

        // panic on unknown instruction
        default: {
            PANIC(
                .reason = CF_PANIC_REASON_UNKNOWN_OPCODE,
                .offset = OFFSET,

                .unknownOpcode = {
                    .opcode = opcode,
                },
            );
        }
        }
    }


    cfStackDtor(stack);
    return;

cfModuleExec_HANDLE_PANIC:

    if (sandbox->handlePanic != NULL)
        sandbox->handlePanic(sandbox->userContextPtr, &panicInfo);
    cfStackDtor(stack);
    return;

#undef PUSH
#undef POP
#undef OFFSET
#undef PANIC
} // cfModuleExec
