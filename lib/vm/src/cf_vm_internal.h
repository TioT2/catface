/**
 * @brief CF virtual machine internal declaration file
 */

#ifndef CF_VM_INTERNAL_H_
#define CF_VM_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cf_darr.h>
#include <setjmp.h>

#include "cf_vm.h"

/// @brief VM context representation structure
typedef struct __CfVm {
    uint8_t *         memory;                  ///< operative memory
    size_t            memorySize;              ///< current memory size (1 MB, actually)

    const CfExecutable * executable;           ///< executed executable
    const CfSandbox * sandbox;                 ///< execution environment (sandbox, actually)

    // registers
    CfRegisters       registers;               ///< user visible register
    const uint8_t   * instructionCounter;      ///< next instruction to execute pointer
    const uint8_t   * instructionCounterBegin; ///< pointer to first instruction
    const uint8_t   * instructionCounterEnd;   ///< pointer to first byte AFTER last instruciton

    // stascks
    CfDarr           operandStack;             ///< function operand stack
    CfDarr           callStack;                ///< call stack (contains previous instructionCounter's)

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
void cfVmTerminate( CfVm *self, const CfTermReason reason );

/**
 * @brief value by instruction counter popping function
 * 
 * @param[in,out] self  virtual machine to perform operation in
 * @param[out]    dst   operation destination
 * @param[in]     count count of bytes to read from instruction stream
 */
void cfVmRead( CfVm *const self, void *const dst, const size_t count );

/**
 * @brief value to operand stack pushing function
 * 
 * @param[in,out] self virtual machine to perform action in
 * @param[out]    src  operand source (non-null, 4 bytes readable)
 */
void cfVmPushOperand( CfVm *const self, const void *const src );

/**
 * @brief value from operand stack popping function
 * 
 * @param[in,out] self virtual machine to perform action in
 * @param[out]    dst  popping destination destination (non-null, 4 bytes writable)
 */
void cfVmPopOperand( CfVm *const self, void *const dst );

/**
 * @brief jumping to certain point function
 * 
 * @param[in,out] self  virtual machine perform operation in
 * @param[in]     point point to jump to
 */
void cfVmJump( CfVm *const self, const uint32_t point );

/**
 * @brief generic conditional jump instruction implementation
 * 
 * @param[in,out] self      vm to perform action in
 * @param[in]     condition condition to 
 */
void cfVmGenericConditionalJump( CfVm *const self, const bool condition );

/**
 * @brief instruction counter to call stack pushing function
 * 
 * @param[in,out] self virtual machine to perform operation in
 */
void cfVmPushIC( CfVm *const self );

/**
 * @brief instruction counter from call stack popping function
 * 
 * @param[in,out] self virtual machine to perform operation in
 */
void cfVmPopIC( CfVm *const self );

/**
 * @brief value to register writing function
 * 
 * @param[in,out] self  virtual machine to perform operation in pointer
 * @param[in]     reg   register to write index
 * @param[in]     value value intended to write to register
 */
void cfVmWriteRegister( CfVm *const self, const uint32_t reg, const uint32_t value );

/**
 * @brief VM video mode setting function
 * 
 * @param storageFormat pixel storage format
 * @param updateMode    screen and memory content synchronization mode
 * 
 * @note all input enumerations are assumed to be valid
 */
void cfVmSetVideoMode(
    CfVm *const self,
    const CfVideoStorageFormat storageFormat,
    const CfVideoUpdateMode updateMode
);

/**
 * @brief value to register writing function
 * 
 * @param[in,out] self virtual machine to perform operation in
 * @param[in]     reg  index of register to read
 * 
 * @return value from register by **reg** index
 */
uint32_t cfVmReadRegister( CfVm *const self, const uint32_t reg );

/**
 * @brief memory pointer getting function
 * 
 * @param[in,out] self vm pointer
 * @param[in]     addr address to access memory by
 * 
 * @return pointer to memory location, always valid to read/write 4 bytes and non-null
 */
void * cfVmGetMemoryPointer( CfVm *const self, const uint32_t addr );

/**
 * @brief VM execution starting function
 * 
 * @param[in] vm reference of VM to start execution in
 */
void cfVmRun( CfVm *const self );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_VM_INTERNAL_H_)

// cf_vm_internal.h
