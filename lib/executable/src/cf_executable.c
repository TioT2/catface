#include <assert.h>
#include <stdlib.h>

#include "cf_executable.h"
#include "cf_hash.h"

/// @brief executable file magic
const uint64_t CF_EXECUTABLE_MAGIC = 0x0045434146544143; // "CATFACE\0" as char

/// @brief executable file header representation structure
typedef struct __CfExecutableHeader {
    uint64_t magic;      ///< executable magic number
    uint64_t codeLength; ///< executable length
    CfHash   codeHash;   ///< executable bytecode hash
} CfExecutableHeader;

CfExecutableReadStatus cfExecutableRead( FILE *file, CfExecutable *dst ) {
    assert(file != NULL);
    assert(dst != NULL);

    CfExecutableHeader header;

    if (!fread(&header, sizeof(CfExecutableHeader), 1, file))
        return CF_EXECUTABLE_READ_STATUS_UNEXPECTED_FILE_END;

    if (header.magic != CF_EXECUTABLE_MAGIC)
        return CF_EXECUTABLE_READ_STATUS_INVALID_EXECUTABLE_MAGIC;

    void *code = calloc(header.codeLength, 1);
    if (code == NULL)
        return CF_EXECUTABLE_READ_STATUS_INTERNAL_ERROR;
    size_t readCount = fread(code, 1, header.codeLength, file);
    if (readCount != header.codeLength) {
        free(code);
        return CF_EXECUTABLE_READ_STATUS_UNEXPECTED_FILE_END;
    }

    CfHash hash = cfHash(code, readCount);
    if (!cfHashCompare(&hash, &header.codeHash)) {
        free(code);
        return CF_EXECUTABLE_READ_STATUS_CODE_INVALID_HASH;
    }

    dst->code = code;
    dst->codeLength = header.codeLength;

    return CF_EXECUTABLE_READ_STATUS_OK;
} // cfExecutableRead

bool cfExecutableWrite( const CfExecutable *executable, FILE *dst ) {
    assert(executable != NULL);
    assert(dst != NULL);

    const CfExecutableHeader executableHeader = {
        .magic = CF_EXECUTABLE_MAGIC,
        .codeLength = executable->codeLength,
        .codeHash = cfHash(executable->code, executable->codeLength),
    };

    return true
        && sizeof(executableHeader) == fwrite(&executableHeader, 1, sizeof(executableHeader), dst)
        && executable->codeLength == fwrite(executable->code, 1, executable->codeLength, dst)
    ;
} // cfExecutableWrite

void cfExecutableDtor( CfExecutable *executable ) {
    assert(executable != NULL);

    free(executable->code);
} // cfExecutableDtor


const char * cfExecutableReadStatusStr( CfExecutableReadStatus status ) {
    switch (status) {
    case CF_EXECUTABLE_READ_STATUS_OK                   : return "ok";
    case CF_EXECUTABLE_READ_STATUS_INTERNAL_ERROR       : return "internal error";
    case CF_EXECUTABLE_READ_STATUS_UNEXPECTED_FILE_END  : return "unexpected file end";
    case CF_EXECUTABLE_READ_STATUS_INVALID_EXECUTABLE_MAGIC : return "invalid executable magic";
    case CF_EXECUTABLE_READ_STATUS_CODE_INVALID_HASH    : return "invalid hash";

    default                                         : return "<invalid>";
    }
} // cfExecutableReadStatusStr

// cf_executable.c
