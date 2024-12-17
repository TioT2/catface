/**
 * @brief code generator library main implementation file
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "cf_codegen_internal.h"

void cfCodeGeneratorFinish( CfCodeGenerator *const self, CfCodegenResult result ) {
    self->result = result;
    longjmp(self->finishBuffer, 1);
} // cfCodeGeneratorFinish

void cfCodeGeneratorAssert( CfCodeGenerator *const self, bool condition ) {
    if (!condition)
        cfCodeGeneratorFinish(self, (CfCodegenResult) { CF_CODEGEN_STATUS_INTERNAL_ERROR });
} // cfCodeGeneratorAssert

void * cfCodeGeneratorAllocTemp( CfCodeGenerator *const self, size_t size ) {
    void *data = cfArenaAlloc(self->tempArena, size);
    cfCodeGeneratorAssert(self, data != NULL);
    return data;
} // cfCodeGeneratorAllocTemp

void cfCodeGeneratorWriteCode( CfCodeGenerator *const self, const void *code, size_t size ) {
    cfCodeGeneratorAssert(self, cfDequePushArrayBack(self->codeDeque, code, size));
} // cfCodeGeneratorWriteCode

void cfCodeGeneratorCheckStrLength( CfCodeGenerator *const self, CfStr str ) {
    if (str.end - str.begin >= CF_LABEL_MAX - 1)
        cfCodeGeneratorFinish(self, (CfCodegenResult) {
            .status = CF_CODEGEN_STATUS_TOO_LONG_NAME,
            .tooLongName = str,
        });
} // cfCodeGeneratorCheckStrLength

void cfCodeGeneratorAddLink( CfCodeGenerator *const self, CfStr linkTo ) {
    CfLink link = { .codeOffset = (uint32_t)cfDequeLength(self->codeDeque) };

    // perform length check and write link str
    cfCodeGeneratorCheckStrLength(self, linkTo);
    memcpy(link.label, linkTo.begin, linkTo.end - linkTo.begin);

    // insert placeholder into code
    const uint32_t placeholder = ~0U;
    cfCodeGeneratorWriteCode(self, &placeholder, 4);

    // insert link
    cfCodeGeneratorAssert(self, cfDequePushBack(self->linkDeque, &link));
} // cfCodeGeneratorWriteCode

void cfCodeGeneratorAddLabel( CfCodeGenerator *const self, CfStr labelName ) {
    CfLabel label = {
        .value      = (uint32_t)cfDequeLength(self->codeDeque),
        .isRelative = true,
    };

    // perform length check
    cfCodeGeneratorCheckStrLength(self, labelName);
    memcpy(label.label, labelName.begin, labelName.end - labelName.begin);

    // insert label
    cfCodeGeneratorAssert(self, cfDequePushBack(self->labelDeque, &label));
} // cfCodeGeneratorAddLabel

void cfCodeGeneratorAddConstant( CfCodeGenerator *const self, CfStr name, uint32_t value ) {
    CfLabel label = {
        .value      = value,
        .isRelative = false,
    };

    // perform length check
    cfCodeGeneratorCheckStrLength(self, name);
    memcpy(label.label, name.begin, name.end - name.begin);

    // insert label
    cfCodeGeneratorAssert(self, cfDequePushBack(self->labelDeque, &label));
} // cfCodeGeneratorAddConstant

void cfCodeGeneratorBegin( CfCodeGenerator *const self ) {
    // write this code to 
    /*
        mgs
        pop ex
        mgs
        pop fx
        call main
        halt
     */

    // prelude code
    const uint8_t code[] = {
        CF_OPCODE_MGS,
        CF_OPCODE_POP,
        (CfPushPopInfo) {
            .registerIndex = CF_REGISTER_EX,
            .isMemoryAccess = false,
            .doReadImmediate = false,
        }.asByte,

        CF_OPCODE_MGS,
        CF_OPCODE_POP,
        (CfPushPopInfo) {
            .registerIndex = CF_REGISTER_FX,
            .isMemoryAccess = false,
            .doReadImmediate = false,
        }.asByte,
        CF_OPCODE_CALL,
    };
    cfCodeGeneratorWriteCode(self, &code, sizeof(code));
    // add link to main
    cfCodeGeneratorAddLink(self, CF_STR("main"));

    // write halt opcode
    uint8_t halt = CF_OPCODE_HALT;
    cfCodeGeneratorWriteCode(self, &halt, sizeof(halt));
} // code generator

