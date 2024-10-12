/**
 * @brief object to string conversion implementation module
 */

#include "cf_asm.h"

#include <assert.h>

const char * cfAssemblyStatusStr( const CfAssemblyStatus status ) {
    switch (status) {
    case CF_ASSEMBLY_STATUS_OK                  : return "ok";
    case CF_ASSEMBLY_STATUS_INTERNAL_ERROR      : return "internal error";
    case CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION : return "unknown instruction";
    case CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END : return "unexpected text end";

    default                                     : return "<invalid>";
    }
} // cfAssemblyStatusStr

const char * cfDisassemblyStatusStr( const CfDisassemblyStatus status ) {
    switch (status) {
    case CF_DISASSEMBLY_STATUS_OK                  : return "ok";
    case CF_DISASSEMBLY_STATUS_INTERNAL_ERROR      : return "internal error";
    case CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE      : return "unknown opcode";
    case CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END : return "unexpected code end";

    default                                        : return "<invalid>";
    }
} // cfDisassemblyStatusStr


void cfAssemblyDetailsDump(
    FILE *const               out,
    const CfAssemblyStatus    status,
    const CfAssemblyDetails * details
) {
    assert(out != NULL);
    assert(details != NULL);

    const char *str = cfAssemblyStatusStr(status);

    if (status == CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION) {
        fprintf(out, "unknown insturction: %.*s",
            (int)(details->unknownInstruction.lineEnd - details->unknownInstruction.lineBegin),
            details->unknownInstruction.lineBegin
        );
    } else {
        fprintf(out, "%s", str);
    }
} // cfAssemblyDetailsDump

void cfDisassemblyDetailsDump(
    FILE *const                  out,
    const CfDisassemblyStatus    status,
    const CfDisassemblyDetails * details
) {
    assert(out != NULL);
    assert(details != NULL);

    const char *str = cfDisassemblyStatusStr(status);

    if (status == CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE) {
        fprintf(out, "unknown opcode: 0x%4X", (int)details->unknownOpcode.opcode);
    } else {
        fprintf(out, "%s", str);
    }
} // cfDisassemblyDetailsDump

// cf_asm_str.h
