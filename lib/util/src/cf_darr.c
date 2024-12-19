#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cf_darr.h"

/// @brief dynamic array implementation structure
typedef struct CfDarrImpl_ {
    size_t elementSize; ///< size of single array element
    size_t size;        ///< count of elements in array
    size_t capacity;    ///< array capacity

    uint8_t data[1];    ///< array elements
} CfDarrImpl;

CfDarr cfDarrCtor( size_t elementSize ) {
    CfDarrImpl *impl = (CfDarrImpl *)calloc(1, sizeof(CfDarrImpl));

    if (impl == NULL)
        return NULL;

    impl->elementSize = elementSize;
    return impl;
} // cfDarrCtor

void cfDarrDtor( CfDarr darr ) {
    free(darr);
} // cfDarrDtor

CfDarrStatus cfDarrPushArray( CfDarr *darr, const void *arr, size_t arrLen ) {
    assert(darr != NULL);
    assert(arr != NULL);

    CfDarrImpl *impl = *darr;
    assert(impl != NULL);

    if (impl->size + arrLen > impl->capacity) {
        size_t newCapacity = impl->capacity;
        size_t requiredCapacity = impl->size + arrLen;
        if (newCapacity == 0)
            newCapacity = 1;
        while (newCapacity < requiredCapacity)
            newCapacity *= 2;

        // perform resize
        CfDarrImpl *newImpl = (CfDarrImpl *)realloc(impl, sizeof(CfDarrImpl) + newCapacity * impl->elementSize);
        if (newImpl == NULL)
            return CF_DARR_INTERNAL_ERROR;
        impl = newImpl;
        impl->capacity = newCapacity;
        *darr = impl;
    }

    memcpy(impl->data + impl->size * impl->elementSize, arr, arrLen * impl->elementSize);
    impl->size += arrLen;

    return CF_DARR_OK;
} // cfDarrPushArray function end

CfDarrStatus cfDarrPush( CfDarr *darr, const void *src ) {
    return cfDarrPushArray(darr, src, 1);
} // cfDarrPush

CfDarrStatus cfDarrPop( CfDarr *darr, void *dst ) {
    assert(darr != NULL);

    CfDarrImpl *impl = *darr;

    assert(impl != NULL);

    if (impl->size == 0)
        return CF_DARR_NO_VALUES;

    impl->size -= 1;
    if (dst != NULL)
        memcpy(dst, impl->data + impl->size * impl->elementSize, impl->elementSize);

    return CF_DARR_OK;
} // cfDarrPop

void cfDarrClear( CfDarr *darr ) {
    assert(darr != NULL);
    CfDarrImpl *impl = *darr;
    assert(impl != NULL);

    impl->size = 0;
} // cfDarrClear

void * cfDarrData( CfDarr darr ) {
    assert(darr != NULL);
    return darr->data;
} // cfDarrData

size_t cfDarrLength( CfDarr darr ) {
    assert(darr != NULL);
    return darr->size;
} // cfDarrLength

CfDarrStatus cfDarrIntoData( CfDarr darr, void **dst ) {
    assert(darr != NULL);
    void *data = calloc(darr->elementSize, darr->size);
    if (data == NULL)
        return CF_DARR_INTERNAL_ERROR;
    memcpy(data, darr->data, darr->size * darr->elementSize);
    *dst = data;
    return CF_DARR_OK;
} // cfDarrIntoData

// cf_darr.c
