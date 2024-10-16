/**
 * @brief object to string conversion implementation module
 */

#include "cf_asm.h"

#include <assert.h>

const char * cfAssemblyStatusStr( const CfAssemblyStatus status ) {
    switch (status) {
    case CF_ASSEMBLY_STATUS_OK                       : return "ok";
    case CF_ASSEMBLY_STATUS_INTERNAL_ERROR           : return "internal error";
    case CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION      : return "unknown instruction";
    case CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END      : return "unexpected text end";
    case CF_ASSEMBLY_STATUS_UNKNOWN_LABEL            : return "unknown label";
    case CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER         : return "unknown register";
    case CF_ASSEMBLY_STATUS_DUPLICATE_LABEL          : return "duplicate label";
    case CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT : return "invalid push/pop argument";

    default                                          : return "<invalid>";
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

    switch (status) {
    case CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION: {
        fprintf(out, "unknown instruction (at %zu): ", details->unknownInstruction.line);
        cfWriteStr(out, details->unknownInstruction.instruction);
        break;
    }

    case CF_ASSEMBLY_STATUS_DUPLICATE_LABEL: {
        fprintf(out, "duplicate label \"");
        cfWriteStr(out, details->duplicateLabel.label);
        fprintf(out, "\" at %zu (initially declared at %zu)",
            details->duplicateLabel.firstDeclaration,
            details->duplicateLabel.secondDeclaration
        );
        break;
    }

    default: {
        fprintf(out, "%s", str);
    }
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

// cf_asm_str.c
