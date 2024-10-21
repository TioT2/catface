/**
 * @brief virtual machine declaration file
 */

#ifndef CF_VM_H_
#define CF_VM_H_

#include <cf_executable.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief reason of program termination
typedef enum __CfTermReason {
    // sef of reasons that are executable-independent
    CF_TERM_REASON_HALT,                 ///< program finished without errors
    CF_TERM_REASON_SANDBOX_ERROR,        ///< sandbox function call failed
    CF_TERM_REASON_INTERNAL_ERROR,       ///< something went totally wrong in VM itself

    // set of reasons called by executable invalidness
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
    CF_TERM_REASON_INVALID_VIDEO_MODE,   ///< invalid video mode
    CF_TERM_REASON_SEGMENTATION_FAULT,   ///< good old segfault
    CF_TERM_REASON_INVALID_POP_INFO,     ///< invalid push/pop info for pop instruction
} CfTermReason;

/// @brief description of program termination reason
typedef struct __CfTermInfo {
    CfTermReason  reason; ///< panic reason
    size_t        offset; ///< offset of code then execution stopped

    union {
        uint8_t       unknownOpcode;     ///< unknown opcode
        uint32_t      unknownRegister;   ///< index of unknown register
        uint32_t      unknownSystemCall; ///< index of unknown systemCall
        CfPushPopInfo invalidPopInfo;    ///< invalid push/pop info for pop instruction data

        struct {
            uint8_t storageFormatBits : 3; ///< bits of video mode
            uint8_t updateModeBits    : 1; ///< bits of update mode
        } invalidVideoMode;

        struct {
            uint32_t memorySize; ///< memory size
            uint32_t addr;       ///< address
        } segmentationFault;
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
     * @brief screen manual refreshing function
     * 
     * @param[in] userContext   user-provided context
     * @param[in] storageFormat image data storage foramt
     * 
     * @return true if succeeded, false if something went wrong.
     */
    bool (*refreshScreen)( void *userContext );

    /**
     * @brief video mode setting callback
     * 
     * @param[in] userContext   user-provided context
     * @param[in] storageFormat new video data storage format
     * @param[in] updateMode    new screen update mode
     * 
     * @return true if succeeded, false if something went wrong.
     */
    bool (*setVideoMode)( void *userContext, CfVideoStorageFormat storageFormat, CfVideoUpdateMode updateMode );

    /**
     * @brief program execution time (in seconds) getting function
     * 
     * @param[in]  userContext user-provided context
     * @param[out] dst         time destination
     * 
     * @return true if succeeded, false if something went wrong.
     */
    bool (*getExecutionTime)( void *userContext, float *dst );

    /**
     * @brief 64-bit integer reading function pointer
     * 
     * @param userContext pointer to some user context
     * 
     * @return some 64-bit integer
     */
    double (*readFloat64)( void *userContext );

    /**
     * @brief 64-bit integer outputting function
     * 
     * @param userContext pointer to some user context
     * @param number      number to write to output
     */
    void (*writeFloat64)( void *userContext, double number );
} CfSandbox;

/**
 * @brief executable execution function
 * 
 * @param[in] executable  executable to execute (non-null, assumed to be valid ASM)
 * @param[in] sandbox connection of VM with environment (non-null)
 * 
 * @return true if execution started, false if not
 */
bool cfExecute( const CfExecutable *executable, const CfSandbox *sandbox );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_VM_H_)

// cf_vm.h
