/**
 * @brief double-ended queue declaration file
 */

#ifndef CF_DEQUE_H_
#define CF_DEQUE_H_

#include "cf_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief arena-located list representaiton structure
typedef struct __CfDequeImpl * CfDeque;

/// @brief undefined chunk size
#define CF_DEQUE_CHUNK_SIZE_UNDEFINED (~(size_t)0)

/**
 * @brief deque constructor
 * 
 * @param[in] elementSize single deque element size ( > 0)
 * @param[in] chunkSize   chunk size ( > 0)
 * @param[in] arena       arena used to allocate deque chunks in (nullable)
 * 
 * @return constructed deque
 * 
 * @note
 * - CF_DEQUE_CHUNK_SIZE_UNDEFINED may be passed to *elementSize* parameter to denote that there is no difference for user in chunk size.
 * 
 * - Function may return NULL in case if smth went wrong.
 * 
 * - Function **do not** gets ownership on 'arena' value.
 */
CfDeque cfDequeCtor( size_t elementSize, size_t chunkSize, CfArena arena );

/**
 * @brief deque destructor
 * 
 * @param[in] deque deque to destroy (nullable)
 */
void cfDequeDtor( CfDeque deque );

/**
 * @brief deque size getting function
 * 
 * @param[in] deque deque to get size of
 * 
 * @return size of deque in elements
 */
size_t cfDequeSize( const CfDeque deque );

/**
 * @brief deque contents to contigious array copying function
 * 
 * @param[in]  deque deque to wrtie values of (non-null)
 * @param[out] dst   writing destination (non-null, at least elementSize * <deque size> bytes writable)
 */
void cfDequeWrite( const CfDeque deque, void *dst );

/**
 * @brief element to back pushing function
 * 
 * @param[in] deque deque to push element in (non-null)
 * @param[in] data  data to push to deque (non-null)
 * 
 * @return true if pushed, false if smth went wrong
 */
bool cfDequePushBack( CfDeque deque, const void *data );

/**
 * @brief element from back popping function
 * 
 * @param[in]  deque deque to pop element from (non-null)
 * @param[out] data  memory to pop into (nullable)
 * 
 * @return true if popped, false if not
 */
bool cfDequePopBack( CfDeque deque, void *data );

/**
 * @brief element to front pushing function
 * 
 * @param[in] deque deque to push element in (non-null)
 * @param[in] data  data to push to deque (non-null)
 * 
 * @return true if pushed, false if smth went wrong
 */
bool cfDequePushFront( CfDeque deque, const void *data );

/**
 * @brief element from front popping function
 * 
 * @param[in]  deque deque to pop element from (non-null)
 * @param[out] data  memory to pop into (nullable)
 * 
 * @return true if popped, false if not
 */
bool cfDequePopFront( CfDeque deque, void *data );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_DEQUE_H_)

// cf_DEQUE.h
