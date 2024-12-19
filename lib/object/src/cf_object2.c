/**
 * @brief new object implementation file
 */

#include <cf_hash.h>

#include <assert.h>

#include "cf_object_internal.h"

size_t cfObjectGetCodeLength( const CfObject2 *object ) {
    assert(object != NULL);

    return object->codeLength;
} // cfObjectGetCodeLength

const void * cfObjectGetCode( const CfObject2 *object ) {
    assert(object != NULL);

    return object->code;
} // cfObjectGetCode

size_t cfObjectGetLinkArrayLength( const CfObject2 *object ) {
    assert(object != NULL);

    return object->linkArrayLength;
} // cfObjectGetLinkArrayLength

const CfLink * cfObjectGetLinkArray( const CfObject2 *object ) {
    assert(object != NULL);

    return object->linkArray;
} // cfObjectGetLinkArray

size_t cfObjectGetLabelArrayLength( const CfObject2 *object ) {
    assert(object != NULL);

    return object->labelArrayLength;
} // cfObjectGetLabelArrayLength

const CfLabel * cfObjectGetLabelArray( const CfObject2 *object ) {
    assert(object != NULL);

    return object->labelArray;
} // cfObjectGetLabelArray


void cfObjectReadResultWrite( FILE *out, const CfObjectReadResult2 *result ) {
    assert(result != NULL);

    switch (result->status) {
    case CF_OBJECT_READ_STATUS_2_OK: {
        break;
        fprintf(out, "succeeded");
    }

    case CF_OBJECT_READ_STATUS_2_INTERNAL_ERROR: {
        fprintf(out, "internal error");
        break;
    }

    case CF_OBJECT_READ_STATUS_2_UNEXPECTED_FILE_END: {
        result->unexpectedFileEnd.offset;
        fprintf(out, "unexpected file end (offset: %zX, read: %zU, required: %zU)",
            result->unexpectedFileEnd.offset,
            result->unexpectedFileEnd.actualByteCount,
            result->unexpectedFileEnd.requiredByteCount
        );
        break;
    }

    case CF_OBJECT_READ_STATUS_2_INVALID_MAGIC: {
        fprintf(out, "invalid magic (actual: %zU, expected: %zU)",
            result->invalidMagic.actual,
            result->invalidMagic.expected
        );
        break;
    }

    case CF_OBJECT_READ_STATUS_2_INVALID_HASH: {
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
    uint32_t labelArrayLength; ///< label section lenght
    uint32_t linkArrayLength;  ///< link section length
    uint32_t codeLength;       ///< code section length
    CfHash   dataHash;         ///< code - link - label hash
} CfObjectFileHeader;


CfObjectReadResult2 cfObjectRead2( FILE *file, CfObjectBuilder *builder ) {
    bool builderOwned = false;
    if (builder == NULL) {
        builder = cfObjectBuilderCtor();
        if (builder == NULL)
            return (CfObjectReadResult2) { CF_OBJECT_READ_STATUS_2_INTERNAL_ERROR };
        builderOwned = true;
    }

    CfObjectReadResult2 result = { CF_OBJECT_READ_STATUS_2_INTERNAL_ERROR };
    size_t pos = 0;
    size_t actual = 0;
    size_t contentLength = 0;
    void *contentBuffer = NULL;
    CfHash actualHash = {};
    CfObject2 *object = NULL;

    // read ALL file contents
    CfObjectFileHeader header = {};

    // current position
    pos = ftell(file);
    if ((actual = fread(&header, 1, sizeof(CfObjectFileHeader), file)) != sizeof(CfObjectFileHeader)) {
        result = (CfObjectReadResult2) {
            .status = CF_OBJECT_READ_STATUS_2_UNEXPECTED_FILE_END,
            .unexpectedFileEnd = {
                .offset          = pos,
                .actualByteCount = actual,
                .requiredByteCount = sizeof(CfObjectFileHeader),
            },
        };

        goto cfObjectRead2__end;
    }

    if (header.magic != CF_OBJECT_MAGIC) {
        result = (CfObjectReadResult2) {
            .status = CF_OBJECT_READ_STATUS_2_INVALID_MAGIC,
            .invalidMagic = {
                .expected = CF_OBJECT_MAGIC,
                .actual = header.magic,
            },
        };

        goto cfObjectRead2__end;
    }

    // data
    contentBuffer = calloc(0
        + header.codeLength
        + header.labelArrayLength
        + header.linkArrayLength
    );

    if (contentBuffer == NULL) {
        result = (CfObjectReadResult2) { CF_OBJECT_READ_STATUS_2_INTERNAL_ERROR };
        goto cfObjectRead2__end;
    }

    contentLength = header.codeLength + header.labelArrayLength + header.linkArrayLength;

    pos = ftell(file);
    actual = fread(contentBuffer, 1, contentLength, file);

    if (actual < contentLength) {
        result = (CfObjectReadResult2) {
            .status = CF_OBJECT_READ_STATUS_2_UNEXPECTED_FILE_END,
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
        result = (CfObjectReadResult2) {
            .status = CF_OBJECT_READ_STATUS_2_INVALID_HASH,
            .invalidHash = {
                .actual = actualHash,
                .expected = header.dataHash,
            },
        };

        goto cfObjectRead2__end;
    }

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
        result = (CfObjectReadResult2) { CF_OBJECT_READ_STATUS_2_INTERNAL_ERROR };
        goto cfObjectRead2__end;
    }

    // assemble object
    object = cfObjectBuilderEmit(builder);
    if (object == NULL) {
        result = (CfObjectReadResult2) { CF_OBJECT_READ_STATUS_2_INTERNAL_ERROR };
        goto cfObjectRead2__end;
    }

    // handle success case
    result = (CfObjectReadResult2) {
        .status = CF_OBJECT_READ_STATUS_2_OK,
        .ok     = object,
    };

    cfObjectRead2__end:

    free(contentBuffer);
    if (builderOwned)
        cfObjectBuilderDtor(builder);
    return result;
} // cfObjectRead2

CfObjectWriteStatus cfObjectWrite2( FILE *file, const CfObject2 *object ) {
    assert(object != NULL);

    CfObjectFileHeader header = {
        .magic            = CF_OBJECT_MAGIC,
        .labelArrayLength = cfObjectGetLabelArrayLength(object),
        .linkArrayLength  = cfObjectGetLinkArrayLength(object),
        .codeLength       = cfObjectGetCodeLength(object),
    };

    // calculate data hash
    CfHasher hasher = {};

    cfHasherInitialize(&hasher);

    cfHasherStep(
        &hasher,
        cfObjectGetLabelArray(object),
        cfObjectGetLabelArrayLength(object) * sizeof(CfLabel)
    );
    cfHasherStep(
        &hasher,
        cfObjectGetLinkArray(object),
        cfObjectGetLinkArrayLength(object) * sizeof(CfLink)
    );
    cfHasherStep(
        &hasher,
        cfObjectGetCode(object),
        cfObjectGetCodeLength(object)
    );

    header.dataHash = cfHasherTerminate(&hasher);

    // write header and data
    bool succeeded = true
        && fwrite(&header, sizeof(header), 1, file) != 0
        && fwrite(
            cfObjectGetLabelArray(object),
            cfObjectGetLabelArrayLength(object) * sizeof(CfLabel),
            1,
            file
        ) != 0
        && fwrite(
            cfObjectGetLinkArray(object),
            cfObjectGetLinkArrayLength(object) * sizeof(CfLink),
            1,
            file
        ) != 0
        && fwrite(
            cfObjectGetCode(object),
            cfObjectGetCodeLength(object),
            1,
            file
        ) != 0
    ;

    return succeeded
        ? CF_OBJECT_WRITE_STATUS_OK
        : CF_OBJECT_WRITE_STATUS_WRITE_ERROR
    ;
} // cfObjectWrite2


// cf_object2.c
