/**
 * @brief VM internal misc functions implementation file
 */

#include <assert.h>
#include <string.h>

#include "cf_vm_internal.h"

/// @brief VM context representation structure
typedef struct __CfVm {
    uint8_t *         memory;                  ///< operative memory // TODO: I think «operative» is confusing in English, use RAM (Random Access Memory)
    size_t            memorySize;              ///< current memory size (1 MB, actually) // TODO: what, if it's 1MB then why is it variable...

    const CfExecutable  *executable;           ///< executed executable
    const CfSandbox * sandbox;                 ///< execution environment (sandbox, actually)
    //              ^ TODO: is it a consistent coding style choice? (Even in such case, it's rarely used)

    // registers
    CfRegisters       registers;               ///< user visible registers
    const uint8_t    *instructionCounter;      ///< next instruction to execute pointer // TODO: (instruction pointer IP)?
    // TODO: struct range?
    const uint8_t    *instructionCounterBegin; ///< pointer to first instruction
    const uint8_t    *instructionCounterEnd;   ///< pointer to first byte AFTER last instruciton

    // stascks // TODO: spelling?
    CfStack           operandStack;            ///< function operand stack
    CfStack           callStack;               ///< call stack (contains previous instructionCounter's)

    /// panic-related fields
    CfTermInfo        termInfo;                ///< info to give to user if panic occured
    jmp_buf           panicJumpBuffer;         ///< to panic handler jump buffer
} CfVm;

/**
 * @brief execution termination function
 *
 * @param[in,out] self   VM pointer
 * @param[in]     reason termination reason
 */
void cfVmTerminate( CfVm *self, const CfTermReason reason ) { // TODO: even for successful?
    assert(self != NULL);

    // fill standard termInfo fields
    self->termInfo.reason = reason;
    self->termInfo.offset = self->instructionCounter - (const uint8_t *)self->executable->code;

    // jump to termination handler
    longjmp(self->panicJumpBuffer, 1);
} // cfVmTerminate

void cfVmRead( CfVm *const self, void *const dst, const size_t count ) {
    if (self->instructionCounterEnd - self->instructionCounter < count)
        cfVmTerminate(self, CF_TERM_REASON_UNEXPECTED_CODE_END);
    memcpy(dst, self->instructionCounter, count);
    self->instructionCounter += count;
} // cfVmRead

void cfVmPushOperand( CfVm *const self, const void *const src ) {
    if (CF_DARR_OK != cfDarrPush(&self->operandStack, src))
        cfVmTerminate(self, CF_TERM_REASON_INTERNAL_ERROR);
} // cfVmPushOperand

/**
 * @brief value from operand stack popping function
 * 
 * @param[in,out] self virtual machine to perform action in
 * @param[out]    dst  popping destination destination (non-null, 4 bytes writable) // TODO: destination destination
 */
void cfVmPopOperand( CfVm *const self, void *const dst ) {
    switch (cfDarrPop(&self->operandStack, dst)) {
    case CF_DARR_INTERNAL_ERROR : cfVmTerminate(self, CF_TERM_REASON_INTERNAL_ERROR);
    case CF_DARR_NO_VALUES      : cfVmTerminate(self, CF_TERM_REASON_NO_OPERANDS);
    case CF_DARR_OK             : break;
    }
} // cfVmPopOperand

void cfVmJump( CfVm *const self, const uint32_t point ) {
    // TODO: assert(self)?
    self->instructionCounter = self->instructionCounterBegin + point;

    // perform bound check
    if (self->instructionCounter >= self->instructionCounterEnd) // TODO: assert-like thing?
        cfVmTerminate(self, CF_TERM_REASON_INVALID_IC);
} // cfVmJump

void cfVmGenericConditionalJump( CfVm *const self, const bool condition ) {
    uint32_t point;
    cfVmRead(self, &point, sizeof(point));
    if (condition)
        cfVmJump(self, point);
} // cfVmGenericConditionalJump

void cfVmPushIC( CfVm *const self ) {
    if (CF_DARR_OK != cfDarrPush(&self->callStack, &self->instructionCounter))
        cfVmTerminate(self, CF_TERM_REASON_INTERNAL_ERROR);
} // cfVmPushCall

void cfVmPopIC( CfVm *const self ) {
    switch (cfDarrPop(&self->callStack, &self->instructionCounter)) {
    case CF_DARR_INTERNAL_ERROR : cfVmTerminate(self, CF_TERM_REASON_INTERNAL_ERROR);
    case CF_DARR_NO_VALUES      : cfVmTerminate(self, CF_TERM_REASON_CALL_STACK_UNDERFLOW);
    case CF_DARR_OK             : break;
    }
} // cfVmPopIC

void cfVmWriteRegister( CfVm *const self, const uint32_t reg, const uint32_t value ) {
    if (reg >= CF_REGISTER_COUNT) {
        self->termInfo.unknownRegister = reg;
        cfVmTerminate(self, CF_TERM_REASON_UNKNOWN_REGISTER);
    }

    if (reg >= 2)
        self->registers.indexed[reg] = value;
} // cfVmWriteRegister

void cfVmSetVideoMode(
    CfVm *const self,
    const CfVideoStorageFormat storageFormat,
    const CfVideoUpdateMode updateMode
) {
    // set corresponding registers
    self->registers.fl.videoStorageFormat = storageFormat;
    self->registers.fl.videoUpdateMode = updateMode;

    // propagate changes to sandbox
    if (!self->sandbox->setVideoMode(self->sandbox->userContext, storageFormat, updateMode))
        cfVmTerminate(self, CF_TERM_REASON_SANDBOX_ERROR);
} // cfVmSetVideoMode

uint32_t cfVmReadRegister( CfVm *const self, const uint32_t reg ) {
    if (reg >= CF_REGISTER_COUNT) {
        self->termInfo.unknownRegister = reg;
        cfVmTerminate(self, CF_TERM_REASON_UNKNOWN_REGISTER);
    }
    return self->registers.indexed[reg];
} // cfVmReadRegister

void * cfVmGetMemoryPointer( CfVm *const self, const uint32_t addr ) {
    // perform address bound check
    if (addr >= self->ramSize - 4) {
        self->termInfo.segmentationFault.memorySize = self->ramSize;
        self->termInfo.segmentationFault.addr = addr;
        cfVmTerminate(self, CF_TERM_REASON_SEGMENTATION_FAULT);
    }

    return self->ram + addr;
} // cfVmGetMemoryPointer

// cf_vm.c
