/**
 * @brief virtual machine declaration file
 */

#ifndef CF_VM_H_
#define CF_VM_H_

#include <cf_module.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief reason of program termination
typedef enum __CfTermReason {
    // sef of reasons that are module-independent
    CF_TERM_REASON_HALT,                 ///< program finished without errors
    CF_TERM_REASON_SANDBOX_ERROR,        ///< sandbox function call failed
    CF_TERM_REASON_INTERNAL_ERROR,       ///< something went totally wrong in VM itself

    // set of reasons called by module invalidness
    CF_TERM_REASON_UNKNOWN_SYSTEM_CALL,  ///< invalid index of systemcall
    CF_TERM_REASON_UNKNOWN_OPCODE,       ///< unknown opcode passed
    CF_TERM_REASON_UNEXPECTED_CODE_END,  ///< unexpected end of bytecode
    CF_TERM_REASON_UNKNOWN_REGISTER,     ///< unknown register index

    // set of reasons called by execution errors
    CF_TERM_REASON_UNREACHABLE,          ///< unreachable
    CF_TERM_REASON_NO_OPERANDS,          ///< no arguments on stack
    CF_TERM_REASON_STACK_UNDERFLOW,      ///< operand stack underflow
    CF_TERM_REASON_CALL_STACK_UNDERFLOW, ///< call stack underflow
    CF_TERM_REASON_INVALID_IC,           ///< invalid jump
} CfTermReason;

/// @brief description of program termination reason
typedef struct __CfTermInfo {
    CfTermReason  reason; ///< panic reason
    size_t        offset; ///< offset of code then execution stopped

    union {
        uint8_t  unknownOpcode;     ///< unknown opcode
        uint32_t unknownRegister;   ///< index of unknown register
        uint32_t unknownSystemCall; ///< index of unknown systemCall
    };
} CfTermInfo;

/// @brief virtual machine context representation sturcture (passed from VM to user during sandbox initialization)
typedef struct __CfExecContext {
    void   * memory;     ///< VM memory
    size_t   memorySize; ///< memory size (usually 1 MB)
} CfExecContext;

/// @brief sandbox description structure.
typedef struct __CfSandbox {
    void *userContext; ///< some pointer user may pass into this structure

    /**
     * @brief initialization callback
     * 
     * @param userContext pointer, passed by user into context
     * @param execContext execution context, that contains all data that may be shared by user (non-null)
     * 
     * @return true if initialized, false if something went wrong
     * 
     * @note user **must not** save pointer of execContext, because execContext is temporary structure.
     * @note terminate() callback **is not called** in case this function returns false.
     */
    bool (*initialize)( void *userContext, const CfExecContext *execContext );

    /**
     * @brief termination callback
     * 
     * @param userContextPtr pointer to some user context
     * @param termInfo       information about program termination
     */
    void (*terminate)( void *userContext, const CfTermInfo *termInfo );

    /**
     * @brief 64-bit integer reading function pointer
     * 
     * @param userContextPtr pointer to some user context
     * 
     * @return some 64-bit integer
     */
    double (*readFloat64)( void *userContextPtr );

    /**
     * @brief 64-bit integer outputting function
     * 
     * @param userContextPtr pointer to some user context
     * @param number         number to write to output
     */
    void (*writeFloat64)( void *userContextPtr, double number );
} CfSandbox;

/**
 * @brief module execution function
 * 
 * @param[in] module  module to execute (non-null, assumed to be valid ASM)
 * @param[in] sandbox connection of VM with environment (non-null)
 * 
 * @return true if execution started, false if not
 */
bool cfModuleExec( const CfModule *module, const CfSandbox *sandbox );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_VM_H_)

// cf_vm.h
