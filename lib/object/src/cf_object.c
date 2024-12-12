#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <cf_hash.h>

#include "cf_object.h"

/// @brief object file magic
const uint64_t CF_OBJECT_MAGIC = 0x00004A424F544143;

/// @brief object file representation structure
typedef struct CfObjectFileHeader_ {
    uint64_t magic;            ///< magic value
    uint32_t sourceNameLength; ///< length of object source file path
    uint32_t codeLength;       ///< code section length
    uint32_t linkCount;        ///< link section length
    uint32_t labelCount;       ///< label section lenght
    CfHash   dataHash;         ///< sourceName - code - link - label hash
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
    char *sourceName = (char *)calloc(header.sourceNameLength, sizeof(char));
    uint8_t *code = (uint8_t *)calloc(header.codeLength, sizeof(uint8_t));
    CfLink *links = (CfLink *)calloc(header.linkCount, sizeof(CfLink));
    CfLabel *labels = (CfLabel *)calloc(header.labelCount, sizeof(CfLabel));
    CfObjectReadStatus status = CF_OBJECT_READ_STATUS_OK;

    CfHasher hasher = {0};
    CfHash dataHash = {0};

    if (sourceName == NULL || code == NULL || links == NULL || labels == NULL) {
        status = CF_OBJECT_READ_STATUS_INTERNAL_ERROR;
        goto cfObjectRead__error;
    }

    // read code/links/labels
    if (false
        || header.sourceNameLength != fread(sourceName,     sizeof(char),    header.sourceNameLength, file)
        || header.codeLength       != fread(code,           sizeof(uint8_t), header.codeLength,       file)
        || header.linkCount        != fread(links,          sizeof(CfLink),  header.linkCount,        file)
        || header.labelCount       != fread(labels,         sizeof(CfLabel), header.labelCount,       file)
    ) {
        status = CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END;
        goto cfObjectRead__error;
    }

    // calculate hash for object data
    cfHasherInitialize(&hasher);
    cfHasherStep(&hasher, sourceName, header.sourceNameLength);
    cfHasherStep(&hasher, code,       header.codeLength);
    cfHasherStep(&hasher, links,      header.linkCount * sizeof(CfLink));
    cfHasherStep(&hasher, labels,     header.labelCount * sizeof(CfLabel));
    dataHash = cfHasherTerminate(&hasher);

    // compare hashes
    if (!cfHashCompare(&dataHash, &header.dataHash)) {
        status = CF_OBJECT_READ_STATUS_INVALID_HASH;
        goto cfObjectRead__error;
    }

    dst->sourceName = sourceName;
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

    CfObjectFileHeader header = {
        .magic = CF_OBJECT_MAGIC,
        .sourceNameLength = (uint32_t)strlen(src->sourceName),
        .codeLength = (uint32_t)src->codeLength,
        .linkCount = (uint32_t)src->linkCount,
        .labelCount = (uint32_t)src->labelCount,
    };

    // calculate header hash

    CfHasher hasher = {0};
    cfHasherInitialize(&hasher);
    cfHasherStep(&hasher, src->sourceName, header.sourceNameLength);
    cfHasherStep(&hasher, src->code,       header.codeLength);
    cfHasherStep(&hasher, src->links,      header.linkCount * sizeof(CfLink));
    cfHasherStep(&hasher, src->labels,     header.labelCount * sizeof(CfLabel));
    header.dataHash = cfHasherTerminate(&hasher);

    // yeah functional style
    return true
        && 1 == fwrite(&header, sizeof(CfObjectFileHeader), 1, file)
        && header.sourceNameLength == fwrite(src->sourceName, sizeof(char), header.sourceNameLength, file)
        && header.codeLength == fwrite(src->code, sizeof(uint8_t), header.codeLength, file)
        && header.linkCount == fwrite(src->links, sizeof(CfLink), header.linkCount, file)
        && header.labelCount == fwrite(src->labels, sizeof(CfLabel), header.labelCount, file)
    ;
} // cfObjectWrite

void cfObjectDtor( CfObject *object ) {
    // write NULLs or not?

    if (object != NULL) {
        free((char *)object->sourceName);
        free(object->code);
        free(object->labels);
        free(object->links);
    }
} // cfObjectDtor

const char * cfObjectReadStatusStr( CfObjectReadStatus status ) {
    switch (status) {
    case CF_OBJECT_READ_STATUS_OK                   : return "ok";
    case CF_OBJECT_READ_STATUS_INTERNAL_ERROR       : return "internal error";
    case CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END  : return "unexpected file end";
    case CF_OBJECT_READ_STATUS_INVALID_OBJECT_MAGIC : return "invalid object magic";
    case CF_OBJECT_READ_STATUS_INVALID_HASH         : return "invalid hash";
    }

    return "<invalid>";
} // cfObjectReadStatusStr

// cf_object.c