void cfCodeGeneratorWritePushPop(
    CfCodeGenerator *const self,
    CfOpcode               opcode,
    CfPushPopInfo          info,
    int32_t                immediate
) {
    cfCodeGeneratorWriteCode(self, &opcode, 1);
    cfCodeGeneratorWriteCode(self, &info, 1);
    if (info.doReadImmediate)
        cfCodeGeneratorWriteCode(self, &immediate, 4);
} // cfCodeGeneratorWritePushPop

void cfCodeGeneratorWriteOpcode( CfCodeGenerator *const self, CfOpcode opcode ) {
    cfCodeGeneratorWriteCode(self, &opcode, 1);
} // cfCodeGeneratorWriteOpcode

void cfCodeGeneratorGenExpression( CfCodeGenerator *const self, const CfTirExpression *expression ) {
    switch (expression->type) {
    case CF_TIR_EXPRESSION_TYPE_CONST_I32: {
        if (expression->constI32 == 0)
            cfCodeGeneratorWritePushPop(self,
                CF_OPCODE_PUSH,
                (CfPushPopInfo) { CF_REGISTER_CZ },
                0
            );
        else
            cfCodeGeneratorWritePushPop(self,
                CF_OPCODE_PUSH,
                (CfPushPopInfo) {
                    .registerIndex   = CF_REGISTER_CZ,
                    .doReadImmediate = true
                },
                expression->constI32
            );
        break;
    }
    case CF_TIR_EXPRESSION_TYPE_CONST_F32: {
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_PUSH,
            (CfPushPopInfo) {
                .registerIndex   = CF_REGISTER_CZ,
                .doReadImmediate = true
            },
            *(const float *)&expression->constF32
        );
        break;
    }
    case CF_TIR_EXPRESSION_TYPE_CONST_U32: {
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_PUSH,
            (CfPushPopInfo) {
                .registerIndex   = CF_REGISTER_CZ,
                .doReadImmediate = true
            },
            *(const int32_t *)&expression->constU32
        );
        break;
    }
    case CF_TIR_EXPRESSION_TYPE_VOID: {
        // nice solution))) (no)
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_PUSH,
            (CfPushPopInfo) { CF_REGISTER_CZ },
            0
        );
        break;
    }
    case CF_TIR_EXPRESSION_TYPE_BINARY_OPERATOR: {
        // generate operands
        cfCodeGeneratorGenExpression(self, expression->binaryOperator.lhs);
        cfCodeGeneratorGenExpression(self, expression->binaryOperator.rhs);

        CfOpcode binaryOperatorOpcodes[][4] = {
            {CF_OPCODE_ADD , CF_OPCODE_ADD, CF_OPCODE_FADD}, // for add
            {CF_OPCODE_SUB , CF_OPCODE_SUB, CF_OPCODE_FSUB}, // for sub
            {CF_OPCODE_IMUL, CF_OPCODE_MUL, CF_OPCODE_FMUL}, // for mul
            {CF_OPCODE_IDIV, CF_OPCODE_DIV, CF_OPCODE_FDIV}, // for div
        };

        uint32_t firstIndex = ~0U;
        switch (expression->binaryOperator.op) {
        case CF_TIR_BINARY_OPERATOR_ADD: firstIndex = 0; break;
        case CF_TIR_BINARY_OPERATOR_SUB: firstIndex = 1; break;
        case CF_TIR_BINARY_OPERATOR_MUL: firstIndex = 2; break;
        case CF_TIR_BINARY_OPERATOR_DIV: firstIndex = 3; break;

        default:
            break;
        }

        uint32_t secondIndex = 0;
        switch (expression->resultingType) {
        case CF_TIR_TYPE_I32: secondIndex = 0; break;
        case CF_TIR_TYPE_U32: secondIndex = 1; break;
        case CF_TIR_TYPE_F32: secondIndex = 2; break;
        case CF_TIR_TYPE_VOID:
            // invalid TIR
            cfCodeGeneratorAssert(self, false);
            break;
        }

        if (firstIndex != ~0U) {
            // generate simple expression
            cfCodeGeneratorWriteOpcode(self, binaryOperatorOpcodes[firstIndex][secondIndex]);
            break;
        }

        // generate comparison opcode
        switch (expression->binaryOperator.lhs->resultingType) {
        case CF_TIR_TYPE_I32: cfCodeGeneratorWriteOpcode(self, CF_OPCODE_ICMP); break;
        case CF_TIR_TYPE_U32: cfCodeGeneratorWriteOpcode(self, CF_OPCODE_CMP ); break;
        case CF_TIR_TYPE_F32: cfCodeGeneratorWriteOpcode(self, CF_OPCODE_FCMP); break;

        case CF_TIR_TYPE_VOID:
            assert(false && "unreachable");
        }

        switch (expression->binaryOperator.op) {
        case CF_TIR_BINARY_OPERATOR_ADD:
        case CF_TIR_BINARY_OPERATOR_SUB:
        case CF_TIR_BINARY_OPERATOR_MUL:
        case CF_TIR_BINARY_OPERATOR_DIV:
            assert(false && "unreachable");

        // I DO NOT WANT TO DO THIS SH*T (TODO: fix command system)
        case CF_TIR_BINARY_OPERATOR_LT:
        case CF_TIR_BINARY_OPERATOR_GT:
        case CF_TIR_BINARY_OPERATOR_LE:
        case CF_TIR_BINARY_OPERATOR_GE:
        case CF_TIR_BINARY_OPERATOR_EQ:
        case CF_TIR_BINARY_OPERATOR_NE:
            assert(false && "unimplemented");
            break;
        }
        break;
    }
    case CF_TIR_EXPRESSION_TYPE_CALL: {
        // execute argument expressions in reverse order
        for (int32_t i = expression->call.inputArrayLength - 1; i >= 0; i--)
            cfCodeGeneratorGenExpression(self, expression->call.inputArray[i]);
        // call function, actually
        const CfTirFunction *function = cfTirGetFunctionById(self->tir, expression->call.functionId);
        cfCodeGeneratorAssert(self, function != NULL);

        // call function by name
        cfCodeGeneratorWriteOpcode(self, CF_OPCODE_CALL);
        cfCodeGeneratorAddLink(self, function->name);

        // push AX to stack
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_PUSH,
            (CfPushPopInfo) { CF_REGISTER_AX },
            0
        );
        break;
    }

    case CF_TIR_EXPRESSION_TYPE_LOCAL: {
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_PUSH,
            (CfPushPopInfo) {
                .registerIndex = CF_REGISTER_FX,
                .isMemoryAccess = true,
                .doReadImmediate = true,
            },
            -(int32_t)(expression->local + 1) * 4
        );
        break;
    }

    case CF_TIR_EXPRESSION_TYPE_GLOBAL: {
        assert(false && "Not implemented yet");
        break;
    }

    case CF_TIR_EXPRESSION_TYPE_ASSIGNMENT: {
        cfCodeGeneratorGenExpression(self, expression->assignment.value);
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_POP,
            (CfPushPopInfo) {
                .registerIndex = CF_REGISTER_FX,
                .isMemoryAccess = true,
                .doReadImmediate = true,
            },
            -(int32_t)(expression->local + 1) * 4
        );

        // void has value)))
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_PUSH,
            (CfPushPopInfo) { CF_REGISTER_CZ },
            0
        );

        break;
    }
    case CF_TIR_EXPRESSION_TYPE_CAST: {
        // cast to void does nothing
        if (expression->cast.type == CF_TIR_TYPE_VOID)
            break;
        // cast to self does nothing
        if (expression->cast.type == expression->cast.expression->resultingType)
            break;
        // cast between integer types isn't required
        if (expression->cast.type != CF_TIR_TYPE_F32 && expression->cast.expression->resultingType != CF_TIR_TYPE_F32)
            break;

        cfCodeGeneratorWriteOpcode(self, expression->cast.type == CF_TIR_TYPE_I32
            ? CF_OPCODE_ITOF
            : CF_OPCODE_FTOI
        );
        break;
    }
    }
} // cfCodeGeneratorGenExpression

