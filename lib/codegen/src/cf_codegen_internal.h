/**
 * @brief code generator internal definition file
 */

#ifndef CF_CODEGEN_INTERNAL_H_
#define CF_CODEGEN_INTERNAL_H_

#include <setjmp.h>

#include <cf_deque.h>
#include <cf_executable.h>

#include "cf_codegen.h"


#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)

/// @brief code generator
typedef struct CfCodeGenerator {
    CfArena         * tempArena;         ///< temporary arena
    CfDeque         * codeDeque;         ///< code destination
    CfDeque         * linkDeque;         ///< link deque
    CfDeque         * labelDeque;        ///< label deque

    const CfTir     * tir;               ///< TIR
    CfStr             currentFunction;   ///< current function name
    uint32_t          conditionCounter;  ///< counter for generating condition labels
    uint32_t          loopCounter;       ///< counter for generating loop labels

    jmp_buf           finishBuffer;      ///< finishing buffer
    CfCodegenResult   result;            ///< result
} CfCodeGenerator;

/**
 * @brief immediately terminate code generation process
 * 
 * @param[in] self   codegenerator pointer
 * @param[in] result codegeneration result
 */
void cfCodeGeneratorFinish( CfCodeGenerator *const self, CfCodegenResult result );

/**
 * @brief simple assertion
 * 
 * @param[in] self      code generator pointer
 * @param[in] condition condition
 * 
 * @note function finishes code generation with INTERNAL_ERROR if condition is false.
 */
void cfCodeGeneratorAssert( CfCodeGenerator *const self, bool condition );

/**
 * @brief allocate some memory on temp arena
 * 
 * @param[in] self code generator
 * @param[in] size required size
 * 
 * @return allocated code block pointer (non-null)
 * 
 * @note in case of allocation fail, throws INTERNAL_ERROR
 */
void * cfCodeGeneratorAllocTemp( CfCodeGenerator *const self, size_t size );

/**
 * @brief to resulting code deque writing function
 * 
 * @param[in] self
 */
void cfCodeGeneratorWriteCode( CfCodeGenerator *const self, const void *code, size_t size );

/**
 * @brief check str length (if str if longer, than CF_LABEL_MAX - 1, finishes with TOO_LONG_NAME error)
 * 
 * @param[in] self codegenerator pointer
 * @param[in] str  str to check length of
 */
void cfCodeGeneratorCheckStrLength( CfCodeGenerator *const self, CfStr str );

/**
 * @brief add link to certain label declared in code right here
 * 
 * @param[in] self   codegenerator pointer
 * @param[in] linkTo label to add link to
 */
void cfCodeGeneratorAddLink( CfCodeGenerator *const self, CfStr linkTo );

/**
 * @brief add label to current code point
 * 
 * @param[in] self      code generator pointer
 * @param[in] labelName label itself
 */
void cfCodeGeneratorAddLabel( CfCodeGenerator *const self, CfStr labelName );

/**
 * @brief add label to current code point
 * 
 * @param[in] self  code generator pointer
 * @param[in] name  name of constant to add
 * @param[in] value constant value
 * 
 * @note this funciton adds new non-relative label
 */
void cfCodeGeneratorAddConstant( CfCodeGenerator *const self, CfStr name, uint32_t value );

/**
 * @brief code generator start function
 * 
 * @param[in] self code generator
 * 
 * @note writes prelude to codegenerator
 */
void cfCodeGeneratorBegin( CfCodeGenerator *const self );

/**
 * @brief write push/pop instruction
 * 
 * @param[in] self      code generator pointer
 * @param[in] opcode    opcode (should be push or pop)
 * @param[in] info      push/pop info
 * @param[in] immediate immediate (ignored if doReadImmediate info field is false)
 */
void cfCodeGeneratorWritePushPop(
    CfCodeGenerator *const self,
    CfOpcode               opcode,
    CfPushPopInfo          info,
    int32_t                immediate
);

/**
 * @brief write opcode
 * 
 * @param[in] self   code generator
 * @param[in] opcode opcode to write
 */
void cfCodeGeneratorWriteOpcode( CfCodeGenerator *const self, CfOpcode opcode );

/**
 * @brief generate expression code
 * 
 * @param[in] self       code generator
 * @param[in] expression expression to generate code for
 */
void cfCodeGeneratorGenExpression( CfCodeGenerator *const self, const CfTirExpression *expression );

/**
 * @brief return expression generation function
 * 
 * @param[in] self  code generator
 * @param[in] value returned value
 */
void cfCodeGeneratorGenReturn( CfCodeGenerator *const self );

/**
 * @brief generate code block
 * 
 * @param[in] self  code generator
 * @param[in] block block to generate code for
 */
void cfCodeGeneratorGenBlock( CfCodeGenerator *const self, const CfTirBlock *block );

/**
 * @brief generate statement code
 * 
 * @param[in] self  code generator
 * @param[in] block statement to generate code of
 */
void cfCodeGeneratorGenStatement( CfCodeGenerator *const self, const CfTirStatement *statement );

/**
 * @brief codegen a function
 * 
 * @param[in] self     codeGenerator
 * @param[in] function TIR function pointer
 */
void cfCodeGeneratorGenFunction( CfCodeGenerator *const self, const CfTirFunction *function );

/**
 * @brief code generation finishing function
 * 
 * @param[in] self       code generator
 * @param[in] sourceName source file name
 * @param[in] objectDst  object destination (non-null)
 */
void cfCodeGeneratorEnd( CfCodeGenerator *const self, CfStr sourceName, CfObject *objectDst );

#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#endif // !defined(CF_CODEGEN_INTERNAL_H_)

// cf_codegen_internal.h
