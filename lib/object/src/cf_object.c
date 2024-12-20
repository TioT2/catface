/**
 * @brief object implementation file
 */

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <cf_hash.h>


#include "cf_object_internal.h"

size_t cfObjectGetCodeLength( const CfObject *object ) {
    assert(object != NULL);

    return object->codeLength;
} // cfObjectGetCodeLength

const void * cfObjectGetCode( const CfObject *object ) {
    assert(object != NULL);

    return object->code;
} // cfObjectGetCode

size_t cfObjectGetLinkArrayLength( const CfObject *object ) {
    assert(object != NULL);

    return object->linkArrayLength;
} // cfObjectGetLinkArrayLength

const CfLink * cfObjectGetLinkArray( const CfObject *object ) {
    assert(object != NULL);

    return object->linkArray;
} // cfObjectGetLinkArray

size_t cfObjectGetLabelArrayLength( const CfObject *object ) {
    assert(object != NULL);

    return object->labelArrayLength;
} // cfObjectGetLabelArrayLength

const CfLabel * cfObjectGetLabelArray( const CfObject *object ) {
    assert(object != NULL);

    return object->labelArray;
} // cfObjectGetLabelArray


void cfObjectReadResultWrite( FILE *out, const CfObjectReadResult *result ) {
    assert(result != NULL);

    switch (result->status) {
    case CF_OBJECT_READ_STATUS_OK: {
        break;
        fprintf(out, "succeeded");
    }

    case CF_OBJECT_READ_STATUS_INTERNAL_ERROR: {
        fprintf(out, "internal error");
        break;
    }

    case CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END: {
        fprintf(out, "unexpected file end (offset: %zX, read: %zu, required: %zu)",
            result->unexpectedFileEnd.offset,
            result->unexpectedFileEnd.actualByteCount,
            result->unexpectedFileEnd.requiredByteCount
        );
        break;
    }

    case CF_OBJECT_READ_STATUS_INVALID_MAGIC: {
        fprintf(out, "invalid magic (actual: %" PRIu64 ", expected: %" PRIu64 ")",
            result->invalidMagic.actual,
            result->invalidMagic.expected
        );
        break;
    }

    case CF_OBJECT_READ_STATUS_INVALID_HASH: {
        fprintf(out, "invalid hash");
        break;
    }
    }
} // cfObjectReadResultWrite


/// @brief object file magic
const uint64_t CF_OBJECT_MAGIC = 0x00004A424F544143;

/// @brief object file representation structure
typedef struct CfObjectFileHeader_ {
    uint64_t magic;            ///< magic value
    uint32_t nameLength;       ///< name length
    uint32_t labelArrayLength; ///< label section lenght
    uint32_t linkArrayLength;  ///< link section length
    uint32_t codeLength;       ///< code section length
    CfHash   dataHash;         ///< code - link - label hash
} CfObjectFileHeader;


