#include "cf_stack.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct __CfStackImpl {
    size_t size;     ///< Stack size
    size_t elemSize; ///< Stack element size
    size_t capacity; ///< Stack capacity
    char   data[1];  ///< Stack data
} CfStackImpl;

CfStack cfStackCtor( size_t elementSize ) {
    CfStackImpl *stk = (CfStackImpl *)calloc(1, sizeof(CfStackImpl));

    if (stk == NULL)
        return CF_STACK_NULL;

    *stk = CfStackImpl {
        .size = 0,
        .elemSize = elementSize,
        .capacity = 0,
    };

    return (CfStack)stk;
} // cfStackCtor

void cfStackDtor( CfStack stack ) {
    free((CfStackImpl *)stack);
} // cfStackDtor

CfStackStatus cfStackPush( CfStack *handle, const void *const data ) {
    return cfStackPushArrayReversed(handle, data, 1);
} // cfStackPush

CfStackStatus cfStackPop( CfStack *handle, void *const data ) {
    assert(handle != NULL);
    assert(*handle != CF_STACK_NULL);

    CfStackImpl *self = (CfStackImpl *)*handle;

    if (self->size == 0)
        return CF_STACK_NO_VALUES;

    if (data != NULL)
        memcpy(data, self->data + self->elemSize * (self->size - 1), self->elemSize);
    self->size -= 1;

    // TODO: Handle size decrease

    return CF_STACK_OK;
} // cfStackPop

bool cfStackToArray( CfStack stack, void **dst ) {
    assert(stack != CF_STACK_NULL);
    assert(dst != NULL);

    CfStackImpl *impl = (CfStackImpl *)stack;

    if (impl->size == 0) {
        *dst = NULL;
        return true;
    }

    void *data = calloc(impl->size * impl->elemSize, 1);
    if (data == NULL)
        return false;
    memcpy(data, impl->data, impl->size * impl->elemSize);
    *dst = data;
    return true;
} // cfStackToArray

size_t cfStackGetSize( CfStack stack ) {
    assert(stack != CF_STACK_NULL);

    return ((CfStackImpl *)stack)->size;
} // cfStackGetSize

CfStackStatus cfStackPushArrayReversed( CfStack *handle, const void *const array, const size_t elementCount ) {
    assert(handle != NULL);
    assert(array != NULL);

    // just don't perform any actions in case if there's no elements to push
    if (elementCount == 0)
        return CF_STACK_OK;

    CfStackImpl *self = (CfStackImpl *)*handle;

    if (self->capacity < self->size + elementCount) {
        // calculate new capacity
        size_t newCapacity = self->capacity;
        if (newCapacity == 0)
            newCapacity = 1;
        while (newCapacity < self->size + elementCount)
            newCapacity *= 2;

        // reallocate stack
        CfStackImpl *newSelf = (CfStackImpl *)realloc(self, sizeof(CfStackImpl) + self->elemSize * newCapacity);

        if (newSelf == NULL)
            return CF_STACK_INTERNAL_ERROR;
        self = newSelf;
        self->capacity = newCapacity;
        *handle = (CfStack)newSelf;
    }

    memcpy(self->data + self->elemSize * self->size, array, self->elemSize * elementCount);
    self->size += elementCount;

    return CF_STACK_OK;
} // cfStackPushArrayReversed

// cf_stack.cpp
