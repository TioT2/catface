#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cf_object.h"

/// @brief object file magic
const uint64_t CF_OBJECT_MAGIC = 0x00004A424F544143;

/// @brief object file representation structure
typedef struct __CfObjectFileHeader {
    uint64_t magic;                ///< magic value
    uint32_t sourceFileNameLength; ///< length of object source file path
    uint32_t codeLength;           ///< code section length
    uint32_t labelCount;          ///< label section lenght
    uint32_t linkCount;           ///< link section length
} CfObjectFileHeader;

CfObjectReadStatus cfObjectRead( FILE *file, CfObject *dst ) {
    assert(file != NULL);
    assert(dst != NULL);

    CfObjectFileHeader header = {0};

    if (1 != fread(&header, sizeof(header), 1, file))
        return CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END;
    if (header.magic != CF_OBJECT_MAGIC)
        return CF_OBJECT_READ_STATUS_INVALID_OBJECT_MAGIC;

    // just for simplicity
    char *sourceFileName = (char *)calloc(header.sourceFileNameLength, sizeof(char));
    uint8_t *code = (uint8_t *)calloc(header.codeLength, sizeof(uint8_t));
    CfLink *links = (CfLink *)calloc(header.linkCount, sizeof(CfLink));
    CfLabel *labels = (CfLabel *)calloc(header.labelCount, sizeof(CfLabel));
    CfObjectReadStatus status = CF_OBJECT_READ_STATUS_OK;

    if (sourceFileName == NULL || code == NULL || links == NULL || labels == NULL) {
        status = CF_OBJECT_READ_STATUS_INTERNAL_ERROR;
        goto cfObjectRead__error;
    }

    // read code/links/labels
    if (false
        || header.sourceFileNameLength != fread(sourceFileName, sizeof(char),    header.sourceFileNameLength, file)
        || header.codeLength           != fread(code,           sizeof(uint8_t), header.codeLength,           file)
        || header.linkCount            != fread(links,          sizeof(CfLink),  header.linkCount,            file)
        || header.labelCount           != fread(labels,         sizeof(CfLabel), header.labelCount,           file)
    ) {
        status = CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END;
        goto cfObjectRead__error;
    }

    dst->code = code;
    dst->codeLength = header.codeLength;
    dst->links = links;
    dst->linkCount = header.linkCount;
    dst->labels = labels;
    dst->labelCount = header.labelCount;

    return CF_OBJECT_READ_STATUS_OK;

cfObjectRead__error:
    free(code);
    free(links);
    free(labels);
    return status;
} // cfObjectRead

bool cfObjectWrite( FILE *file, const CfObject *src ) {
    assert(file != NULL);
    assert(src != NULL);

    const CfObjectFileHeader header = {
        .magic = CF_OBJECT_MAGIC,
        .sourceFileNameLength = (uint32_t)strlen(src->sourceFileName),
        .codeLength = (uint32_t)src->codeLength,
        .labelCount = (uint32_t)src->labelCount,
        .linkCount = (uint32_t)src->linkCount,
    };

    // yeah functional style
    return true
        && 1 == fwrite(&header, sizeof(CfObjectFileHeader), 1, file)
        && header.sourceFileNameLength == fwrite(src->sourceFileName, sizeof(char), header.sourceFileNameLength, file)
        && header.codeLength == fwrite(src->code, sizeof(uint8_t), header.codeLength, file)
        && header.linkCount == fwrite(src->links, sizeof(CfLink), header.linkCount, file)
        && header.labelCount == fwrite(src->labels, sizeof(CfLabel), header.labelCount, file)
    ;
} // cfObjectWrite

void cfObjectDtor( CfObject *object ) {
    // write NULLs or not?

    if (object != NULL) {
        free((char *)object->sourceFileName);
        free(object->code);
        free(object->labels);
        free(object->links);
    }
} // cfObjectDtor

// cf_object.c
