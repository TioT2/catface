/**
 * @brief object builder implementation file
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <cf_darr.h>
#include <cf_deque.h>

#include "cf_object_internal.h"

void cfObjectDtor2( CfObject2 *object ) {
    // literally all
    free(object);
} // cfObjectDtor2

/// @brief object builder structure declaration
struct CfObjectBuilder_ {
    CfDarr code;   ///< code array
    CfDarr links;  ///< link array
    CfDarr labels; ///< label array
}; // struct CfObjectBuilder_

CfObjectBuilder * cfObjectBuilderCtor( void ) {
    CfObjectBuilder *builder = (CfObjectBuilder *)calloc(1, sizeof(CfObjectBuilder));
    CfDarr code = NULL;   ///< code dynamic array
    CfDarr links = NULL;  ///< link dynamic array
    CfDarr labels = NULL; ///< label dynamic array

    if (false
        || (builder = (CfObjectBuilder *)calloc(1, sizeof(CfObjectBuilder))) == NULL
        || (code = cfDarrCtor(1)) == NULL
        || (links = cfDarrCtor(sizeof(CfLink))) == NULL
        || (labels = cfDarrCtor(sizeof(CfLabel))) == NULL
    ) {
        free(builder);
        cfDarrDtor(code);
        cfDarrDtor(links);
        cfDarrDtor(labels);

        return NULL;
    }

    *builder = (CfObjectBuilder) {
        .code = code,
        .links = links,
        .labels = labels,
    };

    return builder;
} // cfObjectBuilderCtor

void cfObjectBuilderDtor( CfObjectBuilder *builder ) {
    if (builder == NULL)
        return;

    cfDarrDtor(builder->labels);
    cfDarrDtor(builder->links);
    cfDarrDtor(builder->code);

    free(builder);
} // cfObjectBuilderDtor

/**
 * @brief up alignment funciton
 */
static size_t cfObjectBuilderAlignUp( size_t toAlign, size_t alignment ) {
    return (toAlign / alignment + (size_t)(toAlign % alignment != 0)) * alignment;
} // cfObjectBuilderAlignUp

void cfObjectBuilderReset( CfObjectBuilder *const self ) {
    assert(self != NULL);

    // clear builder arrays
    cfDarrClear(&self->code);
    cfDarrClear(&self->labels);
    cfDarrClear(&self->links);
} // cfObjectBuilderReset

CfObject2 * cfObjectBuilderEmit( CfObjectBuilder *self ) {
    assert(self != NULL);

    size_t objectLengthBytes = cfObjectBuilderAlignUp(sizeof(CfObject2), sizeof(max_align_t));
    size_t labelsLengthBytes = cfObjectBuilderAlignUp(cfDarrLength(self->labels), sizeof(max_align_t));
    size_t linksLengthBytes  = cfObjectBuilderAlignUp(cfDarrLength(self->links), sizeof(max_align_t));
    size_t codeLengthBytes   = cfObjectBuilderAlignUp(cfDarrLength(self->code), sizeof(max_align_t));

    // object allocation size
    size_t objectAlocationSize = 0
        + objectLengthBytes
        + labelsLengthBytes
        + linksLengthBytes
        + codeLengthBytes
    ;

    // allocate object
    void *allocation = calloc(1, objectAlocationSize);
    if (allocation == NULL)
        return NULL;

    // set values
    CfObject2 *object = (CfObject2 *)((uint8_t *)allocation + 0);
    CfLabel   *labels = (  CfLabel *)((uint8_t *)object     + objectLengthBytes);
    CfLink    *links  = (   CfLink *)((uint8_t *)labels     + labelsLengthBytes);
    void      *code   = (     void *)((uint8_t *)links      + linksLengthBytes);

    // write data
    memcpy(labels, cfDarrData(self->labels), cfDarrLength(self->labels) * sizeof(CfLabel));
    memcpy(links , cfDarrData(self->links ), cfDarrLength(self->links ) * sizeof(CfLink ));
    memcpy(code  , cfDarrData(self->code  ), cfDarrLength(self->code  )                  );

    // compose object
    *object = (CfObject2) {
        .labelArray       = labels,
        .labelArrayLength = cfDarrLength(self->labels),
        .linkArray        = links,
        .linkArrayLength  = cfDarrLength(self->links),
        .code             = code,
        .codeLength       = cfDarrLength(self->code),
    };

    // return object
    return object;
} // cfObjectBuilderFinish

size_t cfObjectBuilderGetCodeLength( CfObjectBuilder *const self ) {
    return cfDarrLength(self->code);
} // cfObjectBuilderGetCodeLength

bool cfObjectBuilderAddCode( CfObjectBuilder *const self, const void *code, size_t size ) {
    assert(self != NULL);

    return cfDarrPushArray(self->code, code, size) == CF_DARR_OK;
} // cfObjectBuilderAddCode

bool cfObjectBuilderAddLink( CfObjectBuilder *const self, const CfLink *link ) {
    assert(self != NULL);

    return cfDarrPush(self->links, link) == CF_DARR_OK;
} // cfObjectBuilderAddLink

bool cfObjectBuilderAddLinkArray( CfObjectBuilder *const self, const CfLink *array, size_t length ) {
    assert(self != NULL);
    assert(array != NULL);

    return cfDarrPushArray(&self->links, array, length) == CF_DARR_OK;
} // cfObjectBuilderAddLinkArray

bool cfObjectBuilderAddLabel( CfObjectBuilder *const self, const CfLabel *label ) {
    assert(self != NULL);

    return cfDarrPush(self->labels, label) == CF_DARR_OK;
} // cfObjectBuilderAddLabel

bool cfObjectBuilderAddLabelArray( CfObjectBuilder *const self, const CfLabel *array, size_t length ) {
    assert(self != NULL);
    assert(array != NULL);

    return cfDarrPushArray(&self->labels, array, length) == CF_DARR_OK;
} // cfObjectBuilderAddLabelArray

// cf_object_builder.c
