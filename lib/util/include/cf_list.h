/**
 * @brief bidirectional list utility declaration file
 */

#ifndef CF_LIST_H_
#define CF_LIST_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief bidirectoinal list handle
typedef struct __CfListImpl * CfList;

/// @brief some list operation status
typedef enum __CfListStatus {
    CF_LIST_STATUS_OK,             ///< operation succeeded
    CF_LIST_STATUS_INTERNAL_ERROR, ///< list internal error occured
    CF_LIST_STATUS_NO_ELEMENTS,    ///< no elements to pop from
} CfListStatus;

/**
 * @brief list constructor
 * 
 * @param[in] elementSize     size of single list element
 * @param[in] initialCapacity initial list capacity
 * 
 * @return list handle. May be NULL if smth went wrong.
 */
CfList cfListCtor( size_t elementSize, size_t initialCapacity );

/**
 * @brief list destructor
 * 
 * @param[in] list pointer (may be null)
 */
void cfListDtor( CfList list );

/**
 * @brief list element getting function
 * 
 * @param[in] list         list to get element from (non-null)
 * @param[in] elementIndex index of element
 * 
 * @return pointer of list element. if such elementIndex is more than list capacity, NULL returned.
 */
void * cfListGetElement( CfList list, uint32_t elementIndex );

/**
 * @brief value to list end pushing function
 * 
 * @param[in,out] list list to push value to pointer (non-null)
 * @param[in]     data data to push (should have at least elementSize bytes readable)
 * 
 * @return operation status
 */
CfListStatus cfListPushBack( CfList *list, const void *data );

/**
 * @brief value from list end popping function
 * 
 * @param[in,out] list list to pop value from pointer (non-null)
 * @param[in]     data data popping destination (should have at least elementSize bytes writable)
 * 
 * @return operation status
 */
CfListStatus cfListPopBack( CfList *list, void *data );

/**
 * @brief value to list start pushing function
 * 
 * @param[in,out] list list to push value to pointer (non-null)
 * @param[in]     data data to push (should have at least elementSize bytes readable)
 * 
 * @return operation status
 */
CfListStatus cfListPushFront( CfList *list, const void *data );

/**
 * @brief value from list start popping function
 * 
 * @param[in,out] list list to pop value from pointer (non-null)
 * @param[in]     data data popping destination (should have at least elementSize bytes writable)
 * 
 * @return operation status
 */
CfListStatus cfListPopFront( CfList *list, void *data );

/// @brief list iterator container.
/// @note this structure **must not** be built by user.
typedef struct __CfListIterator {
    CfList   list;     ///< iterated list
    uint32_t index;    ///< index
    bool     finished; ///< iteration finished if set to true
} CfListIterator;

/**
 * @brief list iterator getting function
 * 
 * @param[in] list list to get iterator on (non-null)
 * 
 * @return iterator on current list
 * 
 * @note iterator remains valid while list itself is not modified.
 */
CfListIterator cfListIter( CfList list );

/**
 * @brief iterator next element getting function
 * 
 * @param[in] iter iterator to get next element from (initialized by cfListIter result)
 * 
 * @return pointer to element if it exists, NULL if not (a.k.a. iteration finished)
 */
void * cfListIterNext( CfListIterator *iter );

/**
 * @brief element dumping function pointer
 * 
 * @param[in,out] file    file to dump element to
 * @param[in]     element element to dump
 */
typedef void (* CfElementDumpFn)( FILE *out, void *element );

/**
 * @brief list dumping function
 * 
 * @param[in] out  file to dump list to
 * @param[in] list list to dump (non-null)
 * @param[in] dump element dumping function (nullable)
 */
void cfListDump( FILE *out, CfList list, CfElementDumpFn dumpElement );


/***
 * Debugging-related functions. Actually, may be removed.
 ***/


/**
 * @brief prev-next match checking function
 * 
 * @param[in] list list to perform check in
 * 
 * @return true if ok, false if not
 */
bool cfListDbgCheckPrevNext( CfList list );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_LIST_H_)

// cf_list.h
