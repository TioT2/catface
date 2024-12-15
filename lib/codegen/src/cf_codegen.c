/**
 * @brief code generator library main implementation file
 */

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <cf_deque.h>
#include <cf_executable.h>

#include "cf_codegen.h"


/// @brief code generator
typedef struct CfCodeGenerator {
    CfArena         * tempArena;    ///< temporary arena
    CfDeque         * codeDeque;    ///< code destination
    CfDeque         * linkDeque;    ///< link deque
    CfDeque         * labelDeque;   ///< label deque
    jmp_buf           finishBuffer; ///< finishing buffer
    CfCodegenResult   result;       ///< result
} CfCodeGenerator;

/**
 * @brief immediately terminate code generation process
 * 
 * @param[in] self   codegenerator pointer
 * @param[in] result codegeneration result
 */
void cfCodeGeneratorFinish( CfCodeGenerator *const self, CfCodegenResult result ) {
    self->result = result;
    longjmp(self->finishBuffer, 1);
} // cfCodeGeneratorFinish

/**
 * @brief simple assertion
 * 
 * @param[in] self      code generator pointer
 * @param[in] condition condition
 * 
 * @note function finishes code generation with INTERNAL_ERROR if condition is false.
 */
void cfCodeGeneratorAssert( CfCodeGenerator *const self, bool condition ) {
    if (!condition)
        cfCodeGeneratorFinish(self, (CfCodegenResult) { CF_CODEGEN_STATUS_INTERNAL_ERROR });
} // cfCodeGeneratorAssert

/**
 * @brief to resulting code deque writing function
 * 
 * @param[in] self
 */
void cfCodeGeneratorWriteCode( CfCodeGenerator *const self, const void *code, size_t size ) {
    cfCodeGeneratorAssert(self, cfDequePushArrayBack(self->codeDeque, code, size));
} // cfCodeGeneratorWriteCode

/**
 * @brief check str length (if str if longer, than CF_LABEL_MAX - 1, finishes with TOO_LONG_NAME error)
 * 
 * @param[in] self codegenerator pointer
 * @param[in] str  str to check length of
 */
void cfCodeGeneratorCheckStrLength( CfCodeGenerator *const self, CfStr str ) {
    if (str.end - str.begin >= CF_LABEL_MAX - 1)
        cfCodeGeneratorFinish(self, (CfCodegenResult) {
            .status = CF_CODEGEN_STATUS_TOO_LONG_NAME,
            .tooLongName = str,
        });
} // cfCodeGeneratorCheckStrLength

/**
 * @brief add link to certain label declared in code right here
 * 
 * @param[in] self   codegenerator pointer
 * @param[in] linkTo label to add link to
 */
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

/**
 * @brief add label to current code point
 * 
 * @param[in] self      code generator pointer
 * @param[in] labelName label itself
 */
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

/**
 * @brief add label to current code point
 * 
 * @param[in] self  code generator pointer
 * @param[in] name  name of constant to add
 * @param[in] value constant value
 * 
 * @note this funciton adds new non-relative label
 */
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

/**
 * @brief code generator start function
 * 
 * @param[in] self code generator
 * 
 * @note writes prelude to codegenerator
 */
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
            .doReadImmediate = false,
            .isMemoryAccess = false,
        }.asByte,

        CF_OPCODE_MGS,
        CF_OPCODE_POP,
        (CfPushPopInfo) {
            .registerIndex = CF_REGISTER_FX,
            .doReadImmediate = false,
            .isMemoryAccess = false,
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

/**
 * @brief codegen a function
 * 
 * @param[in] self     codeGenerator
 * @param[in] function TIR function pointer
 */
void cfCodeGeneratorAddFunction( CfCodeGenerator *const self, const CfTirFunction *function ) {
    // set function label
    cfCodeGeneratorAddLabel(self, function->name);

    // argument storage?
    assert(false && "Not implemented yet");

    // add ret opcode
    uint8_t ret = CF_OPCODE_RET;
    cfCodeGeneratorWriteCode(self, &ret, 1);
} // cfCodeGeneratorAddFunction

/**
 * @brief code generation finishing function
 * 
 * @param[in] self       code generator
 * @param[in] sourceName source file name
 * @param[in] objectDst  object destination (non-null)
 */
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
    cfCodeGeneratorAssert(self, false
        || objectDst->sourceName == NULL
        || objectDst->code       == NULL
        || objectDst->links      == NULL
        || objectDst->labelCount == NULL
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

        .result = (CfCodegenResult) { CF_CODEGEN_STATUS_INTERNAL_ERROR },
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
        cfCodeGeneratorAddFunction(&generator, &functionArray[i]);

    cfCodeGeneratorEnd(&generator, cfTirGetSourceName(tir), dst);

    return (CfCodegenResult) { CF_CODEGEN_STATUS_OK };
} // cfCodegen

// cf_codegen.c
