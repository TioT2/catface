/**
 * @brief linker implementation file
 */

#include <assert.h>
#include <setjmp.h>
#include <string.h>

#include <cf_darr.h>

#include "cf_linker.h"

/// @brief linker internal label representation
typedef struct CfLinkerLabel_ {
    CfStr    sourceName; ///< file label declared at
    uint32_t sourceLine; ///< line label declared at
    uint32_t value;      ///< label underlying value
    CfStr    label;      ///< label name
} CfLinkerLabel;

/// @brief linker internal label and link representations
typedef struct CfLinkerLabelAndLink_ {
    CfStr    sourceName; ///< file label declared in
    uint32_t sourceLine; ///< line label declared at
    uint32_t codeOffset; ///< (global) offset in linked code
    CfStr    label;      ///< label name reference
} CfLinkerLink;

/// @brief linker representation structure
typedef struct CfLinker_ {
    CfDarr          code;            ///< code array
    CfDarr          links;           ///< links
    CfDarr          labels;          ///< labels

    CfLinkStatus    linkStatus;      ///< linking status
    CfLinkDetails * details;         ///< linking details (non-null)
    jmp_buf         errorJumpBuffer; ///< jump buffer
} CfLinker;

/**
 * @brief linking error throwing function
 * 
 * @param[in,out] linker to throw error in
 * @param[in]     error  error to throw
 */
void cfLinkerThrow( CfLinker *const self, const CfLinkStatus error ) {
    self->linkStatus = error;
    longjmp(self->errorJumpBuffer, 0);
} // cfLinkerThrow

/**
 * @brief for linker label searching function function
 * 
 * @param[in] self  linker pointer
 * @param[in] label label to find
 * 
 * @return label pointer if found, NULL if not
 */
CfLinkerLabel * cfLinkerFindLabel( CfLinker *const self, CfStr label ) {
    CfLinkerLabel *labels = (CfLinkerLabel *)cfDarrData(self->labels);

    for (size_t i = 0, n = cfDarrLength(self->labels); i < n; i++)
        if (cfStrIsSame(label, labels[i].label))
            return labels + i;
    return NULL;
} // cfLinkerFindLabel

/**
 * @brief label to linker adding function
 * 
 * @param[in,out] self  linker referrence
 * @param[in]     label label to add
 */
void cfLinkerAddLabel( CfLinker *const self, const CfLinkerLabel *const label ) {

    // search for duplicate
    CfLinkerLabel *duplicate = cfLinkerFindLabel(self, label->label);
    if (duplicate != NULL) {
        self->details->duplicateLabel.firstFile = duplicate->sourceName;
        self->details->duplicateLabel.firstLine = duplicate->sourceLine;
        self->details->duplicateLabel.secondFile = label->sourceName;
        self->details->duplicateLabel.secondLine = label->sourceLine;
        self->details->duplicateLabel.label = label->label;

        cfLinkerThrow(self, CF_LINK_STATUS_DUPLICATE_LABEL);
    }

    // append label
    if (CF_DARR_OK != cfDarrPush(&self->labels, label))
        cfLinkerThrow(self, CF_LINK_STATUS_INTERNAL_ERROR);
} // cfLinkerAddLabel

/**
 * @brief link to linker appending function
 * 
 * @param[in,out] self linker pointer
 * @param[in]     link link pointer
 */
void cfLinkerAddLink( CfLinker *const self, const CfLinkerLink *const link ) {
    if (CF_DARR_OK != cfDarrPush(&self->links, link))
        cfLinkerThrow(self, CF_LINK_STATUS_INTERNAL_ERROR);
} // cfLinkerAddLink

/**
 * @brief object to linker adding function
 * 
 * @param[in,out] self   linker pointer
 * @param[in]     object object to add
 */
void cfLinkerAddObject( CfLinker *const self, const CfObject *const object ) {
    CfStr sourceName = CF_STR(object->sourceName);
    uint32_t codeSize = (uint32_t)cfDarrLength(self->code);

    // TODO: is it ok to duplicate code here?

    // append labels
    for (size_t i = 0; i < object->labelCount; i++) {
        CfLinkerLabel label = {
            .sourceName = sourceName,
            .sourceLine = object->labels[i].sourceLine,
            .value      = object->labels[i].isRelative
                ? object->labels[i].value + codeSize
                : object->labels[i].value
            ,
            .label      = CF_STR(object->labels[i].label),
        };

        cfLinkerAddLabel(self, &label);
    }

    // append links
    for (size_t i = 0; i < object->linkCount; i++) {
        CfLinkerLink link = {
            .sourceName = sourceName,
            .sourceLine = object->links[i].sourceLine,
            .codeOffset = object->links[i].codeOffset + codeSize,
            .label      = CF_STR(object->links[i].label),
        };

        cfLinkerAddLink(self, &link);
    }

    // append code
    if (CF_DARR_OK != cfDarrPushArray(&self->code, object->code, object->codeLength))
        cfLinkerThrow(self, CF_LINK_STATUS_INTERNAL_ERROR);
} // cfLinkerAddObject

