/**
 * @brief CF binary module basic functions declaration file
 */

#ifndef CF_MODULE_H_
#define CF_MODULE_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Count of registers used in vm
#define CF_REGISTER_COUNT 4

/// @brief Instruction header representation enumeration
typedef enum __CfOpcode {
    // system instructions
    CF_OPCODE_UNREACHABLE, ///< unreachable instruction, calls panic
    CF_OPCODE_SYSCALL,     ///< function by pre-defined index from without of sandbox calling function

    // i32 common instructions
    CF_OPCODE_ADD, ///< 32-bit integer
    CF_OPCODE_SUB, ///< 32-bit integer
    CF_OPCODE_SHL, ///< shift 32-bit integer left
    CF_OPCODE_SHR, ///< shift signed   i32 right
    CF_OPCODE_SAR, ///< shift unsigned i32 right

    // i32 signed/unsigned instructions
    CF_OPCODE_IMUL, ///< signed   i32 multiplication
    CF_OPCODE_MUL,  ///< unsigned i32 multiplication
    CF_OPCODE_IDIV, ///< signed   i32 division
    CF_OPCODE_DIV,  ///< unsigned i32 division

    // f32 arithmetical instructions
    CF_OPCODE_FADD, ///< f32 addition
    CF_OPCODE_FSUB, ///< f32 substraction
    CF_OPCODE_FMUL, ///< f32 multiplication
    CF_OPCODE_FDIV, ///< f32 division

    // f32-i32 instructions
    CF_OPCODE_FTOI, ///< f32 into signed i32 conversion
    CF_OPCODE_ITOF, ///< signed i32 into f32 conversion

    // common 32-bit instructions
    CF_OPCODE_PUSH,   ///< 32-bit literal pushing opcode
    CF_OPCODE_POP,    ///< 32-bit value removing opcode
    CF_OPCODE_PUSH_R, ///< push into stack from register
    CF_OPCODE_POP_R,  ///< pop  from stack into register
} CfOpcode;

/// @brief CF compiled module represetnation structure
typedef struct __CfModule {
    void    *code;       ///< module bytecode
    size_t   codeLength; ///< module bytecode length
} CfModule;

/// @brief module reading status
typedef enum __CfModuleReadStatus {
    CF_MODULE_READ_STATUS_OK,                   ///< succeeded
    CF_MODULE_READ_STATUS_INTERNAL_ERROR,       ///< some internal error occured
    CF_MODULE_READ_STATUS_UNEXPECTED_FILE_END,  ///< unexpected end of file
    CF_MODULE_READ_STATUS_INVALID_MODULE_MAGIC, ///< invalid module magic number
    CF_MODULE_READ_STATUS_CODE_INVALID_HASH,    ///< invalid module code hash
} CfModuleReadStatus;

/**
 * @brief module from file reading function
 * 
 * @param[in] file   file to read module from, should allow "rb" access
 * @param[in] module module to write to file (non-null)
 * 
 * @return module reading status
 */
CfModuleReadStatus cfModuleRead( FILE *file, CfModule *dst );

/// @brief module to file writing status
typedef enum __CfModuleWriteStatus {
    CF_MODULE_WRITE_STATUS_OK,          ///< succeeded
    CF_MODULE_WRITE_STATUS_WRITE_ERROR, ///< writing to file operation failed
} CfModuleWriteStatus;

/**
 * @brief module to file writing function
 * 
 * @param[in]  module module to dump to file (non-null)
 * @param[out] file   destination file, should allow "wb" access
 * 
 * @return operation status
 */
CfModuleWriteStatus cfModuleWrite( const CfModule *module, FILE *dst );

/**
 * @brief module destructor
 * 
 * @param[in] module module to destroy
 */
void cfModuleDtor( CfModule *module );


/**
 * @brief module read status corresponding string getting function
 * 
 * @param[in] status module read status
 * 
 * @return corresponding string. In case of invalid status returns "<invalid>"
 */
const char * cfModuleReadStatusStr( CfModuleReadStatus status );

/**
 * @brief module write status corresponding string getting function
 * 
 * @param[in] status module writing status
 * 
 * @return corresponding string. In case of invalid status returns "<invalid>"
 */
const char * cfModuleWriteStatusStr( CfModuleWriteStatus status );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_MODULE_H_)

// cf_module.h
