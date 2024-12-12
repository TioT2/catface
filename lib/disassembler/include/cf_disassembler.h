/**
 * @brief CfExecutable to CfAsm and CfAsm to CfExecutable converters declaratoin files
 */

#ifndef CF_DISASSEMBLER_H_
#define CF_DISASSEMBLER_H_

#include <cf_executable.h>
#include <cf_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief general disassembling process status
typedef enum CfDisassemblyStatus_ {
    CF_DISASSEMBLY_STATUS_OK,                  ///< all's ok
    CF_DISASSEMBLY_STATUS_INTERNAL_ERROR,      ///< internal disassembling error occured
    CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE,      ///< unknown CF opcode
    CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END, ///< unexpected code block end
} CfDisassemblyStatus;

/// @brief disassembling process detailed info
typedef union CfDisassemblyDetails_ {
    struct {
        uint16_t opcode; ///< the opcode bytes
    } unknownOpcode;
} CfDisassemblyDetails;

/**
 * @brief exec disassembling function
 * 
 * @param[in] exec    exec pointer
 * @param[in] dest    code destination
 * @param[in] details disassembling detailed info
 * 
 * @return disassembling status
 */
CfDisassemblyStatus cfDisassemble( const CfExecutable *exec, char **dest, CfDisassemblyDetails *details );

/**
 * @brief disassembly status to string conversion error
 * 
 * @param[in] status disassembly status
 * 
 * @return disassembly status converted to string. In case of invalid status <invalid> returned
 */
const char * cfDisassemblyStatusStr( CfDisassemblyStatus status );

/**
 * @brief disassembly details dumping function
 * 
 * @param[in,out] out     output file
 * @param[in]     status  assembly operation status
 * @param[in]     details corresponding assembly details (non-null)
 */
void cfDisassemblyDetailsDump( FILE *out, CfDisassemblyStatus status, const CfDisassemblyDetails *details );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_DISASSEMBLER_H_)

// cf_asm.h
