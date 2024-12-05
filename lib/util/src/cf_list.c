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

CfListStatus cfListIterInsertBefore( CfListIter *iter, const void *data ) {
    assert(iter != NULL);
    assert(iter->list != NULL);
    assert(!iter->finished);

    return cfListInsertAfter(&iter->list, data, iter->list->links[iter->index].prev); // uugh
} // cfListIterInsertBefore

CfListStatus cfListIterRemoveAt( CfListIter *iter, void *data ) {
    assert(iter != NULL);
    assert(iter->list != NULL);
    assert(!iter->finished);

    const uint32_t newIndex = iter->list->links[iter->index].next;
    const CfListStatus status = cfListPopAt(&iter->list, data, iter->index);

    if (status == CF_LIST_STATUS_OK)
        iter->index = newIndex;
    return status;
} // cfListIterRemoveAt

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

/**
 * @brief node in dot format dumping function
 * 
 * @param[in,out] out       output file
 * @param[in]     nodeIndex node to dump index
 * @param[in]     nodeData  node data
 * @param[in]     desc      short text node description getting function
 * @param[in]     isDeleted true if node is located in free list, false if not
 */
static void cfListPrintDotDumpNode(
    FILE                * out,
    uint32_t              nodeIndex,
    CfList                list,
    CfListElementDumpFn   desc,
    bool                  isDeleted
) {
    void * nodeData = nodeIndex == 0
        ? NULL
        : cfListGetElementByIndex(list, nodeIndex)
    ;
    CfListLinks nodeLinks = list->links[nodeIndex];

    // The Node
    fprintf(out,
        "    node%d [color = \"%s\" label = \""
        "<index>index: %d|"
        "<ptr>data location 0x%016zX",
        nodeIndex,
        nodeIndex == 0
            ? "forestgreen"
            : isDeleted
                ? "black"
                : "green",
        nodeIndex,
        // nodeLogicalIndex - 1,
        (size_t)nodeData
    );

    // don't display data for deleted nodes
    if (!isDeleted) {
        fprintf(out, "|<data> data preview: ");
        if (nodeData == NULL)
            fprintf(out, "[no data]");
        else {
            fprintf(out, "\\\"");
            desc(out, nodeData);
            fprintf(out, "\\\"");
        }
    }

    fprintf(out, "|{<prev>prev: %d | <next>next: %d}", nodeLinks.prev, nodeLinks.next);

    fprintf(out, "\"];\n");
} // cfListPrintDotDumpNode

void cfDbgListPrintDot( FILE *out, CfList list, CfListElementDumpFn desc ) {
    // declare digraph
    fprintf(out, "digraph {\n");
    fprintf(out, "    // set graph parameters\n");
    fprintf(out, "    node [shape = record; penwidth = 3];\n");
    fprintf(out, "    rankdir = LR;\n");
    fprintf(out, "    nodeStat [color = \"purple\"; label = \"<del>list free start index: %d|<cap>list capacity: %d|<size>list length: %d|<elemSize>list element size: %d\"];\n",
        list->freeStartIndex,
        (uint32_t)list->capacity,
        (uint32_t)list->length,
        (uint32_t)list->elementSize
    );
    fprintf(out, "\n\n");

    // check freee element list validness (it's required to determine is element free or not)
    uint32_t index = list->freeStartIndex;

    for (uint32_t i = 0; i < list->capacity - list->length; i++)
        index = list->links[index].next;

    if (index != 0) {
        fprintf(out, "    // invalid free list\n}\n");
        return;
    }

    // add 'overweight' connections to straighten list structure
    for (uint32_t i = 0; i <= list->capacity; i++) {
        // check for node being located in free list
        // quite ugly, but still working solution
        bool isDeleted = false;
        uint32_t freeIndex = list->freeStartIndex;
        while (freeIndex != 0) {
            if (freeIndex == i) {
                isDeleted = true;
                break;
            }
            freeIndex = list->links[freeIndex].next;
        }

        // dump node
        cfListPrintDotDumpNode(out, i, list, desc, isDeleted);

        // setup connection between current and prev node
        if (!isDeleted)
            fprintf(out, "    node%d -> node%d [constraint = false; color = \"#0000FF\"];\n", i, list->links[i].prev);
        // setup connection between current and next node
        fprintf(out, "    node%d -> node%d [constraint = false; color = \"#FF0000\"];\n", i, list->links[i].next);

        // add 'overweight' connection to straighten dump structure
        uint32_t nextOverweightIndex = (i + 1) % (uint32_t)(list->capacity + 1);

        if (nextOverweightIndex != 0)
            fprintf(out, "    node%d -> node%d [weight = 1000; color = \"#0000FF00\"];\n", i, nextOverweightIndex);
    }

    fprintf(out, "\n\n");

    // close digraph
    fprintf(out, "}\n");
} // cfDbgListPrintDot

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
