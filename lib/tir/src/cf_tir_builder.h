/**
 * @brief TIR builder declaration file
 */

#ifndef CF_TIR_BUILDER_H_
#define CF_TIR_BUILDER_H_

#include <assert.h>
#include <setjmp.h>

#include <cf_deque.h>

#include "cf_tir.h"

#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)

/// @brief TIR, actually
struct CfTir_ {
    CfArena       * dataArena;           ///< actual content storage arena
    CfTirFunction * functionArray;       ///< functions declared/implemented in this module
    size_t          functionArrayLength; ///< function array length
    CfStr           sourceName;          ///< source file name
}; // struct CfTir

/// @brief build-time internal function structure
typedef struct CfTirBuilderFunction_ {
    CfTirFunction         function;     ///< tir-level function
    CfTirFunctionId       id;           ///< identifier
    const CfAstFunction * astFunction;  ///< AST function
} CfTirBuilderFunction;

/// @brief TIR builder
typedef struct CfTirBuilder_ {
    CfArena             * dataArena;   ///< 'final' allocation arena
    CfArena             * tempArena;   ///< temporary allocation arena
    CfDeque             * functions;   ///< set of defined functions
    jmp_buf               errorBuffer; ///< error buffer
    CfTirBuildingResult   error;       ///< error itself
} CfTirBuilder;

/**
 * @brief immediately finish building process
 * 
 * @param[in] self   building pointer
 * @param[in] result building result (must be error)
 * 
 * @note this function would never return
 */
void cfTirBuilderFinish( CfTirBuilder *const self, CfTirBuildingResult result );

/**
 * @brief assertion (finishes with INTERNAL_ERROR if condition is false)
 * 
 * @param[in] self builder pointer
 * @param[in] cond condition
 */
void cfTirBuilderAssert( CfTirBuilder *const self, bool cond );

/**
 * @brief allocate in temporary arena
 * 
 * @param[in] self builder pointer
 * @param[in] size allocation size
 * 
 * @return allocated memory pointer (non-null)
 * 
 * @note the function throws INTERNAL_ERROR in case of allocation fail.
 */
void * cfTirBuilderAllocTemp( CfTirBuilder *const self, size_t size );

/**
 * @brief allocate in data arena
 * 
 * @param[in] self builder pointer
 * @param[in] size allocation size
 * 
 * @return allocated memory pointer (non-null)
 * 
 * @note the function throws INTERNAL_ERROR in case of allocation fail.
 */
void * cfTirBuilderAllocData( CfTirBuilder *const self, size_t size );

/**
 * @brief get function by name
 * 
 * @param[in] self         tir builder
 * @param[in] functionName name of function to search for
 * 
 * @return found function pointer (NULL if failed)
 */
CfTirBuilderFunction * cfTirBuilderFindFunction( CfTirBuilder *const self, CfStr functionName );

/**
 * @brief get function by id
 * 
 * @param[in] self       tir builder
 * @param[in] functionID function id
 * 
 * @return function pointer (throws INTERNAL_ERROR if function not found)
 */
CfTirBuilderFunction * cfTirBuilderGetFunction( CfTirBuilder *const self, CfTirFunctionId functionId );

/**
 * @brief start function building phase enerance
 * 
 * @param[in] self TIR builder
 * 
 * @note this phase builds function implemetnations 
 */
void cfTirBuildFunctions( CfTirBuilder *const self );

/**
 * @brief TIR type from AST one building function
 * 
 * @param[in] type AST type
 * 
 * @return corresponding TIR type
 */
CfTirType cfTirTypeFromAstType( CfAstType type );

/**
 * @brief TIR type from AST one building function
 * 
 * @param[in] type TIR type
 * 
 * @return corresponding AST type
 */
CfAstType cfAstTypeFromTirType( CfTirType type );

/**
 * @brief convert AST binary operator to TIR binary operator
 * 
 * @param[in] op AST binary operator
 * 
 * @return corresponding TIR binary operator
 */
CfTirBinaryOperator cfTirBinaryOperatorFromAstBinaryOperator( CfAstBinaryOperator op );

#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#endif // !defined(CF_TIR_BUILDER_H_)

// cf_tir_builder.h