/**
 * @brief executable building (aka linking finalization) function
 * 
 * @param[in,out] self linker pointer
 * @param[in]     dst  object to add
 */
void cfLinkerBuildExecutable( CfLinker *const self, CfExecutable *const dst ) {
    uint8_t *code = (uint8_t *)cfDarrData(self->code);

    CfLinkerLink *link = (CfLinkerLink *)cfDarrData(self->links);
    CfLinkerLink *const end = link + cfDarrLength(self->links);

    for (;link < end; link++) {
        CfLinkerLabel *label = cfLinkerFindLabel(self, link->label);

        if (label == NULL) {
            self->details->unknownLabel.file  = link->sourceName;
            self->details->unknownLabel.line  = link->sourceLine;
            self->details->unknownLabel.label = link->label;

            cfLinkerThrow(self, CF_LINK_STATUS_UNKNOWN_LABEL);
        }

        memcpy(code + link->codeOffset, &label->value, sizeof(link->codeOffset));
    }

    dst->codeLength = cfDarrLength(self->code);
    if (CF_DARR_OK != cfDarrIntoData(self->code, &dst->code))
        cfLinkerThrow(self, CF_LINK_STATUS_INTERNAL_ERROR);
} // cfLinkerBuildExecutable

CfLinkStatus cfLink(
    const CfObject *const objects,
    const size_t          objectCount,
    CfExecutable   *const dst,
    CfLinkDetails  *const details
) {
    assert(dst != NULL);
    assert(objects != NULL);
    assert(objectCount != 0);

    CfLinker linker = {0};
    CfLinkDetails dummyDetails;

    // initialize jump buffer
    int jmp = setjmp(linker.errorJumpBuffer);
    if (jmp)
        goto cfLink__end;

    linker.code = cfDarrCtor(1);
    linker.links = cfDarrCtor(sizeof(CfLinkerLink));
    linker.labels = cfDarrCtor(sizeof(CfLinkerLabel));
    linker.details = details == NULL ? &dummyDetails : details;
    linker.linkStatus = CF_LINK_STATUS_OK;

    // construct
    if (linker.code == NULL || linker.links == NULL || linker.labels == NULL) {
        linker.linkStatus = CF_LINK_STATUS_INTERNAL_ERROR;
        goto cfLink__end;
    }

    for (size_t i = 0; i < objectCount; i++)
        cfLinkerAddObject(&linker, objects + i);
    cfLinkerBuildExecutable(&linker, dst);

cfLink__end:
    cfDarrDtor(linker.code);
    cfDarrDtor(linker.links);
    cfDarrDtor(linker.labels);
    return linker.linkStatus;
} // cfLink


void cfLinkDetailsWrite( FILE *output, CfLinkStatus status, const CfLinkDetails *details ) {
    assert(details != NULL);

    switch (status) {
    case CF_LINK_STATUS_INTERNAL_ERROR:
        fprintf(output, "internal linker error.");
        break;

    case CF_LINK_STATUS_OK:
        fprintf(output, "ok.");
        break;

    case CF_LINK_STATUS_DUPLICATE_LABEL:
        fprintf(output, "duplicate declaration of label \"");
        cfStrWrite(output, details->duplicateLabel.label);
        fprintf(output, "\" (first: ");
        cfStrWrite(output, details->duplicateLabel.firstFile);
        fprintf(output, ":%d", details->duplicateLabel.firstLine);
        fprintf(output, ", second: ");
        cfStrWrite(output, details->duplicateLabel.secondFile);
        fprintf(output, ":%d)", details->duplicateLabel.secondLine);
        break;

    case CF_LINK_STATUS_UNKNOWN_LABEL:
        fprintf(output, "unknown label \"");
        cfStrWrite(output, details->unknownLabel.label);
        fprintf(output, "\" referenced at ");
        cfStrWrite(output, details->unknownLabel.file);
        fprintf(output, ":%d", details->unknownLabel.line);
        break;
    }
} // cfLinkDetailsWrite

// cf_linker.c
