/**
 * @brief stack utility declaration file
 */

#ifndef CF_STACK_H_
#define CF_STACK_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief stack handle representation typedef
typedef struct __CfStackImpl * CfStack;

typedef enum __CfPopStatus {
    CF_STACK_OK,             ///< succeeded
    CF_STACK_NO_VALUES,      ///< no values on stack
    CF_STACK_INTERNAL_ERROR, ///< stack internal error occured
} CfStackStatus;

/**
 * @brief stack constructor
 * 
 * @param[in] elementSize stack element size (non-zero)
 * 
 * @return created stack (may be null)
 */
CfStack cfStackCtor( size_t elementSize );

/**
 * @brief stack destructor
 * 
 * @param[in] stack stack to destroy (MUST be constructed)
 */
void cfStackDtor( CfStack stack );

/**
 * @brief value to stack pushing function
 * 
 * @param[in,out] self stack to push value to (non-null)
 * @param[in]     data data to push to stack (non-null, must have enough readable space
 * 
 * @return true if pushed, false if smth went very wrong
 */
CfStackStatus cfStackPush( CfStack *self, const void *data );

/**
 * @brief value from stack popping function
 * 
 * @param[in,out] self stack to pop value from (non-null)
 * @param[out]    data buffer to read data to (nullable, in the non-null case must have at least elementSize bytes of writable space)
 * 
 * @return true if popped, false if smth went wrong
 */
CfStackStatus cfStackPop( CfStack *self, void *data );

/**
 * @brief stack to underlying data
 * 
 * @param[in]  stack stack to build array from
 * @param[out] dst   stack converted to array destination
 * 
 * @note elements of destination array are located in PUSHing order
 * (e.g. first element of resulting array will be first element pushed to the stack.).
 * Also, call of this function DOES NOT destroys stack.
 * 
 * @return true if allocated successfully, false if not. This strange output and input combination is required, because it's ok for dst to be null in case if stack is empty.
 * 
 */
bool cfStackToArray( CfStack stack, void **dst );

/**
 * @brief stack size getting function
 * 
 * @param[in] stack stack to get size of (non-null)
 * 
 * @return count of elements already pushed to stack
 */
size_t cfStackGetSize( CfStack stack );

/**
 * @brief array to stack in reversed order pushing function (e.g. last element of array will be located on top of stack)
 * 
 * @param[in,out] stack        stack to push elements to (non-null)
 * @param[in]     array        data to push to stack (non-null)
 * @param[in]     elementCount count of pushed elements
 * 
 * @return operation status
 */
CfStackStatus cfStackPushArrayReversed( CfStack *stack, const void *array, size_t elementCount );

#ifdef __cplusplus
}
#endif

#endif // !defiend(CF_STACK_H_)