void cfCodeGeneratorGenReturn( CfCodeGenerator *const self ) {
    // ex = fx
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_PUSH,
        (CfPushPopInfo) { CF_REGISTER_FX, },
        0
    );
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_POP,
        (CfPushPopInfo) { CF_REGISTER_EX },
        0
    );

    // restore fx
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_POP,
        (CfPushPopInfo) { CF_REGISTER_FX },
        0
    );

    // write return
    cfCodeGeneratorWriteOpcode(self, CF_OPCODE_RET);
} // cfCodeGeneratorGenReturn

void cfCodeGeneratorGenBlock( CfCodeGenerator *const self, const CfTirBlock *block ) {
    // ex += localCount * 4
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_PUSH,
        (CfPushPopInfo) {
            .registerIndex   = CF_REGISTER_EX,
            .isMemoryAccess  = false,
            .doReadImmediate = true,
        },
        (int32_t)block->localCount * 4
    );
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_POP,
        (CfPushPopInfo) { CF_REGISTER_EX },
        0
    );

    // generate block statements
    for (size_t i = 0; i < block->statementCount; i++)
        cfCodeGeneratorGenStatement(self, &block->statements[i]);

    // ex -= localCount * 4
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_PUSH,
        (CfPushPopInfo) {
            .registerIndex   = CF_REGISTER_EX,
            .isMemoryAccess  = false,
            .doReadImmediate = true,
        },
        -(int32_t)block->localCount * 4
    );
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_POP,
        (CfPushPopInfo) { CF_REGISTER_EX },
        0
    );
} // cfCodeGeneratorGenBlock

