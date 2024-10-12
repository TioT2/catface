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

#define PANIC(...) { panicInfo = { __VA_ARGS__ }; goto cfModuleExec_HANDLE_PANIC; }

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
        case CF_OPCODE_I64_MUL_S   : {
            int64_t lhs, rhs;

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
        case CF_OPCODE_I64_MUL_U   : {
            uint64_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs *= rhs;
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
            // readInt64
            case 0: {
                uint64_t value = 0xDEDC0DE;
                if (sandbox->readInt64 != NULL)
                    value = sandbox->readInt64(sandbox->userContextPtr);
                PUSH(value);
                break;
            }

            // writeInt64
            case 1: {
                int64_t argument;
                POP(argument);
                if (sandbox->writeInt64 != NULL)
                    sandbox->writeInt64(sandbox->userContextPtr, argument);
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
