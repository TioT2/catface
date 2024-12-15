/**
 * @brief code generator library
 */

#ifndef CF_CODEGEN_H_
#define CF_CODEGEN_H_

#include <cf_object.h>
#include <cf_tir.h>

/**
 * @brief compile TIR to CF Object format
 * 
 * @param[in] tir TIR pointer (non-null)
 * @param[in] dst code generation destination (non-null)
 * 
 * @return built object
 */
bool cfCodegen( const CfTir *tir, CfObject *dst );

#endif // !defined(CF_CODEGEN_H_)

// cf_codegen.h
