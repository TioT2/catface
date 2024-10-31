/**
 * @brief bidirectional list utility implementation file
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "cf_list.h"

/// invalid list index
#define CF_LIST_INVALID_INDEX ((uint32_t)(~0U))

/// @brief list element links conatainer
typedef struct __CfListLinks {
    uint32_t prev; ///< previous element index
    uint32_t next; ///< next element index
} CfListLinks;

/// @brief list internal representation structure
typedef struct __CfListImpl {
    size_t   elementSize;    ///< list element size
    size_t   capacity;       ///< current list capacity

    uint32_t usedStartIndex; ///< used RING start index
    uint32_t freeStartIndex; ///< free STACK start index


    CfListLinks links[1];    ///< START of link array
} CfListImpl;

/**
 * @brief list data getting function
 * 
 * @param[in] list list to get data of
 * 
 * @return list data array pointer
 */
static void * cfListGetData( CfList list ) {
    return (CfListLinks *)(list + 1) + list->capacity;
} // cfListGetData

/**
 * @brief free index array region setting up funciton
 * 
 * @param[out] link       link array to setup region of
 * @param[in]  startIndex index
 * @param[in]  count      count of elements to setup
 */
static void cfListSetupFreeIndices(
    CfListLinks *const  free,
    const uint32_t      startIndex,
    const size_t        count
) {
    for (size_t i = 0; i < count; i++)
        free[startIndex + i].next = startIndex + i + 1;
    free[startIndex + count - 1].next = CF_LIST_INVALID_INDEX;
} // cfListSetupFreeIndices

/**
 * @brief list allocation function
 * 
 * @param[in] elementSize size of list element
 * @param[in] capacity    list capacity
 * 
 * @return list pointer, may be null
 * 
 * @note resulting pointer is allocated by stdlib functions, so it's ok to use free/realloc on it if needed.
 */
static CfList cfListAlloc( const size_t elementSize, const size_t capacity ) {
    CfListImpl * list = (CfListImpl *)calloc(
        sizeof(CfListImpl) + capacity * (elementSize + sizeof(CfListLinks)),
        sizeof(uint8_t)
    );

    if (list != NULL) {
        list->elementSize = elementSize;
        list->capacity = capacity;
    }

    return list;
} // cfListAlloc

/**
 * @brief list resize function
 * 
 * @param[in,out] list        list to resize pointer
 * @param[in]     newCapacity new list capacity
 * 
 * @return true if resized successfully, false otherwise
 */
static bool cfListResize( CfList *list, const size_t newCapacity ) {
    assert(list != NULL);

    CfListImpl *impl = *list;

    assert(impl != NULL);

    CfListImpl *newImpl = cfListAlloc(impl->elementSize, newCapacity);

    if (newImpl == NULL)
        return false;

    // copy links
    memcpy(
        newImpl->links,
        impl->links,
        impl->capacity * sizeof(CfListLinks)
    );

    // copy data
    memcpy(
        cfListGetData(newImpl),
        cfListGetData(impl),
        impl->capacity * impl->elementSize
    );

    // setup new indices
    cfListSetupFreeIndices(
        newImpl->links,
        impl->capacity,
        newImpl->capacity - impl->capacity
    );

    // chain new elements to free stack
    newImpl->links[newImpl->capacity - 1].next = impl->freeStartIndex;
    newImpl->freeStartIndex = impl->capacity;

    newImpl->usedStartIndex = impl->usedStartIndex;

    free(impl);
    *list = newImpl;

    return true;
} // cfListResize

/**
 * @brief free element from list popping function
 * 
 * @param[in,out] impl list to pop element from
 * 
 * @return free element index. In case if there is no such element, returns CF_LIST_INVALID_INDEX.
 * 
 * @note .next and .prev of link by resulting index contain trash values
 */
static uint32_t cfListPopFree( CfListImpl *const impl ) {
    assert(impl != NULL);

    if (impl->freeStartIndex == CF_LIST_INVALID_INDEX)
        return CF_LIST_INVALID_INDEX;

    const uint32_t result = impl->freeStartIndex;
    impl->freeStartIndex = impl->links[impl->freeStartIndex].next;

    return result;
} // cfListPopFree

/**
 * @brief list pushing function
 * 
 * @param[in,out] list  list to push free in
 * @param[in]     index index
 */
static void cfListPushFree( CfListImpl *const impl, const uint32_t index ) {
    assert(impl != NULL);

    impl->links[index].next = impl->freeStartIndex;
    impl->freeStartIndex = index;
} // cfListPushFree

CfList cfListCtor( const size_t elementSize, size_t initialCapacity ) {
    CfListImpl *list = cfListAlloc(elementSize, initialCapacity);

    if (list == NULL)
        return NULL;

    list->usedStartIndex = CF_LIST_INVALID_INDEX;
    list->freeStartIndex = 0;

    cfListSetupFreeIndices(list->links, 0, initialCapacity);

    return list;
} // cfListCtor

void cfListDtor( CfList list ) {
    free(list);
} //cfListDtor

void * cfListGetElement( const CfList list, uint32_t elementIndex ) {
    assert(list != NULL);

    if (elementIndex >= list->capacity)
        return NULL;
    
    uint32_t index = list->usedStartIndex;
    for (uint32_t i = 0; i < elementIndex; i++)
        index = list->links[list->usedStartIndex].next;

    return (uint8_t *)cfListGetData(list) + index * list->elementSize;
} // cfListGetElement