CfObjectReadResult cfObjectRead( FILE *file, CfObjectBuilder *builder ) {
    bool builderOwned = false;
    if (builder == NULL) {
        builder = cfObjectBuilderCtor();
        if (builder == NULL)
            return (CfObjectReadResult) { CF_OBJECT_READ_STATUS_INTERNAL_ERROR };
        builderOwned = true;
    }

    CfObjectReadResult result = { CF_OBJECT_READ_STATUS_INTERNAL_ERROR };
    size_t pos = 0;
    size_t actual = 0;
    size_t contentLength = 0;
    void *contentBuffer = NULL;
    CfHash actualHash = {};
    CfObject *object = NULL;

    char *name = NULL;
    CfLabel *labels = NULL;
    CfLink *links = NULL;
    void *code = NULL;

    // read ALL file contents
    CfObjectFileHeader header = {};

    // current position
    pos = ftell(file);
    if ((actual = fread(&header, 1, sizeof(CfObjectFileHeader), file)) != sizeof(CfObjectFileHeader)) {
        result = (CfObjectReadResult) {
            .status = CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END,
            .unexpectedFileEnd = {
                .offset            = pos,
                .requiredByteCount = sizeof(CfObjectFileHeader),
                .actualByteCount   = actual,
            },
        };

        goto cfObjectRead2__end;
    }

    if (header.magic != CF_OBJECT_MAGIC) {
        result = (CfObjectReadResult) {
            .status = CF_OBJECT_READ_STATUS_INVALID_MAGIC,
            .invalidMagic = {
                .expected = CF_OBJECT_MAGIC,
                .actual = header.magic,
            },
        };

        goto cfObjectRead2__end;
    }

    contentLength = 0
        + header.nameLength
        + header.codeLength
        + header.labelArrayLength
        + header.linkArrayLength
    ;

    // data
    contentBuffer = calloc(1, contentLength);

    if (contentBuffer == NULL) {
        result = (CfObjectReadResult) { CF_OBJECT_READ_STATUS_INTERNAL_ERROR };
        goto cfObjectRead2__end;
    }

    pos = ftell(file);
    actual = fread(contentBuffer, 1, contentLength, file);

    if (actual < contentLength) {
        result = (CfObjectReadResult) {
            .status = CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END,
            .unexpectedFileEnd = {
                .offset = pos,
                .requiredByteCount = contentLength,
                .actualByteCount = actual,
            },
        };

        goto cfObjectRead2__end;
    }

    // calculate hash
    actualHash = cfHash(contentBuffer, contentLength);
    if (!cfHashCompare(&header.dataHash, &actualHash)) {
        result = (CfObjectReadResult) {
            .status = CF_OBJECT_READ_STATUS_INVALID_HASH,
            .invalidHash = {
                .expected = header.dataHash,
                .actual   = actualHash,
            },
        };

        goto cfObjectRead2__end;
    }

    name   = (   char *)((uint8_t *)contentBuffer + 0                );
    labels = (CfLabel *)((uint8_t *)name          + header.nameLength);
    links  = ( CfLink *)((uint8_t *)labels        + header.labelArrayLength * sizeof(CfLabel));
    code   = (   void *)((uint8_t *)links         + header.linkArrayLength * sizeof(CfLink));

    // write object contents to builder
    if (false
        || !cfObjectBuilderAddLabelArray(
            builder,
            (const CfLabel *)contentBuffer,
            header.labelArrayLength
        )
        || !cfObjectBuilderAddLinkArray(
            builder,
            (const CfLink *)((const CfLabel *)contentBuffer + header.labelArrayLength),
            header.linkArrayLength
        )
        || !cfObjectBuilderAddLinkArray(
            builder,
            (const CfLink *)((const CfLabel *)contentBuffer + header.linkArrayLength) + header.labelArrayLength,
            header.codeLength
        )
    ) {
        result = (CfObjectReadResult) { CF_OBJECT_READ_STATUS_INTERNAL_ERROR };
        goto cfObjectRead2__end;
    }

    // assemble object
    object = cfObjectBuilderEmit(builder, (CfStr) { name, name + header.nameLength });
    if (object == NULL) {
        result = (CfObjectReadResult) { CF_OBJECT_READ_STATUS_INTERNAL_ERROR };
        goto cfObjectRead2__end;
    }

    // handle success case
    result = (CfObjectReadResult) {
        .status = CF_OBJECT_READ_STATUS_OK,
        .ok     = object,
    };

    cfObjectRead2__end:

    free(contentBuffer);
    if (builderOwned)
        cfObjectBuilderDtor(builder);
    return result;
} // cfObjectRead

CfObjectWriteStatus cfObjectWrite( FILE *file, const CfObject *object ) {
    assert(object != NULL);

    CfObjectFileHeader header = {
        .magic            = CF_OBJECT_MAGIC,
        .nameLength       = (uint32_t)strlen(cfObjectGetSourceFileName(object)),
        .labelArrayLength = (uint32_t)cfObjectGetLabelArrayLength(object),
        .linkArrayLength  = (uint32_t)cfObjectGetLinkArrayLength(object),
        .codeLength       = (uint32_t)cfObjectGetCodeLength(object),
    };

    // calculate data hash
    CfHasher hasher = {};

    cfHasherInitialize(&hasher);

    cfHasherStep(&hasher, cfObjectGetSourceFileName(object), header.nameLength);
    cfHasherStep(&hasher, cfObjectGetLabelArray(object),     header.labelArrayLength * sizeof(CfLabel));
    cfHasherStep(&hasher, cfObjectGetLinkArray(object),      header.linkArrayLength * sizeof(CfLink));
    cfHasherStep(&hasher, cfObjectGetCode(object),           header.codeLength);

    header.dataHash = cfHasherTerminate(&hasher);

    // write header and data
    bool succeeded = true

        // write header
        && fwrite(&header, sizeof(header), 1, file) != 0

        // write arrays
        && 0 != fwrite(cfObjectGetSourceFileName(object), header.nameLength,                         1, file)
        && 0 != fwrite(cfObjectGetLabelArray(object),     header.labelArrayLength * sizeof(CfLabel), 1, file)
        && 0 != fwrite(cfObjectGetLinkArray(object),      header.linkArrayLength * sizeof(CfLink),   1, file)
        && 0 != fwrite(cfObjectGetCode(object),           header.codeLength,                         1, file)
    ;

    return succeeded
        ? CF_OBJECT_WRITE_STATUS_OK
        : CF_OBJECT_WRITE_STATUS_WRITE_ERROR
    ;
} // cfObjectWrite


// cf_object2.c
