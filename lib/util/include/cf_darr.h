/**
 * @brief dynamic-sized array utility declaration file
 */

#ifndef CF_DARR_H_
#define CF_DARR_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/// @brief dynamic array handle definition
typedef struct __CfDarrImpl * CfDarr;

/// @brief dynamic array operation status
typedef enum __CfDarrStatus {
    CF_DARR_OK,             ///< succeeded
    CF_DARR_NO_VALUES,      ///< no values
    CF_DARR_INTERNAL_ERROR, ///< internal error occured
} CfDarrStatus;

/**
 * @brief dynamic array create function
 * 
 * @param[in] elementSize size of dynamic array element
 * 
 * @return created dynamic array, may be null
 */
CfDarr cfDarrCtor( size_t elementSize );

/**
 * @brief dynamic array destructor
 * 
 * @param[in] darr dynamic array
 */
void cfDarrDtor( CfDarr darr );

/**
 * @brief dynamic array value pushing function
 * 
 * @param[in,out] darr   dynamic array to push value to (non-null)
 * @param[in]     source data to push to array (non-null, at least elemSize bytes available for read)
 * 
 * @return operation status. in case if status isn't OK, stack conserved in previous state.
 */
CfDarrStatus cfDarrPush( CfDarr *darr, const void *src );

/**
 * @brief array to dynamic array pushing function
 * 
 * @param[in,out] darr   dynamic array to push value to (non-nulll)
 * @param[in]     arr    array to push (non-null, should have at least arrLen * elemSize bytes readable)
 * @param[in]     arrLen array length (in elements, may be zero)
 * 
 * @return operation status
 * @note in case of non-OK status, array is just conserved in the previous state.
 */
CfDarrStatus cfDarrPushArray( CfDarr *darr, const void *arr, size_t arrLen );

/**
 * @brief dynamic array popping function
 * 
 * @param[in,out] darr dynamic array to pop value from (non-null)
 * @param[out]    dst  popping destination (non-nulll, should have at least elemSize bytes writable)
 */
CfDarrStatus cfDarrPop( CfDarr *darr, void *dst );

/**
 * @brief dynamic array contents getting function
 * 
 * @param[in] darr dynamic array to get data of 
 * 
 * @return pointer to array data (non-null)
 */
void * cfDarrData( CfDarr darr );

/**
 * @brief dynamic array size getting function
 * 
 * @param[in] darr dynamic array to get size of
 * 
 * @return array size
 */
size_t cfDarrSize( CfDarr darr );

/**
 * @brief non-dynamic-sized array with exactly same data allocation function
 * 
 * @param[in] darr array to do operation with (non-null)
 * @param[in] dst  allocated array destination (non-null)
 * 
 * @return operation status
 * 
 * @note operation success doesn't means, that *dst is non-null,
 * because *dst is null then array size is zero.
 * @note *dst is ok to use with standard allocation functions.
 */
CfDarrStatus cfDarrIntoData( CfDarr darr, void **dst );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_DARR_H_)

// cf_darr.h