void cfCodeGeneratorGenStatement( CfCodeGenerator *const self, const CfTirStatement *statement ) {
    switch (statement->type) {
    case CF_TIR_STATEMENT_TYPE_EXPRESSION: {
        // generate expression (note, that ANY expression returns 32-bit value now)
        cfCodeGeneratorGenExpression(self, statement->expression);

        // drop expression result
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_POP,
            (CfPushPopInfo) { CF_REGISTER_CZ },
            0
        );
        break;
    }
    case CF_TIR_STATEMENT_TYPE_BLOCK: {
        cfCodeGeneratorGenBlock(self, statement->block);
        break;
    }

    case CF_TIR_STATEMENT_TYPE_RETURN: {
        // generate return expression
        cfCodeGeneratorGenExpression(self, statement->return_);

        // pop ax
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_POP,
            (CfPushPopInfo) { CF_REGISTER_AX },
            0
        );
        cfCodeGeneratorGenReturn(self);
        break;
    }

    case CF_TIR_STATEMENT_TYPE_IF: {
        /*
            [condition]
            push 0
            je __fnname__else_[index]

            [code then]

            jmp __fnname__if_end_[index]

            __fnname__else_[index]:

            [code else]

            __fnname__if_end_[index]:
        */

        uint32_t condIndex = self->conditionCounter++;

        char elseLabel[CF_LABEL_MAX] = {0};

        snprintf(elseLabel, sizeof(elseLabel), "__%*.s__else_%d",
            (int)cfStrLength(self->currentFunction),
            self->currentFunction.begin,
            condIndex
        );

        char ifEndLabel[CF_LABEL_MAX] = {0};

        snprintf(ifEndLabel, sizeof(ifEndLabel), "__%*.s__if_end_%d",
            (int)cfStrLength(self->currentFunction),
            self->currentFunction.begin,
            condIndex
        );

        cfCodeGeneratorGenExpression(self, statement->if_.condition);
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_PUSH,
            (CfPushPopInfo) { CF_REGISTER_CZ },
            0
        );
        cfCodeGeneratorWriteOpcode(self, CF_OPCODE_CMP);
        cfCodeGeneratorWriteOpcode(self, CF_OPCODE_JE);
        cfCodeGeneratorAddLink(self, CF_STR(elseLabel));

        cfCodeGeneratorGenBlock(self, statement->if_.blockThen);
        cfCodeGeneratorWriteOpcode(self, CF_OPCODE_JMP);
        cfCodeGeneratorAddLink(self, CF_STR(ifEndLabel));

        cfCodeGeneratorAddLabel(self, CF_STR(elseLabel));
        cfCodeGeneratorGenBlock(self, statement->if_.blockElse);
        cfCodeGeneratorAddLabel(self, CF_STR(ifEndLabel));
        break;
    }
    case CF_TIR_STATEMENT_TYPE_LOOP: {
        /*
            __fnname_loop_[index]:

            ; condition and code

            jmp __fnname_loop[index]
            __fnname__loop_end_[index]:
         */

        uint32_t loopIndex = self->loopCounter++;

        char loopLabel[CF_LABEL_MAX] = {0};

        snprintf(loopLabel, sizeof(loopLabel), "__%.*s__loop_%d",
            (int)cfStrLength(self->currentFunction),
            self->currentFunction.begin,
            loopIndex
        );

        char loopEndLabel[CF_LABEL_MAX] = {0};

        snprintf(loopEndLabel, sizeof(loopEndLabel), "__%.*s__loop_end_%d",
            (int)cfStrLength(self->currentFunction),
            self->currentFunction.begin,
            loopIndex
        );

        // generate loop start
        cfCodeGeneratorAddLabel(self, CF_STR(loopLabel));

        if (statement->loop.condition != NULL) {
            /*
                [condition]
                push 0
                cmp
                je __fnname__loop_end_[index]
            */

            cfCodeGeneratorGenExpression(self, statement->loop.condition);
            cfCodeGeneratorWritePushPop(self,
                CF_OPCODE_PUSH,
                (CfPushPopInfo) { CF_REGISTER_CZ },
                0
            );
            cfCodeGeneratorWriteOpcode(self, CF_OPCODE_CMP);
            cfCodeGeneratorWriteOpcode(self, CF_OPCODE_JE);
            cfCodeGeneratorAddLink(self, CF_STR(loopEndLabel));
        }

        // generate loop block
        cfCodeGeneratorGenBlock(self, statement->loop.block);

        // generate final jump
        cfCodeGeneratorWriteOpcode(self, CF_OPCODE_JMP);
        cfCodeGeneratorAddLink(self, CF_STR(loopLabel));

        // generate loop end label
        cfCodeGeneratorAddLabel(self, CF_STR(loopEndLabel));
        break;
    }
    }
} // cfCodeGeneratorGenStatement

