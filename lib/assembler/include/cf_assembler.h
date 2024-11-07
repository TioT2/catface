/**
 * @brief CFASM -> CFOBJ compilator declaration file
 */

#ifndef CF_ASSEMBLER_H_
#define CF_ASSEMBLER_H_

#include <cf_executable.h>
#include <cf_object.h>
#include <cf_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Assembling status
typedef enum __CfAssemblyStatus {
    CF_ASSEMBLY_STATUS_OK,                       ///< all's ok
    CF_ASSEMBLY_STATUS_INTERNAL_ERROR,           ///< internal error (e.g. error that shouldn't occur in normal situation, such as memory allocation failure etc.)
    CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION,      ///< unknown instruction occured
    CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END,      ///< unexpected text end (missing something)
    CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER,         ///< unknown register
    CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT, ///< invalid argument of push/pop instructions
    CF_ASSEMBLY_STATUS_UNKNOWN_OPCODE,           ///< unknown opcode
    CF_ASSEMBLY_STATUS_UNKNOWN_TOKEN,            ///< unknown token

    CF_ASSEMBLY_STATUS_INVALID_SYSCALL_ARGUMENT, ///< invalid argument of 'systemcall' instruction
    CF_ASSEMBLY_STATUS_SYSCALL_ARGUMENT_MISSING, ///< systemcall parameter missing.

    CF_ASSEMBLY_STATUS_INVALID_JUMP_ARGUMENT,    ///< invalid jump-family instructino argument
    CF_ASSEMBLY_STATUS_JUMP_ARGUMENT_MISSING,    ///< jump-family instruction argument missing

    CF_ASSEMBLY_STATUS_EMPTY_LABEL,              ///< label must not be empty
    CF_ASSEMBLY_STATUS_TOO_LONG_LABEL,           ///< label is longer than CF_LABEL_MAX

    CF_ASSEMBLY_STATUS_INVALID_CONSTANT_VALUE,   ///< invalid constant value

    CF_ASSEMBLY_STATUS_UNEXPECTED_CHARACTERS,    ///< unexpected (a.k.a. unrelated to instruction) characters occured.
} CfAssemblyStatus;

/// @brief detailed info about assembling process
typedef struct __CfAssemblyDetails {
    size_t line;     ///< index of line during parsing of error occured
    CfStr  contents; ///< line error occured at contents
} CfAssemblyDetails;

/**
 * @brief text slice assembling function
 *
 * @param[in]  text       code to assembly (valid)
 * @param[in]  sourceName name of file source
 * @param[out] dst        assembling destination (non-null)
 * @param[out] details    detailed info about assembling process (nullable)
 *
 * @return assembling status
 */
CfAssemblyStatus cfAssemble( CfStr text, CfStr sourceName, CfObject *dst, CfAssemblyDetails *details );

/**
 * @brief assembly status to string conversion error
 * 
 * @param[in] status assembly status
 * 
 * @return assembly status converted to string. In case of invalid status <invalid> returned
 */
const char * cfAssemblyStatusStr( CfAssemblyStatus status );

/**
 * @brief assembly details dumping function
 * 
 * @param[in,out] out     output file
 * @param[in]     status  assembly operation status
 * @param[in]     details corresponding assembly details (non-null)
 */
void cfAssemblyDetailsWrite( FILE *out, CfAssemblyStatus status, const CfAssemblyDetails *details );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_ASSEMBLER_H_)

// cf_asm.h
