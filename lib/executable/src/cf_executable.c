#include <assert.h>
#include <stdlib.h>

#include "cf_executable.h"
#include "cf_hash.h"

/// @brief executable file magic
const uint64_t CF_EXECUTABLE_MAGIC = 0x0045434146544143; // "CATFACE\0" as char

/// @brief executable file header representation structure
typedef struct CfExecutableHeader_ {
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

bool cfExecutableWrite( FILE *const dst, const CfExecutable *const executable ) {
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

CfKey cfKeyFromUint32( const uint32_t num ) {
#define _CASE(k) case (uint32_t)(k): return (k);

    switch (num) {
    _CASE(CF_KEY_A)
    _CASE(CF_KEY_B)
    _CASE(CF_KEY_C)
    _CASE(CF_KEY_D)
    _CASE(CF_KEY_E)
    _CASE(CF_KEY_F)
    _CASE(CF_KEY_G)
    _CASE(CF_KEY_H)
    _CASE(CF_KEY_I)
    _CASE(CF_KEY_J)
    _CASE(CF_KEY_K)
    _CASE(CF_KEY_L)
    _CASE(CF_KEY_M)
    _CASE(CF_KEY_N)
    _CASE(CF_KEY_O)
    _CASE(CF_KEY_P)
    _CASE(CF_KEY_Q)
    _CASE(CF_KEY_R)
    _CASE(CF_KEY_S)
    _CASE(CF_KEY_T)
    _CASE(CF_KEY_U)
    _CASE(CF_KEY_V)
    _CASE(CF_KEY_W)
    _CASE(CF_KEY_X)
    _CASE(CF_KEY_Y)
    _CASE(CF_KEY_Z)

    _CASE(CF_KEY_0)
    _CASE(CF_KEY_1)
    _CASE(CF_KEY_2)
    _CASE(CF_KEY_3)
    _CASE(CF_KEY_4)
    _CASE(CF_KEY_5)
    _CASE(CF_KEY_6)
    _CASE(CF_KEY_7)
    _CASE(CF_KEY_8)
    _CASE(CF_KEY_9)

    _CASE(CF_KEY_ENTER        )
    _CASE(CF_KEY_BACKSPACE    )
    _CASE(CF_KEY_MINUS        )
    _CASE(CF_KEY_EQUAL        )
    _CASE(CF_KEY_DOT          )
    _CASE(CF_KEY_COMMA        )
    _CASE(CF_KEY_SLASH        )
    _CASE(CF_KEY_BACKSLASH    )
    _CASE(CF_KEY_QUOTE        )
    _CASE(CF_KEY_BACKQUOTE    )
    _CASE(CF_KEY_TAB          )
    _CASE(CF_KEY_LEFT_BRACKET )
    _CASE(CF_KEY_RIGHT_BRACKET)
    _CASE(CF_KEY_SPACE        )
    _CASE(CF_KEY_SEMICOLON    )

    _CASE(CF_KEY_UP   )
    _CASE(CF_KEY_DOWN )
    _CASE(CF_KEY_LEFT )
    _CASE(CF_KEY_RIGHT)

    _CASE(CF_KEY_SHIFT )
    _CASE(CF_KEY_ALT   )
    _CASE(CF_KEY_CTRL  )
    _CASE(CF_KEY_DELETE)
    _CASE(CF_KEY_ESCAPE)
    }

#undef _CASE

    return CF_KEY_NULL;
} // cfKeyFromUint32

// cf_executable.c