void cfCodeGeneratorGenFunction( CfCodeGenerator *const self, const CfTirFunction *function ) {
    // only if function is implemented...
    if (function->impl == NULL)
        return;

    // enter 'function context'
    self->currentFunction = function->name;
    self->conditionCounter = 0;
    self->loopCounter = 0;

    // set function label
    cfCodeGeneratorAddLabel(self, function->name);

    // pop function arguments from operand stack to variable stack
    for (size_t i = 0; i < function->prototype.inputTypeArrayLength; i++)
        cfCodeGeneratorWritePushPop(self,
            CF_OPCODE_POP,
            (CfPushPopInfo) {
                .registerIndex   = CF_REGISTER_EX,
                .isMemoryAccess  = true,
                .doReadImmediate = true,
            },
            -(int32_t)(i + 1) * 4
        );

    // save old fx
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_PUSH,
        (CfPushPopInfo) { CF_REGISTER_FX },
        0
    );

    // fx = ex
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_PUSH,
        (CfPushPopInfo) { CF_REGISTER_EX },
        0
    );
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_POP,
        (CfPushPopInfo) { CF_REGISTER_FX },
        0
    );

    // ex += argCount * 4
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_PUSH,
        (CfPushPopInfo) {
            .registerIndex   = CF_REGISTER_EX,
            .isMemoryAccess  = false,
            .doReadImmediate = true,
        },
        (int32_t)function->prototype.inputTypeArrayLength * 4
    );
    cfCodeGeneratorWritePushPop(self,
        CF_OPCODE_POP,
        (CfPushPopInfo) { CF_REGISTER_EX },
        0
    );

    // codegenerate block
    cfCodeGeneratorGenBlock(self, function->impl);

    // generate 'forced' return
    cfCodeGeneratorGenReturn(self);
} // cfCodeGeneratorGenFunction

