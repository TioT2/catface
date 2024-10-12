#ifndef CF_VM_H_
#define CF_VM_H_

#include <cf_module.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/// @brief reason of occured panic
typedef enum __CfPanicReason {
    CF_PANIC_REASON_UNKNOWN_OPCODE,      ///< unknown instruction passed
    CF_PANIC_REASON_UNREACHABLE,         ///< unreachable
    CF_PANIC_REASON_UNKNOWN_INT,         ///< unknown interrupt
    CF_PANIC_REASON_INTERNAL_ERROR,      ///< VM internal error
    CF_PANIC_REASON_NO_OPERANDS,         ///< no arguments on stack
    CF_PANIC_REASON_UNKNOWN_SYSTEM_CALL, ///< invalid index of systemcall
} CfPanicReason;

/// @brief description of occured panic
typedef struct __CfPanicInfo {
    CfPanicReason reason; ///< panic reason
    size_t        offset; ///< offset of code then instruction interrupted

    union {
        struct {
            uint32_t interrupt; ///< interrupt index
        } unknownInt;

        struct {
            uint16_t opcode; ///< the unknown opcode
        } unknownOpcode;

        struct {
            uint32_t index; ///< unknown systemcall
        } unknownSystemCall;
    };
} CfPanicInfo;

/// @brief sandbox description structure.
/// Actually, this structure is temporary and (then functions will be introduced)
/// will be partially (readInt64 and writeInt64 fields) replaced by import table.
typedef struct __CfSandbox {
    void *userContextPtr; ///< some pointer user may pass into this structure

    /**
     * @brief 64-bit integer reading function pointer
     * 
     * @param userContextPtr pointer to some user context
     * 
     * @return some 64-bit integer
     */
    int64_t (*readInt64)( void *userContextPtr );

    /**
     * @brief 64-bit integer outputting function
     * 
     * @param userContextPtr pointer to some user context
     * @param i              integer to write to output
     */
    void (*writeInt64)( void *userContextPtr, int64_t i );

    /**
     * @brief panic handle function
     * 
     * @param userContextPtr pointer to some user context
     * @param panicInfo useful information for panic
     */
    void (*handlePanic)( void *userContextPtr, const CfPanicInfo *panicInfo );
} CfSandbox;

/**
 * @brief module execution function
 * 
 * @param[in] module  module to execute (non-null, assumed to be valid ASM)
 * @param[in] sandbox connection of VM with environment (non-null)
 */
void cfModuleExec( const CfModule *module, const CfSandbox *sandbox );

#endif // !defined(CF_VM_H_)

// cf_vm.h
