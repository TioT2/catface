/**
 * @brief double-ended queue implementation file
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cf_deque.h"

/// @brief single allocation chunk structure
typedef struct __CfDequeChunk CfDequeChunk;

// chunk structure implemetation
struct __CfDequeChunk {
    union {
        struct {
            CfDequeChunk * prev;     ///< previous chunk in chunk ring
            CfDequeChunk * next;     ///< next chunk in chunk ring
            bool           isPinned; ///< true if chunk is under cursor, false if not.
        };
        max_align_t _aligner; ///< force chunk alignment
    };

    char           data[1]; ///< chunk data array
}; // struct __CfDequeChunk

/// @brief pointer to deque edge structure
typedef struct __CfDequeCursor {
    CfDequeChunk * chunk; ///< chunk cursor points to
    size_t         index; ///< first free index
} CfDequeCursor;

/// @brief deque internal structure
typedef struct __CfDequeImpl {
    CfArena       arena;       ///< arena allocator (nullable, if null - all dealocation is handled manually)
    size_t        elementSize; ///< deque element size
    size_t        chunkSize;   ///< deque chunk size

    size_t        size;        ///< current deque size (in elements)
    CfDequeCursor front;       ///< cursor to deque front
    CfDequeCursor back;        ///< cursor to deque back
} CfDequeImpl;

/**
 * @brief deque chunk allocation function
 * 
 * @param[in] deque deque to allocate chunk in
 * 
 * @return allocated chunk. may be NULL.
 * 
 * @note it's ok to free chunk by 'free' standard function in case if deque->arena == NULL.
 */
static CfDequeChunk * cfDequeAllocChunk( CfDeque deque ) {
    assert(deque != NULL);

    size_t allocSize = sizeof(CfDequeChunk) + deque->chunkSize * deque->elementSize - 1;

    return deque->arena == NULL
        ? (CfDequeChunk *)calloc(allocSize, 1)
        : (CfDequeChunk *)cfArenaAlloc(deque->arena, allocSize);
} // cfDequeAllocChunk

CfDeque cfDequeCtor( size_t elementSize, size_t chunkSize, CfArena arena ) {
    assert(elementSize > 0);
    assert(chunkSize > 0);

    // allocate deque
    CfDequeImpl *deque = arena == NULL
        ? (CfDequeImpl *)calloc(1, sizeof(CfDequeImpl))
        : (CfDequeImpl *)cfArenaAlloc(arena, sizeof(CfDequeImpl));

    if (deque == NULL)
        return NULL;

    *deque = (CfDequeImpl) {
        .arena = arena,
        .elementSize = elementSize,
        .chunkSize = chunkSize == CF_DEQUE_CHUNK_SIZE_UNDEFINED
            ? 16
            : chunkSize,
    };

    CfDequeChunk *chunk = cfDequeAllocChunk(deque);

    if (chunk == NULL) {
        if (arena == NULL)
            free(deque);
        return NULL;
    }

    // bad practice, but IntelliSense is dying if I use names in this case
    *chunk = (CfDequeChunk) { chunk, chunk, true, };

    deque->front = (CfDequeCursor) { .chunk = chunk, .index = 0 };
    deque->back  = (CfDequeCursor) { .chunk = chunk, .index = 0 };

    return deque;
} // cfDequeCtor

void cfDequeDtor( CfDeque deque ) {
    // check for NULL
    if (deque == NULL)
        return;

    // actually, any actions should be performed only in case if memory is managed manually.
    if (deque->arena != NULL)
        return;

    CfDequeChunk *const first = deque->front.chunk;
    CfDequeChunk *current = first;

    for (;;) {
        CfDequeChunk *next = current->next;
        free(current);
        if (next == first)
            break;
        current = next;
    }

    // free deque itself
    free(deque);
} // cfDequeDtor

size_t cfDequeSize( const CfDeque deque ) {
    assert(deque != NULL);
    return deque->size;
} // cfDequeSize