void cfCodeGeneratorEnd( CfCodeGenerator *const self, CfStr sourceName, CfObject *objectDst ) {
    assert(objectDst != NULL);

    // build object
    *objectDst = (CfObject) {
        .sourceName = cfStrOwnedCopy(sourceName),
        .codeLength = cfDequeLength(self->codeDeque),
        .code       = (uint8_t *)calloc(cfDequeLength(self->codeDeque), 1),
        .linkCount  = cfDequeLength(self->linkDeque),
        .links      = (CfLink *)calloc(cfDequeLength(self->linkDeque), sizeof(CfLink)),
        .labelCount = cfDequeLength(self->labelDeque),
        .labels     = (CfLabel *)calloc(cfDequeLength(self->labelDeque), sizeof(CfLabel)),
    };

    // check that all allocations are success
    cfCodeGeneratorAssert(self, true
        && objectDst->sourceName != NULL
        && objectDst->code       != NULL
        && objectDst->links      != NULL
        && objectDst->labels     != NULL
    );

    // write deque data
    cfDequeWrite(self->codeDeque , objectDst->code  );
    cfDequeWrite(self->linkDeque , objectDst->links );
    cfDequeWrite(self->labelDeque, objectDst->labels);
} // cfCodeGeneratorEnd

CfCodegenResult cfCodegen( const CfTir *tir, CfObject *dst, CfArena *tempArena ) {
    // zero destination memory
    memset(dst, 0, sizeof(CfObject));

    bool tempArenaOwned = (tempArena == NULL);

    if (tempArenaOwned) {
        tempArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED);
        if (tempArena == NULL)
            return (CfCodegenResult) { CF_CODEGEN_STATUS_INTERNAL_ERROR };
    }
    CfCodeGenerator generator = {
        .tempArena    = tempArena,
        .codeDeque    = cfDequeCtor(1, 512, tempArena),
        .linkDeque    = cfDequeCtor(sizeof(CfLink), CF_DEQUE_CHUNK_SIZE_UNDEFINED, tempArena),
        .labelDeque   = cfDequeCtor(sizeof(CfLabel), CF_DEQUE_CHUNK_SIZE_UNDEFINED, tempArena),
        .tir          = tir,

        .result       = (CfCodegenResult) { CF_CODEGEN_STATUS_INTERNAL_ERROR },
    };

    int isError = setjmp(generator.finishBuffer);

    if (false
        || isError
        || generator.codeDeque == NULL
        || generator.linkDeque == NULL
        || generator.labelDeque == NULL
    ) {
        cfObjectDtor(dst);
        return generator.result;
    }

    cfCodeGeneratorBegin(&generator);

    const CfTirFunction *functionArray = cfTirGetFunctionArray(tir);
    size_t functionArrayLength = cfTirGetFunctionArrayLength(tir);

    for (size_t i = 0; i < functionArrayLength; i++)
        cfCodeGeneratorGenFunction(&generator, &functionArray[i]);

    cfCodeGeneratorEnd(&generator, cfTirGetSourceName(tir), dst);

    return (CfCodegenResult) { CF_CODEGEN_STATUS_OK };
} // cfCodegen

// cf_codegen.c
