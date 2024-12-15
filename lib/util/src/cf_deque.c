/**
 * @brief double-ended queue implementation file
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cf_deque.h"

/// @brief single allocation chunk structure
typedef struct CfDequeChunk_ CfDequeChunk;

// chunk structure implemetation
struct CfDequeChunk_ {
    union {
        struct {
            CfDequeChunk * prev;     ///< previous chunk in chunk ring
            CfDequeChunk * next;     ///< next chunk in chunk ring
            bool           isPinned; ///< true if chunk is under cursor, false if not.
        };
        max_align_t _aligner; ///< force chunk alignment
    };

    char           data[1]; ///< chunk data array
}; // struct CfDequeChunk_

/// @brief pointer to deque edge structure
typedef struct CfDequeBorderCursor_ {
    CfDequeChunk * chunk; ///< chunk cursor points to
    size_t         index; ///< first free index
} CfDequeBorderCursor;

/// @brief deque internal structure
struct CfDeque_ {
    CfArena             arena;       ///< arena allocator (nullable, if null - all dealocation is handled manually)
    size_t              elementSize; ///< deque element size
    size_t              chunkSize;   ///< deque chunk size

    size_t              size;        ///< current deque size (in elements)
    CfDequeBorderCursor front;       ///< cursor to deque front
    CfDequeBorderCursor back;        ///< cursor to deque back
};

/**
 * @brief deque chunk allocation function
 * 
 * @param[in] deque deque to allocate chunk in
 * 
 * @return allocated chunk. may be NULL.
 * 
 * @note it's ok to free chunk by 'free' standard function in case if deque->arena == NULL.
 */
static CfDequeChunk * cfDequeAllocChunk( CfDeque *deque ) {
    assert(deque != NULL);

    size_t allocSize = sizeof(CfDequeChunk) + deque->chunkSize * deque->elementSize;

    return deque->arena == NULL
        ? (CfDequeChunk *)calloc(allocSize, 1)
        : (CfDequeChunk *)cfArenaAlloc(deque->arena, allocSize);
} // cfDequeAllocChunk

CfDeque *cfDequeCtor( size_t elementSize, size_t chunkSize, CfArena arena ) {
    assert(elementSize > 0);
    assert(chunkSize > 0);

    // allocate deque
    CfDeque *deque = arena == NULL
        ? (CfDeque *)calloc(1, sizeof(CfDeque))
        : (CfDeque *)cfArenaAlloc(arena, sizeof(CfDeque));

    if (deque == NULL)
        return NULL;

    *deque = (CfDeque) {
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

    deque->front = (CfDequeBorderCursor) { .chunk = chunk, .index = 0 };
    deque->back  = (CfDequeBorderCursor) { .chunk = chunk, .index = 0 };

    return deque;
} // cfDequeCtor

void cfDequeDtor( CfDeque *deque ) {
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

size_t cfDequeLength( const CfDeque *deque ) {
    assert(deque != NULL);
    return deque->size;
} // cfDequeLength

void cfDequeWrite( const CfDeque *deque, void *dst ) {
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

bool cfDequePushBack( CfDeque *deque, const void *data ) {
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

    deque->size++;
    return true;
} // cfDequePushBack

bool cfDequePopBack( CfDeque *deque, void *data ) {
    assert(deque != NULL);

    // check case where this is no elements to pop
    if (deque->back.chunk == deque->front.chunk && deque->back.index == deque->front.index)
        return false;

    // no elements to pop
    if (deque->back.index <= 0) {
        deque->back.chunk->isPinned = false;
        deque->back.chunk = deque->back.chunk->prev;
        deque->back.chunk->isPinned = true;
        deque->back.index = deque->chunkSize - 1;
    } else {
        deque->back.index--;
    }

    if (data != NULL)
        memcpy(
            data,
            deque->back.chunk->data + deque->back.index * deque->elementSize,
            deque->elementSize
        );

    deque->size--;
    return true;
} // cfDequePopBack

bool cfDequePushFront( CfDeque *deque, const void *data ) {
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

    deque->size++;
    return true;
} // cfDequePushFront

bool cfDequePopFront( CfDeque *deque, void *data ) {
    assert(deque != NULL);

    // check case where this is no elements to pop
    if (deque->front.chunk == deque->back.chunk && deque->front.index == deque->back.index)
        return false;

    if (data != NULL)
        memcpy(
            data,
            deque->front.chunk->data + deque->front.index * deque->elementSize,
            deque->elementSize
        );

    // no elements to pop from current chunk
    if (deque->front.index >= deque->chunkSize - 1) {
        deque->front.chunk->isPinned = false;
        deque->front.chunk = deque->front.chunk->next;
        deque->front.chunk->isPinned = true;
        deque->front.index = 0;
    } else {
        deque->front.index++;
    }

    deque->size--;
    return true;
} // cfDequePopFront


bool cfDequeFrontCursor( CfDeque *deque, CfDequeCursor *cursor ) {
    assert(deque != NULL);
    assert(cursor != NULL);

    if (deque->size == 0)
        return false;

    *cursor = (CfDequeCursor) {
        .deque = deque,
        .chunk = deque->front.chunk,
        .index = deque->front.index,
    };

    return true;
} // cfDequeFrontCursor

bool cfDequeBackCursor( CfDeque *deque, CfDequeCursor *cursor ) {
    assert(deque != NULL);
    assert(cursor != NULL);

    if (deque->size == 0)
        return false;

    cursor->deque = deque;

    *cursor = deque->back.index == 0
        ? (CfDequeCursor) {
            .deque = deque,
            .chunk = deque->back.chunk->prev,
            .index = deque->chunkSize - 1,
        }
        : (CfDequeCursor) {
            .deque = deque,
            .chunk = deque->back.chunk,
            .index = deque->back.index - 1,
        }
    ;

    return true;
} // cfDequeBackCursor

void * cfDequeCursorGet( CfDequeCursor *cursor ) {
    assert(cursor != NULL);

    return cursor->chunk->data + cursor->index * cursor->deque->elementSize;
} // cfDequeCursorGet

bool cfDequeCursorAdvance( CfDequeCursor *cursor, ptrdiff_t offset ) {
    assert(cursor != NULL);

    ptrdiff_t totalOffset = offset + cursor->index;
    CfDequeChunk *current = cursor->chunk;

    if (totalOffset > 0) {
        while (totalOffset >= cursor->deque->chunkSize) {
            if (current->isPinned)
                return false;

            current = current->next;
            totalOffset -= cursor->deque->chunkSize;
        }

        if (totalOffset >= cursor->deque->back.index)
            return false;
    }
    else {
        while (totalOffset < 0) {
            if (current->isPinned)
                return false;

            current = current->prev;
            totalOffset += cursor->deque->chunkSize;
        }

        if (totalOffset < cursor->deque->front.index)
            return false;
    }

    cursor->chunk = current;
    cursor->index = totalOffset;

    return true;
} // cfDequeCursorAdvance

void cfDequeCursorDtor( CfDequeCursor *cursor ) {
    assert(cursor != NULL);

    // currently, does nothing
} // cfDequeCursorDtor

// cf_deque.c
