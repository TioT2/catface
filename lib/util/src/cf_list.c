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
    size_t      length;         ///< list used element count

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
 * @brief list data element by index in data array accessing function
 * 
 * @param[in] list  list to access data array of (non-null)
 * @param[in] index index to access data array by ( > 0)
 * 
 * @return data element pointer
 */
static void * cfListGetElementByIndex( CfList list, const uint32_t index ) {
    assert(list != NULL);
    assert(index > 0);

    return (uint8_t *)cfListGetData(list) + list->elementSize * (index - 1);
} // cfListGetElementByIndex

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
    newImpl->length = impl->length;

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

/**
 * @brief list element insertion function
 * 
 * @param[in] list  list to insert element in
 * @param[in] data  data to insert
 * @param[in] index index to insert element after
 */
static CfListStatus cfListInsertAfter( CfList *list, const void *data, uint32_t index ) {
    assert(data != NULL);

    uint32_t freeIndex = cfListPopFree(list);
    if (freeIndex == 0)
        return CF_LIST_STATUS_INTERNAL_ERROR; // the only reason why popFree may fail, actually

    CfListImpl *impl = *list;

    // copy data
    memcpy(cfListGetElementByIndex(impl, freeIndex), data, impl->elementSize);

    impl->links[freeIndex].next = impl->links[index].next;
    impl->links[freeIndex].prev = index;

    impl->links[impl->links[index].next].prev = freeIndex;
    impl->links[index].next = freeIndex;

    impl->length++;

    return CF_LIST_STATUS_OK;
} // cfListInsertByIndex

/**
 * @brief list element by index popping function
 * 
 * @param[in,out] list  list to pop element from (non-null)
 * @param[out]    data  data to write popped element data to (nullable)
 * @param[in]     index index to pop element by
 * 
 * @return operation status
 */
static CfListStatus cfListPopAt( CfList *list, void *data, uint32_t index ) {
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    if (index == 0)
        return CF_LIST_STATUS_NO_ELEMENTS;

    const uint32_t prev = impl->links[index].prev;
    const uint32_t next = impl->links[index].next;

    impl->links[impl->links[index].next].prev = prev;
    impl->links[impl->links[index].prev].next = next;

    if (data != NULL) // ?
        memcpy(data, cfListGetElementByIndex(impl, index), impl->elementSize);

    impl->links[index].next = impl->freeStartIndex;
    impl->freeStartIndex = index;

    impl->length--;
    return CF_LIST_STATUS_OK;
} // cfListPopAt

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
    // uugh
    assert(data != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    return cfListInsertAfter(list, data, impl->links[0].prev);
} // cfListPushBack

CfListStatus cfListPopBack( CfList *list, void *data ) {
    // uugh
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    return cfListPopAt(list, data, impl->links[0].prev);
} // cfListPopBack

CfListStatus cfListPushFront( CfList *list, const void *data ) {
    assert(data != NULL);

    return cfListInsertAfter(list, data, 0);
} // cfListPushFront

CfListStatus cfListPopFront( CfList *list, void *data ) {
    assert(list != NULL);
    CfListImpl *impl = *list;
    assert(impl != NULL);

    return cfListPopAt(list, data, impl->links[0].next);
} // cfListPopFront

CfListIter cfListIterStart( CfList list ) {
    assert(list != NULL);

    const uint32_t index = list->links[0].next;

    return (CfListIter){
        .list = list,
        .index = index,
        .finished = index == 0,
    };
} // cfListIterStart

CfList cfListIterFinish( CfListIter *iter ) {
    assert(iter != NULL);

    return iter->list;
} // cfListIterFinish

void * cfListIterNext( CfListIter *const iter ) {
    assert(iter != NULL);
    assert(iter->list != NULL);

    if (iter->finished)
        return NULL;

    void *const data = cfListGetElementByIndex(iter->list, iter->index);
    const uint32_t nextIndex = iter->list->links[iter->index].next;

    // update finish flag
    iter->finished = (nextIndex == 0);

    // update index
    iter->index = nextIndex;

    return data;
} // cfListIterNext

void * cfListIterGet( const CfListIter *iter ) {
    assert(iter != NULL);
    assert(!iter->finished);

    return cfListGetElementByIndex(iter->list, iter->index);
} // cfListIterGet

CfListStatus cfListIterInsertAfter( CfListIter *iter, const void *data ) {
    assert(iter != NULL);
    assert(iter->list != NULL);
    assert(!iter->finished);

    return cfListInsertAfter(&iter->list, data, iter->index); // uugh
} // cfListIterInsertBefore

void cfListPrint( FILE *const out, CfList list, CfListElementDumpFn dumpElement ) {
    assert(list != NULL);

    uint32_t index = 0;
    uint32_t elementIndex = 0;

    for (;;) {
        const uint32_t nextIndex = list->links[index].next;

        if (nextIndex == 0)
            return;

        void *const data = cfListGetElementByIndex(list, nextIndex);

        fprintf(out, "%5d: ", elementIndex++);
        dumpElement(out, data);
        fprintf(out, "\n");

        index = nextIndex;
    }
} // cfListPrint

void cfListPrintDot( FILE *out, CfList list, CfListElementDumpFn desc ) {
    // declare digraph
    fprintf(out, "digraph {\n");
    fprintf(out, "    // set graph parameters\n");
    fprintf(out, "    node [shape=record];\n");
    fprintf(out, "    rankdir = LR;\n");
    fprintf(out, "\n\n");

    // describe list elements
    uint32_t index = 0;

    fprintf(out, "    // setup nodes and logical node connections\n");
    for (uint32_t logicalIndex = 0; logicalIndex <= list->length; logicalIndex++) {
        // read data
        void *data = index != 0
            ? cfListGetElementByIndex(list, index)
            : NULL;

        // The Node
        fprintf(out, "    node%d [label = \""
            "<index>'ring' index: %d|"
            "<LIndex> 'iter' index: %d|"
            "<ptr>data at 0x%08zX|"
            "<data> data preview: ",
            index,
            index,
            logicalIndex - 1,
            (size_t)data
        );

        if (data == NULL)
            fprintf(out, "[no data]");
        else {
            fprintf(out, "\\\"");
            desc(out, data);
            fprintf(out, "\\\"");
        }

        fprintf(out, "\"];\n");

        // setup connection between current and prev node
        if (index != 0)
            fprintf(out, "    node%d -> node%d [color = \"#0000FF\"];\n", index, list->links[index].prev);
        // setup connection between current and next node
        if (list->links[index].next != 0)
            fprintf(out, "    node%d -> node%d [color = \"#FF0000\"];\n", index, list->links[index].next);

        index = list->links[index].next;
    }
    fprintf(out, "\n\n");

    // add 'overweight' connections for straight list structure
    fprintf(out, "    // add invisible 'aligner' connections to make scheme linear\n");
    for (uint32_t i = 0; i < list->length; i++)
        fprintf(out, "    node%d:ptr -> node%d:ptr [weight = 1000; color = \"#0000FF00\"];\n", i, i + 1);
    fprintf(out, "\n\n");

    // close digraph
    fprintf(out, "}\n");
} // cfListPrintDot

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
