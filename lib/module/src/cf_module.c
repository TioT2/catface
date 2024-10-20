#include <assert.h>
#include <stdlib.h>

#include "cf_module.h"
#include "cf_hash.h"

/// @brief module file magic
const uint64_t CF_MODULE_MAGIC = 0x0045434146544143; // "CATFACE\0" as char

/// @brief module file header representation structure
typedef struct __CfModuleHeader {
    uint64_t magic;      ///< module magic number
    uint64_t codeLength; ///< module length
    CfHash   codeHash;   ///< module bytecode hash
} CfModuleHeader;

CfModuleReadStatus cfModuleRead( FILE *file, CfModule *dst ) {
    assert(file != NULL);
    assert(dst != NULL);

    CfModuleHeader header;

    if (!fread(&header, sizeof(CfModuleHeader), 1, file))
        return CF_MODULE_READ_STATUS_UNEXPECTED_FILE_END;

    if (header.magic != CF_MODULE_MAGIC)
        return CF_MODULE_READ_STATUS_INVALID_MODULE_MAGIC;

    void *code = calloc(header.codeLength, 1);
    if (code == NULL)
        return CF_MODULE_READ_STATUS_INTERNAL_ERROR;
    size_t readCount = fread(code, 1, header.codeLength, file);
    if (readCount != header.codeLength) {
        free(code);
        return CF_MODULE_READ_STATUS_UNEXPECTED_FILE_END;
    }

    CfHash hash = cfHash(code, readCount);
    if (!cfHashCompare(&hash, &header.codeHash)) {
        free(code);
        return CF_MODULE_READ_STATUS_CODE_INVALID_HASH;
    }

    dst->code = code;
    dst->codeLength = header.codeLength;

    return CF_MODULE_READ_STATUS_OK;
} // cfModuleRead

bool cfModuleWrite( const CfModule *module, FILE *dst ) {
    assert(module != NULL);
    assert(dst != NULL);

    const CfModuleHeader moduleHeader = {
        .magic = CF_MODULE_MAGIC,
        .codeLength = module->codeLength,
        .codeHash = cfHash(module->code, module->codeLength),
    };

    return true
        && sizeof(moduleHeader) == fwrite(&moduleHeader, 1, sizeof(moduleHeader), dst)
        && module->codeLength == fwrite(module->code, 1, module->codeLength, dst)
    ;
} // cfModuleWrite

void cfModuleDtor( CfModule *module ) {
    assert(module != NULL);

    free(module->code);
} // cfModuleDtor


const char * cfModuleReadStatusStr( CfModuleReadStatus status ) {
    switch (status) {
    case CF_MODULE_READ_STATUS_OK                   : return "ok";
    case CF_MODULE_READ_STATUS_INTERNAL_ERROR       : return "internal error";
    case CF_MODULE_READ_STATUS_UNEXPECTED_FILE_END  : return "unexpected file end";
    case CF_MODULE_READ_STATUS_INVALID_MODULE_MAGIC : return "invalid module magic";
    case CF_MODULE_READ_STATUS_CODE_INVALID_HASH    : return "invalid hash";

    default                                         : return "<invalid>";
    }
} // cfModuleReadStatusStr

// cf_module.h
