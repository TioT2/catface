/**
 * @brief arena allocator utility declaration file
 */

#ifndef CF_ARENA_H_
#define CF_ARENA_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief arena allocator handle representation structure
typedef struct CfArenaImpl_ * CfArena;

/// @brief value that may be passed to cfArenaCtor value to denote that there is no preferred chunk size
#define CF_ARENA_CHUNK_SIZE_UNDEFINED (~(size_t)0)

/**
 * @brief arena allocator constructor
 * 
 * @param[in] chunkSize default arena allocation size ( > 0)
 * 
 * @return newly created arena allocator (may be null in case if construction somehow failed)
 * 
 * @note CF_ARENA_CHUNK_SIZE_UNDEFINED value may be passed to *chunkSize*
 * parameter to denote that there is no preferred chunk size for user
 */
CfArena cfArenaCtor( size_t chunkSize );

/**
 * @brief arena allocator destructor
 * 
 * @param[in] arena arena to destroy (nullable)
 */
void cfArenaDtor( CfArena arena );

/**
 * @brief arena allocation function
 * 
 * @param[in] arena  arena to allocate memory from (non-null)
 * @param[in] size   size of memory chunk to be allocated
 * 
 * @return pointer to allocated memory region (may be null)
 * 
 * @note return value of this function is aligned by size of max_align_t
 * @note allocated memory is zeroed
 */
void * cfArenaAlloc( CfArena arena, size_t size );

/**
 * @brief **all** arena allocations free'ing function
 * 
 * @param[in] arena arena to free allocations in (non-null)
 * 
 * @note calling this function much is better, than
 * just destroying current and constructing new arena,
 * because this function reuses already allocated 
 * memory chunks.
 */
void cfArenaFree( CfArena arena );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_ARENA_H_)

// cf_arena.h