void cfDequeWrite( const CfDeque deque, void *dst ) {
    assert(deque != NULL);
    assert(dst != NULL);

    // handle single-chunk case
    if (deque->front.chunk == deque->back.chunk) {
        memcpy(
            dst,
            deque->front.chunk->data + deque->front.index * deque->elementSize,
            (deque->back.index - deque->front.index) * deque->elementSize
        );
        return;
    }

    size_t index = 0;

    // handle front chunk
    memcpy(
        (char *)dst + index * deque->elementSize,
        deque->front.chunk->data + deque->front.index * deque->elementSize,
        (deque->chunkSize - deque->front.index) * deque->elementSize
    );

    index += deque->chunkSize - deque->front.index;

    // handle middle chunks
    CfDequeChunk *chunk = deque->front.chunk->next;

    while (chunk != deque->back.chunk) {
        memcpy(
            (char *)dst + index * deque->elementSize,
            chunk->data,
            deque->chunkSize * deque->elementSize
        );

        index += deque->chunkSize;
        chunk = chunk->next;
    }

    // handle back chunk
    memcpy(
        (char *)dst + index * deque->elementSize,
        deque->back.chunk->data,
        deque->back.index * deque->elementSize
    );
} // cfDequeWrite

bool cfDequePushBack( CfDeque deque, const void *data ) {
    assert(deque != NULL);
    assert(data != NULL);

    memcpy(
        deque->back.chunk->data + deque->back.index * deque->elementSize,
        data,
        deque->elementSize
    );

    if (deque->back.index >= deque->chunkSize - 1) {
        // move to next chunk or allocate new one
        CfDequeChunk *nextChunk = NULL;

        if (deque->back.chunk->next->isPinned) {
            // allocate new chunk and insert it to chunk ring
            nextChunk = cfDequeAllocChunk(deque);

            if (nextChunk == NULL)
                return false;

            // bad practice, but IntelliSense is dying if I use names in this case
            *nextChunk = (CfDequeChunk) { deque->back.chunk, deque->back.chunk->next };

            // conect chunk with list
            nextChunk->prev->next = nextChunk;
            nextChunk->next->prev = nextChunk;
        } else {
            // just use next chunk
            nextChunk = deque->back.chunk->next;
        }

        // remains pinned only if it's pinned by front cursor too.
        deque->back.chunk->isPinned = (deque->front.chunk == deque->back.chunk);

        nextChunk->isPinned = true;

        deque->back.chunk = nextChunk;
        deque->back.index = 0;
    } else {
        deque->back.index++;
    }

    return true;
} // cfDequePushBack

bool cfDequePopBack( CfDeque deque, void *data ) {
    assert(false && "Not implemented yet");
    return false;
} // cfDequePopBack

bool cfDequePushFront( CfDeque deque, const void *data ) {
    assert(deque != NULL);
    assert(data != NULL);

    // move cursor back
    if (deque->front.index <= 0) {
        CfDequeChunk *prevChunk = NULL;

        if (deque->front.chunk->prev->isPinned) {
            // allocate new chunk
            prevChunk = cfDequeAllocChunk(deque);

            if (prevChunk == NULL)
                return false;

            // initialize prevChunk
            *prevChunk = (CfDequeChunk) { deque->front.chunk->prev, deque->front.chunk };

            // connect prev chunk with ring
            prevChunk->next->prev = prevChunk;
            prevChunk->prev->next = prevChunk;
        } else {
            prevChunk = deque->front.chunk->prev;
        }

        // current chunk remains pinned only if it's pinned by back cursor too.
        deque->front.chunk->isPinned = (deque->back.chunk == deque->front.chunk);

        prevChunk->isPinned = true;

        deque->front.chunk = prevChunk;
        deque->front.index = deque->chunkSize - 1;
    } else {
        deque->front.index--;
    }

    // copy memory
    memcpy(
        deque->front.chunk->data + deque->front.index * deque->elementSize,
        data,
        deque->elementSize
    );

    return true;
} // cfDequePushFront

bool cfDequePopFront( CfDeque deque, void *data ) {
    assert(false && "Not implemented yet");
    return false;
} // cfDequePopFront

// cf_deque.c
