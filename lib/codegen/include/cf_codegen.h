/**
 * @brief code generator library
 */

#ifndef CF_CODEGEN_H_
#define CF_CODEGEN_H_

#include <cf_object.h>
#include <cf_tir.h>

/// @brief code generation status
typedef enum CfCodegenStatus_ {
    CF_CODEGEN_STATUS_OK,                 ///< parsing succeeded
    CF_CODEGEN_STATUS_INTERNAL_ERROR,     ///< internal error
    CF_CODEGEN_STATUS_TOO_LONG_NAME,      ///< function name is too long
    CF_CODEGEN_STATUS_RESERVED_NAME_USED, ///< reserved name computed
} CfCodegenStatus;

/// @brief code generation result
typedef struct CfCodegenResult_ {
    CfCodegenStatus status; ///< codegen status

    union {
        CfStr tooLongName;  ///< name is TOO long (exceeds CF_LABEL_MAX)
        CfStr reservedName; ///< reserved name
    };
} CfCodegenResult;

/**
 * @brief compile TIR to CF Object format
 * 
 * @param[in] tir       TIR pointer (non-null)
 * @param[in] dst       code generation destination (non-null)
 * @param[in] tempArena arena for temporary variable allocation (nullable)
 * 
 * @return code generation result
 */
CfCodegenResult cfCodegen( const CfTir *tir, CfObject *dst, CfArena *tempArena );

#endif // !defined(CF_CODEGEN_H_)

// cf_codegen.h
