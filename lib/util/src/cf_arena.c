/**
 * @brief arena allocator utility implementation file
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cf_arena.h"

/// @brief arena allocation chunk forward declaration
typedef struct CfArenaChunk_ CfArenaChunk;

/// @brief arena chunk structure declaration
struct CfArenaChunk_ {
    // TODO: is it the best solution?
    union {
        struct {
            size_t         size; ///< chunk size
            CfArenaChunk * next; ///< next chunk in list
        };
        max_align_t _align; ///< max_align_t alignment forcer
    };

    uint8_t data[1]; ///< chunk memory (extends behind structure borders by at least size - 1 bytes)
}; // struct CfArenaChunk_

/**
 * @brief chunk allocation function
 * 
 * @param[in] size chunk data size ( > 0)
 * 
 * @return allocated arena chunk (may be null)
 * 
 * @note it's ok to free allocated chunk with standard library 'free' function
 */
static CfArenaChunk * cfArenaChunkAlloc( size_t size ) {
    assert(size > 0);

    CfArenaChunk *data = (CfArenaChunk *)calloc(sizeof(CfArenaChunk) + size, 1);

    if (data != NULL)
        data->size = size;
    return data;
} // cfArenaChunkAlloc

/**
 * @brief number up alignment function
 * 
 * @param[in] number    number to align
 * @param[in] alignment required alignment ( > 0)
 * 
 * @return aligned number
 */
static size_t cfArenaAlignUp( size_t number, size_t alignment ) {
    return (number / alignment + (size_t)(number % alignment != 0)) * alignment;
} // cfArenaAlignUp

/// @brief arena implementation structure declaration
typedef struct CfArenaImpl_ {
    CfArenaChunk * free;      ///< free stack top
    CfArenaChunk * firstUsed; ///< used stack top
    CfArenaChunk * lastUsed;  ///< used stack bottom (it is ok to read it's value only in case if firstUsed != NULL)
    size_t         chunkSize; ///< minimal chunk allocation size

    void *         currStart; ///< current chunk memory start
    void *         currEnd;   ///< current chunk memory end
} CfArenaImpl;

CfArena cfArenaCtor( size_t chunkSize ) {
    assert(chunkSize > 0);

    // set 1024 as default arena chunk size
    if (chunkSize == CF_ARENA_CHUNK_SIZE_UNDEFINED)
        chunkSize = 1024;

    CfArenaImpl *arena = (CfArenaImpl *)calloc(1, sizeof(CfArenaImpl));

    if (arena != NULL)
        arena->chunkSize = chunkSize;

    return arena;
} // cfArenaCtor

void cfArenaDtor( CfArena arena ) {
    if (arena == NULL)
        return;

    CfArenaChunk *chunk = arena->firstUsed == NULL
        ? arena->free
        : (arena->lastUsed->next = arena->free, arena->firstUsed) // comma operator usecase!
    ;

    while (chunk != NULL) {
        CfArenaChunk *next = chunk->next;
        free(chunk);
        chunk = next;
    }

    // free arena allocation itself
    free(arena);
} // cfArenaDtor

void * cfArenaAlloc( CfArena arena, size_t size ) {
    assert(arena != NULL);

    // any non-null pointer is considered good enough in case if zero-sized allocation wanted.
    if (size == 0)
        return (void *)~(size_t)0;

    uint8_t *allocation = (uint8_t *)cfArenaAlignUp((size_t)arena->currStart, sizeof(max_align_t));

    // if allocation from current chunk overflows arena
    // In case if currStart == NULL, NULL + size will be definetely >= NULL, so new chunk will be allocated.
    if (allocation + size > (uint8_t *)arena->currEnd) {
        size_t requiredChunkSize = arena->chunkSize >= size
            ? arena->chunkSize
            : size;

        CfArenaChunk *chunk = NULL;

        // check if it's possible to use already allocated chunk
        if (arena->free != NULL && arena->free->size >= requiredChunkSize) {
            // pop free chunk from free stack
            chunk = arena->free;
            arena->free = chunk->next;

            // zero chunk memory (free stack element data may be not zeroed.)
            memset(chunk->data, 0, chunk->size);
        } else {
            // allocate new chunk
            chunk = cfArenaChunkAlloc(requiredChunkSize);

            // check if allocation failed
            if (chunk == NULL)
                return NULL;

            // set lastUsed if there's no used elements at all
            if (arena->firstUsed == NULL)
                arena->lastUsed = chunk;
        }

        // chain chunk after top of arena->used stack, because it holds arena allocation itself
        chunk->next = arena->firstUsed;
        arena->firstUsed = chunk;

        // set new allocation range
        arena->currStart = chunk->data;
        arena->currEnd = chunk->data + chunk->size;

        // currStart is aligned by size of max_align_t by chunk structure definition
        allocation = (uint8_t *)arena->currStart;
    }

    arena->currStart = allocation + size;

    return allocation;
} // cfArenaAlloc

void cfArenaFree( CfArena arena ) {
    assert(arena != NULL);

    // check for chunks to free
    if (arena->firstUsed == NULL)
        return;

    // chain used and free stacks
    arena->lastUsed->next = arena->free;
    arena->free = arena->firstUsed;

    // remove chunks from used stack
    arena->firstUsed = NULL;
} // cfArenaFree

// cf_arena.c
