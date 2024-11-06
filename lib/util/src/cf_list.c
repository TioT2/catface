/**
 * @brief bidirectional list utility implementation file
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "cf_list.h"

/// @brief list element links conatainer
typedef struct __CfListLinks {
    uint32_t prev; ///< previous element index
    uint32_t next; ///< next element index
} CfListLinks;

/// @brief list internal representation structure
typedef struct __CfListImpl {
    size_t      elementSize;    ///< list element size
    size_t      capacity;       ///< current list capacity
    size_t      length;         ///< list length

    uint32_t    freeStartIndex; ///< free STACK start index

    CfListLinks links[1];       ///< START of link array
} CfListImpl;

/**
 * @brief list data getting function
 * 
 * @param[in] list list to get data of (non-null)
 * 
 * @return list data array pointer
 */
static void * cfListGetData( CfList list ) {
    return (CfListLinks *)(list + 1) + list->capacity;
} // cfListGetData

/**
 * @brief list data array accessing function
 * 
 * @param[in] list  list to access data array of (non-null)
 * @param[in] index index to access data array by ( > 0)
 * 
 * @return element pointer
 */
static void * cfListDataArrayAccess( CfList list, uint32_t index ) {
    assert(list != NULL);
    assert(index > 0);

    return (uint8_t *)cfListGetData(list) + list->elementSize * (index - 1);
} // cfListDataArrayAccess

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
    free[startIndex + count - 1].next = 0;
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
 * @param[in]     newCapacity new list capacity ( > current list capacity)
 * 
 * @return true if resized successfully, false otherwise
 */
static bool cfListResize( CfList *list, const size_t newCapacity ) {
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);
    assert(newCapacity > impl->capacity);

    CfListImpl *newImpl = cfListAlloc(impl->elementSize, newCapacity);

    if (newImpl == NULL)
        return false;

    // copy links
    memcpy(
        newImpl->links,
        impl->links,
        (impl->capacity + 1) * sizeof(CfListLinks)
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
        impl->capacity + 1,
        newImpl->capacity - impl->capacity
    );

    // chain new elements to free stack
    newImpl->links[newImpl->capacity].next = impl->freeStartIndex;
    newImpl->freeStartIndex = impl->capacity + 1;

    free(impl);
    *list = newImpl;

    return true;
} // cfListResize

/**
 * @brief free element from list popping function
 * 
 * @param[in,out] impl list to pop element from
 * 
 * @return free element index. In case if there is no such element, returns 0.
 * 
 * @note .next and .prev of link by resulting index contain trash values
 * @note the only reason this function may fail is internal list error.
 */
static uint32_t cfListPopFree( CfList *const list ) {
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    if (impl->freeStartIndex == 0) {
        const size_t newCapacity = impl->capacity == 0 ? 1 : impl->capacity * 2;

        if (!cfListResize(list, newCapacity))
            return 0;
        impl = *list;
    }

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

    if (initialCapacity > 0) {
        cfListSetupFreeIndices(list->links, 1, initialCapacity - 1);
        list->freeStartIndex = 1;
    }

    return list;
} // cfListCtor

void cfListDtor( CfList list ) {
    free(list);
} //cfListDtor

CfListStatus cfListPushBack( CfList *list, const void *data ) {
    assert(data != NULL);

    uint32_t dstIndex = cfListPopFree(list);
    if (dstIndex == 0)
        return CF_LIST_STATUS_INTERNAL_ERROR; // see popFree doc

    CfListImpl *impl = *list;

    // copy data
    memcpy(cfListDataArrayAccess(impl, dstIndex), data, impl->elementSize);

    // link last element with dst
    impl->links[dstIndex].prev = impl->links[0].prev;
    impl->links[dstIndex].next = 0;

    impl->links[impl->links[0].prev].next = dstIndex;
    impl->links[0].prev = dstIndex;

    impl->length++;

    return CF_LIST_STATUS_OK;
} // cfListPushBack

