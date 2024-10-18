#include <cf_vm.h>
#include <cf_stack.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

// TODO Split this fckng function into structure + set of little functions (?)

void cfModuleExec( const CfModule *module, const CfSandbox *sandbox ) {
    assert(module != NULL);
    assert(module->code != NULL);
    assert(sandbox != NULL);


#define OFFSET ((size_t)(instructionCounter - (const uint8_t *)module->code))

#define PANIC(...) { panicInfo = (CfPanicInfo){ __VA_ARGS__ }; goto cfModuleExec__handle_panic; }

#define PUSH(value)                                                    \
    if (CF_STACK_OK != cfStackPush(&stack, &(value))) {                \
        PANIC(                                                         \
            .reason = CF_PANIC_REASON_NO_OPERANDS,                     \
            .offset = OFFSET,                                          \
        );                                                             \
    }                                                                  \

#define POP(value)                                          \
    switch (cfStackPop(&stack, &(value))) {                 \
        case CF_STACK_OK:                                   \
            break;                                          \
        case CF_STACK_NO_VALUES:                            \
            PANIC(                                          \
                .reason = CF_PANIC_REASON_STACK_UNDERFLOW,  \
                .offset = OFFSET,                           \
            );                                              \
            break;                                          \
        default:                                            \
            PANIC(                                          \
                .reason = CF_PANIC_REASON_INTERNAL_ERROR,   \
                .offset = OFFSET,                           \
            );                                              \
    }                                                       \

#define READ(value)                                                     \
    {                                                                   \
        if (instructionCounter + sizeof(value) > instructionCounterEnd) \
            PANIC(                                                      \
                .reason = CF_PANIC_REASON_UNEXPECTED_CODE_END,          \
                .offset = OFFSET,                                       \
            );                                                          \
        memcpy(&value, instructionCounter, sizeof(value));              \
        instructionCounter += sizeof(value);                            \
    }                                                                   \

#define READ_REGISTER(reg)                                      \
        {                                                       \
            READ(reg);                                          \
            if (reg >= CF_REGISTER_COUNT) {                     \
                PANIC(                                          \
                    .reason = CF_PANIC_REASON_UNKNOWN_REGISTER, \
                    .offset = OFFSET,                           \
                    .unknownRegister = { .index = reg },        \
                )                                               \
            }                                                   \
        }                                                       \

    // this name inspired by x86v7 architecture
    const uint8_t *const instructionCounterBegin = (const uint8_t *)module->code;
    const uint8_t *const instructionCounterEnd = (const uint8_t *)module->code + module->codeLength;

    const uint8_t *instructionCounter = instructionCounterBegin;

    CfRegisters registers;
    CfPanicInfo panicInfo;

    CfStack stack = cfStackCtor(sizeof(uint32_t));
    CfStack callStack = cfStackCtor(sizeof(uint8_t *));

    if (stack == NULL || callStack == NULL)
        PANIC(
            .reason = CF_PANIC_REASON_INTERNAL_ERROR,
        );
    
    while (instructionCounter < instructionCounterEnd) {
        uint8_t opcode = *instructionCounter++;

        switch ((CfOpcode)opcode) {
        /***
         * Integer instructions
         ***/

        // TODO Standardize binary operations?
        case CF_OPCODE_ADD     : {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs += rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_SUB     : {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs -= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_SHL     : {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs <<= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_IMUL   : {
            int32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs *= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_MUL   : {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs *= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_IDIV   : {
            int32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs /= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_DIV   : {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs /= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_SHR: {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs >>= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_SAR: {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs >>= rhs;
            PUSH(lhs);

            break;
        }

        case CF_OPCODE_FTOI: {
            float f64;
            int32_t i64;

            POP(f64);
            i64 = f64;
            PUSH(i64);

            break;
        }

        /***
         * Floating-point instructions
         ***/
        case CF_OPCODE_FADD: {
            float lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs += rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_FSUB: {
            float lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs -= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_FMUL: {
            float lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs *= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_FDIV: {
            float lhs, rhs;

            POP(rhs);
            POP(lhs);
            lhs /= rhs;
            PUSH(lhs);
            break;
        }

        case CF_OPCODE_ITOF: {
            int32_t i64;
            float f64;

            POP(i64);
            f64 = i64;
            PUSH(f64);
            break;
        }

        case CF_OPCODE_PUSH    : {
            uint32_t val;
            READ(val);
            PUSH(val);
            break;
        }
        case CF_OPCODE_POP     : {
            uint32_t dst;
            POP(dst);
            break;
        }
        case CF_OPCODE_PUSH_R  : {
            uint8_t reg;
            READ_REGISTER(reg);
            PUSH(registers.indexed[reg]);
            break;
        }

        case CF_OPCODE_POP_R   : {
            uint8_t reg;
            uint32_t dst;
            READ_REGISTER(reg);
            POP(dst);

            // prevent FL and CZ register writing
            if (reg >= 2)
                registers.indexed[reg] = dst;
            break;
        }

        case CF_OPCODE_JL : {
            uint32_t point;
            READ(point);
            if (registers.fl.cmpIsLt)
                instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_JLE: {
            uint32_t point;
            READ(point);
            if (registers.fl.cmpIsLt || registers.fl.cmpIsEq)
                instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_JG: {
            uint32_t point;
            READ(point);
            if (!registers.fl.cmpIsLt && !registers.fl.cmpIsEq)
                instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_JGE: {
            uint32_t point;
            READ(point);
            if (!registers.fl.cmpIsLt)
                instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_JE: {
            uint32_t point;
            READ(point);
            if (registers.fl.cmpIsEq)
                instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_JNE: {
            uint32_t point;
            READ(point);

            if (!registers.fl.cmpIsEq)
                instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_JMP: {
            uint32_t point;
            READ(point);
            instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_CALL: {
            uint32_t point;
            READ(point);

            if (CF_STACK_OK != cfStackPush(&callStack, &instructionCounter))
                PANIC(.reason = CF_PANIC_REASON_INTERNAL_ERROR);
            instructionCounter = instructionCounterBegin + point;
            break;
        }

        case CF_OPCODE_RET: {
            uint8_t *newInstructionCounter = NULL;
            CfStackStatus status = cfStackPop(&callStack, &newInstructionCounter);

            // panic if not ok
            if (status != CF_STACK_OK) {
                CfPanicReason reason = CF_PANIC_REASON_INTERNAL_ERROR;
                if (status == CF_STACK_NO_VALUES)
                    reason = CF_PANIC_REASON_CALL_STACK_UNDERFLOW;
                PANIC(.reason = reason);
            }

            instructionCounter = newInstructionCounter;

            break;
        }

        case CF_OPCODE_CMP: {
            uint32_t lhs, rhs;

            POP(rhs);
            POP(lhs);

            registers.fl.cmpIsEq = (lhs == rhs);
            registers.fl.cmpIsLt = (lhs < rhs);
            break;
        }

        case CF_OPCODE_ICMP: {
            int32_t lhs, rhs;

            POP(rhs);
            POP(lhs);

            registers.fl.cmpIsEq = (lhs == rhs);
            registers.fl.cmpIsLt = (lhs < rhs);
            break;
        }

        case CF_OPCODE_FCMP: {
            float lhs, rhs;

            POP(rhs);
            POP(lhs);

            registers.fl.cmpIsEq = (lhs == rhs);
            registers.fl.cmpIsLt = (lhs < rhs);
            break;
        }

        case CF_OPCODE_SYSCALL     : {
            uint32_t index;
            READ(index);

            /// TODO: remove this sh*tcode then import tables will be added.
            switch (index) {
            // readFloat64
            case 0: {
                float value = 0.304780;
                if (sandbox->readFloat64 != NULL)
                    value = sandbox->readFloat64(sandbox->userContextPtr);
                PUSH(value);
                break;
            }

            // writeFloat64
            case 1: {
                float argument;
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

        case CF_OPCODE_HALT: {
            // halt program without additional checks
            instructionCounter = instructionCounterEnd + 1;
            break;
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

cfModuleExec__cleanup:

    cfStackDtor(stack);
    cfStackDtor(callStack);
    return;

cfModuleExec__handle_panic:

    if (sandbox->handlePanic != NULL)
        sandbox->handlePanic(sandbox->userContextPtr, &panicInfo);
    goto cfModuleExec__cleanup;

#undef PUSH
#undef POP
#undef OFFSET
#undef PANIC
#undef READ
#undef READ_REGISTER
} // cfModuleExec

// cf_vm.c
