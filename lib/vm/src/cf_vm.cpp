#include <cf_vm.h>
#include <cf_stack.h>

#include <assert.h>

void cfModuleExec( const CfModule *module, const CfSandbox *sandbox ) {
    assert(module != NULL);
    assert(module->code != NULL);
    assert(sandbox != NULL);

    // construct 64-bit stack
    CfStack stack = cfStackCtor(sizeof(uint64_t));

    if (stack == CF_STACK_NULL) {
        if (sandbox->handlePanic != NULL) {
            CfPanicInfo panicInfo = {
                .reason = CF_PANIC_REASON_INTERNAL_ERROR,
                .offset = 0,
            };
            sandbox->handlePanic(sandbox->userContextPtr, &panicInfo);
        }
        return;
    }

    // what are you
    const uint8_t *instructionCounter = (const uint8_t *)module->code;
    const uint8_t *const instructionCounterEnd = (const uint8_t *)module->code + module->codeLength;

    while (instructionCounter + 2 < instructionCounterEnd) {
        uint16_t opcode = *(const uint16_t *)instructionCounter;

        instructionCounter += 2;

        switch ((CfOpcode)opcode) {
        case CF_OPCODE_I64_ADD     : break;
        case CF_OPCODE_I64_SUB     : break;
        case CF_OPCODE_I64_MUL_S   : break;
        case CF_OPCODE_I64_DIV_S   : break;
        case CF_OPCODE_I64_MUL_U   : break;
        case CF_OPCODE_I64_DIV_U   : break;
        case CF_OPCODE_R64_PUSH    : break;
        case CF_OPCODE_R64_POP     : break;
        case CF_OPCODE_SYSCALL     : break;
        case CF_OPCODE_UNREACHABLE : {
            // panic on unreachable
            if (sandbox->handlePanic != NULL) {
                CfPanicInfo panicInfo = {
                    .reason = CF_PANIC_REASON_UNREACHABLE,
                    .offset = (size_t)(instructionCounter - (const uint8_t *)module->code),
                };

                sandbox->handlePanic(sandbox->userContextPtr, &panicInfo);
            }

            cfStackDtor(stack);
            return;
        }

        // panic on unknown instruction
        default: {
            if (sandbox->handlePanic != NULL) {
                CfPanicInfo panicInfo = {
                    .reason = CF_PANIC_REASON_UNKNOWN_OPCODE,
                    .offset = (size_t)(instructionCounter - (const uint8_t *)module->code),

                    // fill reason-specific data
                    .unknownOpcode = {
                        .opcode = opcode,
                    }
                };

                sandbox->handlePanic(sandbox->userContextPtr, &panicInfo);
            }

            cfStackDtor(stack);

            return;
        }
        }
    }

    cfStackDtor(stack);
} // cfModuleExec
