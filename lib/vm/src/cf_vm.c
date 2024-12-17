/**
 * @brief VM internal misc functions implementation file
 */

#include <assert.h>
#include <string.h>

#include "cf_vm_internal.h"

void cfVmTerminate( CfVm *self, const CfTermReason reason ) {
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

void cfVmPopOperand( CfVm *const self, void *const dst ) {
    switch (cfDarrPop(&self->operandStack, dst)) {
    case CF_DARR_INTERNAL_ERROR : cfVmTerminate(self, CF_TERM_REASON_INTERNAL_ERROR);
    case CF_DARR_NO_VALUES      : cfVmTerminate(self, CF_TERM_REASON_NO_OPERANDS);
    case CF_DARR_OK             : break;
    }
} // cfVmPopOperand

void cfVmJump( CfVm *const self, const uint32_t point ) {
    self->instructionCounter = self->instructionCounterBegin + point;

    // perform instructionCuonter bound check
    if (self->instructionCounter >= self->instructionCounterEnd)
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
    if (addr + 4 > self->ramSize) {
        self->termInfo.segmentationFault.memorySize = self->ramSize;
        self->termInfo.segmentationFault.addr = addr;
        cfVmTerminate(self, CF_TERM_REASON_SEGMENTATION_FAULT);
    }

    return self->ram + addr;
} // cfVmGetMemoryPointer

// cf_vm.c
