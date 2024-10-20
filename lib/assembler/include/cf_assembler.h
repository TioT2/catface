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

/// Assembling status
typedef enum __CfAssemblyStatus {
    CF_ASSEMBLY_STATUS_OK,                       ///< all's ok
    CF_ASSEMBLY_STATUS_INTERNAL_ERROR,           ///< internal error (e.g. error that shouldn't occur in normal situation, such as memory allocation failure etc.)
    CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION,      ///< unknown instruction occured
    CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END,      ///< unexpected text end (missing something)
    CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER,         ///< unknown register
    CF_ASSEMBLY_STATUS_UNKNOWN_LABEL,            ///< unknown label
    CF_ASSEMBLY_STATUS_DUPLICATE_LABEL,          ///< duplicated label declaration
    CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT, ///< invalid argument of push/pop instructions
    CF_ASSEMBLY_STATUS_TOO_LONG_LABEL,           ///< label is longer than CF_LABEL_MAX
} CfAssemblyStatus;

/// @brief detailed info about assembling process
typedef union __CfAssemblyDetails {
    CfStr unknownRegister;
    CfStr tooLongLabel;

    struct {
        CfStr  instruction; ///< the instruction (whole line)
        size_t line;        ///< line the instruction occured
    } unknownInstruction;

    struct {
        CfStr  label; ///< unknown label name
        size_t line;  ///< line reference of this label occured
    } unknownLabel;


    struct {
        CfStr  label;             ///< label itself
        size_t firstDeclaration;  ///< line there duplicated label was initially declared
        size_t secondDeclaration; ///< line there duplicated label was secondary declared
    } duplicateLabel;

    struct {
        CfStr  argument; ///< argument text
        size_t line;     ///< line
    } invalidPushPopArgument;
} CfAssemblyDetails;

/**
 * @brief text slice assembling function
 * 
 * @param[in]  text       code to assembly (valid)
 * @param[out] dst        assembling destination (non-null)
 * @param[out] details    detailed info about assembling process (nullable)
 * 
 * @return assembling status
 */
CfAssemblyStatus cfAssemble( CfStr text, CfExecutable *dst, CfAssemblyDetails *details );

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
void cfAssemblyDetailsDump( FILE *out, CfAssemblyStatus status, const CfAssemblyDetails *details );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_ASSEMBLER_H_)

// cf_asm.h
