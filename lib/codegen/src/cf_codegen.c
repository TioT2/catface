/**
 * @brief code generator library main implementation file
 */

#include <setjmp.h>
#include <string.h>

#include <cf_deque.h>

#include "cf_codegen.h"


/// @brief code generator
typedef struct CfCodeGenerator {
    CfDeque * codeDeque;  ///< code destination
    CfDeque * linkDeque;  ///< link deque
    CfDeque * labelDeque; ///< label deque

    jmp_buf         finishBuffer;
    CfCodegenResult result;
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

CfCodegenResult cfCodegen( const CfTir *tir, CfObject *dst, CfArena *tempArena ) {
    const CfTirFunction *functionArray = cfTirGetFunctionArray(tir);
    size_t functionArrayLength = cfTirGetFunctionArrayLength(tir);

    return (CfCodegenResult) { CF_CODEGEN_STATUS_INTERNAL_ERROR };
} // cfCodegen

// cf_codegen.c