CfListStatus cfListPushBack( CfList *list, const void *data ) {
    assert(data != NULL);

    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    uint32_t dstIndex = cfListPopFree(impl);
    if (dstIndex == CF_LIST_INVALID_INDEX) {
        const size_t newCapacity = impl->capacity == 0 ? 1 : impl->capacity * 2;

        if (!cfListResize(list, newCapacity))
            return CF_LIST_STATUS_INTERNAL_ERROR;
        impl = *list;
        dstIndex = cfListPopFree(impl); // this function should not return CF_INVALID_INDEX in this case.
    }

    // copy data
    memcpy(
        (uint8_t *)cfListGetData(impl) + dstIndex * impl->elementSize,
        data,
        impl->elementSize
    );

    CfListLinks *links = impl->links;

    if (impl->usedStartIndex == CF_LIST_INVALID_INDEX) {
        // create unitary loop
        links[dstIndex].next = dstIndex;
        links[dstIndex].prev = dstIndex;

        impl->usedStartIndex = dstIndex;
    } else {
        // link last element with dst
        links[links[impl->usedStartIndex].prev].next = dstIndex;
        links[dstIndex].prev = links[impl->usedStartIndex].prev;

        // link first element with dst
        links[dstIndex].next = impl->usedStartIndex;
        links[impl->usedStartIndex].prev = dstIndex;
    }

    return CF_LIST_STATUS_OK;
} // cfListPushBack

CfListStatus cfListPopBack( CfList *list, void *data ) {
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    if (impl->usedStartIndex == CF_LIST_INVALID_INDEX)
        return CF_LIST_STATUS_NO_ELEMENTS;

    // CfListLinks *links = cfListGetLinks(impl);
    // uint64_t *listData = (uint64_t *)cfListGetData(impl);
    impl->usedStartIndex = impl->links[impl->usedStartIndex].prev;

    // quite bad implementation, actually
    return cfListPopFront(list, data);
} // cfListPopBack

CfListStatus cfListPushFront( CfList *list, const void *data ) {
    CfListStatus status = cfListPushBack(list, data);

    if (status == CF_LIST_STATUS_OK) {
        CfListImpl *impl = *list;

        impl->usedStartIndex = impl->links[impl->usedStartIndex].prev;
    }

    return status;
} // cfListPushFront

CfListStatus cfListPopFront( CfList *list, void *data ) {
    assert(list != NULL);
    CfListImpl *const impl = *list;
    assert(impl != NULL);

    if (impl->usedStartIndex == CF_LIST_INVALID_INDEX)
        return CF_LIST_STATUS_NO_ELEMENTS;

    CfListLinks *const links = impl->links;

    const uint32_t currIndex = impl->usedStartIndex;
    CfListLinks *const curr = links + impl->usedStartIndex;

    // detach current from used list
    if (curr->next != impl->usedStartIndex) {
        CfListLinks *const prev = links + curr->prev;
        CfListLinks *const next = links + curr->next;

        prev->next = curr->next;
        next->prev = curr->prev;

        impl->usedStartIndex = curr->next;
    } else {
        impl->usedStartIndex = CF_LIST_INVALID_INDEX;
    }

    // copy data
    memcpy(
        data,
        (uint8_t *)cfListGetData(impl) + currIndex * impl->elementSize,
        impl->elementSize
    );

    cfListPushFree(impl, currIndex);

    return CF_LIST_STATUS_OK;
} // cfListPopFront

CfListIterator cfListIter( CfList list ) {
    assert(list != NULL);

    CfListIterator result = {
        .list = list,
        .index = list->usedStartIndex,
        .finished = list->usedStartIndex == CF_LIST_INVALID_INDEX,
    };

    return result;
} // cfListIter

void * cfListIterNext( CfListIterator *const iter ) {
    assert(iter != NULL);
    assert(iter->list != NULL);

    if (iter->finished)
        return NULL;

    void *const data = (uint8_t *)cfListGetData(iter->list) + iter->list->elementSize * iter->index;
    const uint32_t nextIndex = iter->list->links[iter->index].next;

    // update finish flag
    iter->finished = (nextIndex == iter->list->usedStartIndex);

    // update index
    iter->index = nextIndex;

    return data;
} // cfListIterNext

void cfListPrint( FILE *const out, CfList list, CfListElementDumpFn dumpElement ) {
    uint32_t listIndex = list->usedStartIndex;
    uint32_t index = 0;
    uint8_t *data = (uint8_t *)cfListGetData(list);

    if (listIndex == CF_LIST_INVALID_INDEX) {
        fprintf(out, "<empty>\n");
        return;
    }

    for (;;) {
        fprintf(out, "%5d: ", index++);
        dumpElement(out, data + listIndex * list->elementSize);
        fprintf(out, "\n");

        uint32_t next = list->links[listIndex].next;
        if (next == list->usedStartIndex)
            return;
        listIndex = next;
    }
} // cfListPrint

bool cfListDbgCheckPrevNext( CfList list ) {
    uint32_t index = list->usedStartIndex;

    if (index == CF_LIST_INVALID_INDEX)
        return true; // nothing to check, actually

    for (;;) {
        uint32_t next = list->links[index].next;
        uint32_t nextPrev = list->links[next].prev;

        if (nextPrev != index)
            return false;

        if (next == list->usedStartIndex)
            return true;
        index = next;
    }
} // cfListDbgCheckPrevNext

// cf_list.c
