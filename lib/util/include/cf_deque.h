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
typedef struct CfDeque_ CfDeque;

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
 * - Resulting deque **do not** gets ownership on 'arena' value.
 * 
 * - In case if 'arena' is not NULL, deque constructor may not be called.
 */
CfDeque * cfDequeCtor( size_t elementSize, size_t chunkSize, CfArena *arena );

/**
 * @brief deque destructor
 * 
 * @param[in] deque deque to destroy (nullable)
 */
void cfDequeDtor( CfDeque *deque );

/**
 * @brief deque length getting function
 * 
 * @param[in] deque deque to get length of
 * 
 * @return count of elements located in deque
 */
size_t cfDequeLength( const CfDeque *deque );

/**
 * @brief deque contents to contigious array copying function
 * 
 * @param[in]  deque deque to wrtie values of (non-null)
 * @param[out] dst   writing destination (non-null, at least elementSize * <deque size> bytes writable)
 */
void cfDequeWrite( const CfDeque *deque, void *dst );

/**
 * @brief element to back pushing function
 * 
 * @param[in] deque deque to push element in (non-null)
 * @param[in] data  data to push to deque (non-null)
 * 
 * @return true if pushed, false if smth went wrong
 */
bool cfDequePushBack( CfDeque *deque, const void *data );

/**
 * @brief array to front pushing function
 * 
 * @param[in] deque       deque to push array to (non-null)
 * @param[in] array       array to push (non-null if arrayLength > 0)
 * @param[in] arrayLength array length
 */
bool cfDequePushArrayBack( CfDeque *deque, const void *array, size_t arrayLength );

/**
 * @brief element from back popping function
 * 
 * @param[in]  deque deque to pop element from (non-null)
 * @param[out] data  memory to pop into (nullable)
 * 
 * @return true if popped, false if not
 */
bool cfDequePopBack( CfDeque *deque, void *data );

/**
 * @brief element to front pushing function
 * 
 * @param[in] deque deque to push element in (non-null)
 * @param[in] data  data to push to deque (non-null)
 * 
 * @return true if pushed, false if smth went wrong
 */
bool cfDequePushFront( CfDeque *deque, const void *data );

/**
 * @brief element from front popping function
 * 
 * @param[in]  deque deque to pop element from (non-null)
 * @param[out] data  memory to pop into (nullable)
 * 
 * @return true if popped, false if not
 */
bool cfDequePopFront( CfDeque *deque, void *data );

/**
 * @brief get element at deque back getting
 * 
 * @param[in] deque deque to get element from (non-null)
 * 
 * @return pointer to element (non-null if deque is not empty)
 */
void * cfDequeGetBack( CfDeque *deque );

/**
 * @brief get element at deque front
 * 
 * @param[in] deque deque to get element from (non-null)
 * 
 * @return pointer to element (non-null if deque is not empty)
 */
void * cfDequeGetFront( CfDeque *deque );

/**
 * @brief element by index getting function
 */
void * cfDequeGetByIndex( CfDeque *deque );

/**
 * @brief cursor
 * 
 * @note cursor deque MUST NOT be modified (except destroyed) while cursor exist
 */
typedef struct CfDequeCursor_ {
    CfDeque              * deque; ///< deque itself
    struct CfDequeChunk_ * chunk; ///< chunk to 
    size_t                 index; ///< index
} CfDequeCursor;

/**
 * @brief get deque front cursor
 * 
 * @param[in] deque  deque to get       (non-null)
 * @param[in] cursor cursor destination (non-null)
 * 
 * @return true if got, false if not
 * 
 * @note cursor is possible to get only if deque is not empty.
 * @note initially, cursor points on deque front
 */
bool cfDequeFrontCursor( CfDeque *deque, CfDequeCursor *cursor );

/**
 * @brief get deque back cursor
 * 
 * @param[in] deque  deque to get       (non-null)
 * @param[in] cursor cursor destination (non-null)
 * 
 * @return true if got, false if not
 * 
 * @note cursor is possible to get only if deque is not empty.
 * @note initially, cursor points on deque back
 */
bool cfDequeBackCursor( CfDeque *deque, CfDequeCursor *cursor );

/**
 * @brief cursor destructor
 * 
 * @param[in] cursor cursor to destroy (non-null, initialized)
 */
void cfDequeCursorDtor( CfDequeCursor *cursor );

/**
 * @brief value at current point getting function
 * 
 * @param[in] cursor cursor to get value on (non-null)
 * 
 * @return value under cursor (non-null)
 */
void * cfDequeCursorGet( CfDequeCursor *cursor );

/**
 * @brief cursor advancing function
 * 
 * @param[in] cursor cursor (non-null)
 * @param[in] offset offset to advance iter on
 * 
 * @return true if advanced, false if advancing is impossible
 */
bool cfDequeCursorAdvance( CfDequeCursor *cursor, ptrdiff_t offset );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_DEQUE_H_)

// cf_DEQUE.h
