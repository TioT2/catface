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
 * @param[in] list pointer (nullable)
 */
void cfListDtor( CfList list );

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
 * @param[in]     data data popping destination (nullable, should have at least elementSize bytes writable if not null)
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
 * @param[in]     data data popping destination (nullable, should have at least elementSize bytes writable if not null)
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
} CfListIter;

/**
 * @brief list iterator getting function
 * 
 * @param[in] list list to get iterator on (non-null)
 * 
 * @return iterator on current list
 * 
 * @note iter structure takes list ownership
 */
CfListIter cfListIterStart( CfList list );

/**
 * @brief list iteration finishing function
 * 
 * @param[in] iter iterator pointer
 * 
 * @return iterated list (function returns list ownership to user)
 */
CfList cfListIterFinish( CfListIter *iter );

/**
 * @brief iterator next element getting function
 * 
 * @param[in] iter iterator to get next element from (initialized by cfListIterStart result)
 * 
 * @return pointer to element if it exists, NULL if not (a.k.a. iteration finished)
 */
void * cfListIterNext( CfListIter *iter );

/**
 * @brief data by iterator getting function
 * 
 * @param[in] iter iterator to get data from
 * 
 * @return underlying data
 */
void * cfListIterGet( const CfListIter *iter );

/**
 * @brief by iterator insertion function
 * 
 * @param[in] iter iterator (non-null)
 * @param[in] data data to insert (non-null)
 * 
 * @return operation status
 */
CfListStatus cfListIterInsertAfter( CfListIter *iter, const void *data );

/**
 * @brief by iteration insertion function
 * 
 * @param[in] iter iterator (non-null)
 * @param[in] data data to insert (non-null)
 * 
 * @return operation status
 */
CfListStatus cfListIterInsertBefore( CfListIter *iter, const void *data );

/**
 * @brief removing at iterator function
 * 
 * @param[in] iter iterator (non-nulll)
 * @param[in] data data destination (nullable)
 * 
 * @return operation status
 * 
 * @note iterator jumps to next element
 */
CfListStatus cfListIterRemoveAt( CfListIter *iter, void *data );

/**
 * @brief element dumping function pointer
 * 
 * @param[in,out] file    file to dump element to
 * @param[in]     element element to dump
 * 
 * @note callback of this type may be used for binary and for text dumping too.
 */
typedef void (* CfListElementDumpFn)( FILE *out, const void *element );

/**
 * @brief list printig function
 * 
 * @param[in] out   file to dump list to
 * @param[in] list  list to dump (non-null)
 * @param[in] print element printing function (nullable)
 */
void cfListPrint( FILE *out, CfList list, CfListElementDumpFn print );

/**
 * @brief list length getting function
 * 
 * @param[in] list list handle (non-null)
 * 
 * @return current list length
 */
size_t cfListLength( CfList list );

/***
 * Debugging-related functions. Actually, may be removed.
 ***/

/**
 * @brief list in dot format printing function
 * 
 * @param[in] out  output file
 * @param[in] list list to dump(non-null)
 * @param[in] desc short element description printing function (nullable)
 * 
 * @note this function is dbg because it displays list's internals
 */
void cfDbgListPrintDot( FILE *out, CfList list, CfListElementDumpFn desc );

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
