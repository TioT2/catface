/**
 * @brief AST parser internal declaration file
 */

#ifndef CF_AST_INTERNAL_H_
#define CF_AST_INTERNAL_H_

#include "cf_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief AST main structure
typedef struct __CfAstImpl {
    CfArena      mem;            ///< AST allocation holder
    CfStr        sourceName;     ///< source file name
    CfStr        sourceContents; ///< source file contents
    CfAstDecl  * declArray;      ///< declaration array (extends beyond structure memory for declCount - 1 elements)
    size_t       declArrayLen;   ///< declaration array length
} CfAstImpl;

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_INTERNAL_H_)

// cf_ast_internal.h