CfListStatus cfListPopBack( CfList *list, void *data ) {
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    const uint32_t elem = impl->links[0].prev;

    if (elem == 0)
        return CF_LIST_STATUS_NO_ELEMENTS;

    impl->links[impl->links[elem].prev].next = 0;
    impl->links[0].prev = impl->links[elem].prev;

    if (data != NULL) // ?
        memcpy(data, cfListDataArrayAccess(impl, elem), impl->elementSize);

    impl->links[elem].next = impl->freeStartIndex;
    impl->freeStartIndex = elem;

    impl->length--;

    return CF_LIST_STATUS_OK;
} // cfListPopBack

CfListStatus cfListPushFront( CfList *list, const void *data ) {
    assert(data != NULL);

    uint32_t dstIndex = cfListPopFree(list);
    if (dstIndex == 0)
        return CF_LIST_STATUS_INTERNAL_ERROR; // the only reason why popFree may fail, actually

    CfListImpl *impl = *list;

    // copy data
    memcpy(cfListDataArrayAccess(impl, dstIndex), data, impl->elementSize);

    // link last element with dst
    impl->links[dstIndex].next = impl->links[0].next;
    impl->links[dstIndex].prev = 0;

    impl->links[impl->links[0].next].prev = dstIndex;
    impl->links[0].next = dstIndex;

    impl->length++;

    return CF_LIST_STATUS_OK;
} // cfListPushFront

CfListStatus cfListPopFront( CfList *list, void *data ) {
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    const uint32_t elem = impl->links[0].next;

    if (elem == 0)
        return CF_LIST_STATUS_NO_ELEMENTS;

    impl->links[impl->links[elem].next].prev = 0;
    impl->links[0].next = impl->links[elem].next;

    if (data != NULL) // ?
        memcpy(data, cfListDataArrayAccess(impl, elem), impl->elementSize);

    impl->links[elem].next = impl->freeStartIndex;
    impl->freeStartIndex = elem;

    impl->length--;

    return CF_LIST_STATUS_OK;
} // cfListPopFront

CfListIterator cfListIter( CfList list ) {
    assert(list != NULL);

    const uint32_t index = list->links[0].next;

    return (CfListIterator){
        .list = list,
        .index = index,
        .finished = index == 0,
    };
} // cfListIter

void * cfListIterNext( CfListIterator *const iter ) {
    assert(iter != NULL);
    assert(iter->list != NULL);

    if (iter->finished)
        return NULL;

    void *const data = cfListDataArrayAccess(iter->list, iter->index);
    const uint32_t nextIndex = iter->list->links[iter->index].next;

    // update finish flag
    iter->finished = (nextIndex == 0);

    // update index
    iter->index = nextIndex;

    return data;
} // cfListIterNext

void cfListPrint( FILE *const out, CfList list, CfListElementDumpFn dumpElement ) {
    assert(list != NULL);

    uint32_t index = 0;
    uint32_t elementIndex = 0;

    for (;;) {
        const uint32_t nextIndex = list->links[index].next;

        if (nextIndex == 0)
            return;

        void *const data = cfListDataArrayAccess(list, nextIndex);

        fprintf(out, "%5d: ", elementIndex++);
        dumpElement(out, data);
        fprintf(out, "\n");

        index = nextIndex;
    }
} // cfListPrint

size_t cfListLength( CfList list ) {
    assert(list != NULL);

    return list->length;
} // cfListLength

bool cfListDbgCheckPrevNext( CfList list ) {
    assert(list != NULL);

    uint32_t index = 0;

    for (;;) {
        const uint32_t nextIndex = list->links[index].next;
        const uint32_t nextPrevIndex = list->links[nextIndex].prev;

        if (index != nextPrevIndex)
            return false;

        if (nextIndex == 0)
            return true;

        index = nextIndex;
    }
} // cfListDbgCheckPrevNext

// cf_list.c
