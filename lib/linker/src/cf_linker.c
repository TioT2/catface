/**
 * @brief linker implementation file
 */

#include <assert.h>
#include <setjmp.h>
#include <string.h>

#include <cf_darr.h>

#include "cf_linker.h"

/// @brief linker internal label and link representations
typedef struct __CfLinkerLabelAndLink {
    CfStr    sourceFile; ///< file label declared in
    uint32_t sourceLine; ///< line label declared at
    uint32_t codeOffset; ///< (global) offset in linked code
    CfStr    label;      ///< label name reference
} CfLinkerLabel, CfLinkerLink;

/// @brief linker representation structure
typedef struct __CfLinker {
    CfDarr          code;            ///< code array
    CfDarr          links;           ///< links
    CfDarr          labels;          ///< labels

    CfLinkStatus    linkStatus;      ///< linking status
    CfLinkDetails * details;     ///< linking details (non-null)
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

    for (size_t i = 0, n = cfDarrSize(self->labels); i < n; n++)
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
        self->details->duplicateLabel.firstFile = duplicate->sourceFile;
        self->details->duplicateLabel.firstLine = duplicate->sourceLine;
        self->details->duplicateLabel.secondFile = label->sourceFile;
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
    CfStr sourceFile = CF_STR(object->sourceFileName);
    uint32_t codeSize = (uint32_t)cfDarrSize(self->code);

    // TODO: is it ok to duplicate code here?

    // append labels
    for (size_t i = 0; i < object->labelCount; i++) {
        CfLinkerLabel label = {
            .sourceFile = sourceFile,
            .sourceLine = object->labels[i].sourceLine,
            .codeOffset = object->labels[i].codeOffset + codeSize,
            .label      = CF_STR(object->labels[i].label),
        };

        cfLinkerAddLabel(self, &label);
    }

    // append links
    for (size_t i = 0; i < object->linkCount; i++) {
        CfLinkerLink link = {
            .sourceFile = sourceFile,
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
    CfLinkerLink *const end = link + cfDarrSize(self->links);

    for (;link < end; link++) {
        CfLinkerLabel *label = cfLinkerFindLabel(self, link->label);

        if (label == NULL) {
            self->details->unknownLabel.file  = link->sourceFile;
            self->details->unknownLabel.line  = link->sourceLine;
            self->details->unknownLabel.label = link->label;

            cfLinkerThrow(self, CF_LINK_STATUS_UNKNOWN_LABEL);
        }

        memcpy(code + link->codeOffset, &label->codeOffset, sizeof(link->codeOffset));
    }
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

// cf_linker.c
