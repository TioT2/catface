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
typedef enum CfAssemblyStatus_ {
    CF_ASSEMBLY_STATUS_OK,             ///< all's ok
    CF_ASSEMBLY_STATUS_ERORRS_OCCURED, ///< assembly errors occured
    CF_ASSEMBLY_STATUS_INTERNAL_ERROR, ///< internal error (e.g. error that shouldn't occur in normal situation, such as memory allocation failure etc.)
} CfAssemblyStatus;

/// @brief detailed info about assembling process
typedef struct CfAssemblyDetails_ {
    size_t line;     ///< index of line during parsing of error occured
    CfStr  contents; ///< line error occured at contents
} CfAssemblyDetails;

/// @brief assembler erorr kind
typedef enum CfAssemblyErrorKind_ {
    CF_ASSEMBLY_ERROR_KIND_NEWLINE_EXPECTED,         ///< newline expected (e.g. unexpected sh*t occured on previous line)
    CF_ASSEMBLY_ERROR_KIND_TOO_LONG_LABEL,           ///< label is longer than CF_LABEL_MAX
    CF_ASSEMBLY_ERROR_KIND_CONSTANT_NUMBER_EXPECTED, ///< number as constant value expected
    CF_ASSEMBLY_ERROR_KIND_UNIDENTIFIED_CODE,        ///< cannot identify code type
    CF_ASSEMBLY_ERROR_KIND_UNKNOWN_REGISTER,         ///< unknown register
    CF_ASSEMBLY_ERROR_KIND_UNEXPECTED_CHARACTER,     ///< tokenization error
    CF_ASSEMBLY_ERROR_KIND_INVALID_SYSCALL_ARGUMENT, ///< invalid syscall argument
    CF_ASSEMBLY_ERROR_KIND_INVALID_JUMP_ARGUMENT,    ///< invalid jump-family instructino argument
    CF_ASSEMBLY_ERROR_KIND_INVALID_PUSHPOP_ARGUMENT, ///< invalid push/pop argument
} CfAssemblyErrorKind;

/// @brief assembly process error
typedef struct CfAssemblyError_ {
    CfAssemblyErrorKind kind;         ///< error kind
    size_t              lineIndex;    ///< index of line error occured at
    CfStr               lineContents; ///< contents of line error occured at

    union {
        CfStrSpan newlineExpected;        ///< some trash code
        CfStrSpan tooLongLabel;           ///< label span
        CfStrSpan constantNumberExpected; ///< constant number expected
        CfStrSpan invalidSyscallArgument; ///< invalid syscall argument
        CfStrSpan invalidJumpArgument;    ///< invalid jump argument
        CfStrSpan unknownRegister;        ///< unknown register

        const char *unexpectedCharacter; ///< character pointer
    };
} CfAssemblyError;

/// @brief assembly result
typedef struct CfAssemblyResult_ {
    CfAssemblyStatus status; ///< assembly status

    union {
        CfObject *ok; ///< success case

        struct {
            CfAssemblyError *erorrArray;       ///< assembly error array
            size_t           errorArrayLength; ///< error array length
        } errorsOccured;
    };
} CfAssemblyResult;

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
CfAssemblyResult cfAssemble( CfStr text, CfStr sourceName );

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
