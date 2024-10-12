/**
 * @brief CfModule to CfAsm and CfAsm to CfModule converters implementation module
 */

#ifndef CF_ASM_H_
#define CF_ASM_H_

#include <cf_module.h>

/// Assembling status
typedef enum __CfAssemblyStatus {
    CF_ASSEMBLY_STATUS_OK,                  ///< all's ok
    CF_ASSEMBLY_STATUS_INTERNAL_ERROR,      ///< internal error (e.g. error that shouldn't occur in normal situation, such as memory allocation failure etc.)
    CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION, ///< unknown instruction occured
    CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END, ///< unexpected text end (missing something)
} CfAssemblyStatus;

/// @brief detailed info about assembling process
typedef union __CfAssemblyDetails {
    struct {
        const char *lineBegin; ///< begin of line unknown instruction occured
        const char *lineEnd;   ///< end of line unknown instruction occured
    } unknownInstruction;
} CfAssemblyDetails;

/**
 * @brief text assembling function
 * 
 * @param[in]  text       code to assembly (non-null)
 * @param[in]  textLength code length
 * @param[out] dst        assembling destination (non-null)
 * @param[out] details    detailed info about assembling process (nullable)
 * 
 * @return assembling status
 */
CfAssemblyStatus cfAssemble( const char *text, size_t textLength, CfModule *dst, CfAssemblyDetails *details );

/// @brief general disassembling process status
typedef enum __CfDisassemblyStatus {
    CF_DISASSEMBLY_STATUS_OK,                  ///< all's ok
    CF_DISASSEMBLY_STATUS_INTERNAL_ERROR,      ///< internal disassembling error occured
    CF_DISASSEMBLY_STATUS_UNKNOWN_OPCODE,      ///< unknown CF opcode
    CF_DISASSEMBLY_STATUS_UNEXPECTED_CODE_END, ///< unexpected code block end
} CfDisassemblyStatus;

/// @brief disassembling process detailed info
typedef union __CfDisassemblyDetails {
    struct {
        uint16_t opcode; ///< the opcode bytes
    } unknownOpcode;
} CfDisassemblyDetails;

/**
 * @brief module disassembling function
 * 
 * @param[in] module  module pointer
 * @param[in] dest    code destination
 * @param[in] details disassembling detailed info
 * 
 * @return disassembling status
 */
CfDisassemblyStatus cfDisassemble( const CfModule *module, char **dest, CfDisassemblyDetails *details );


/**
 * @brief assembly status to string conversion error
 * 
 * @param[in] status assembly status
 * 
 * @return assembly status converted to string. In case of invalid status <invalid> returned
 */
const char * cfAssemblyStatusStr( CfAssemblyStatus status );

/**
 * @brief disassembly status to string conversion error
 * 
 * @param[in] status disassembly status
 * 
 * @return disassembly status converted to string. In case of invalid status <invalid> returned
 */
const char * cfDisassemblyStatusStr( CfDisassemblyStatus status );

/**
 * @brief assembly details dumping function
 * 
 * @param[in,out] out     output file
 * @param[in]     status  assembly operation status
 * @param[in]     details corresponding assembly details (non-null)
 */
void cfAssemblyDetailsDump( FILE *out, CfAssemblyStatus status, const CfAssemblyDetails *details );

/**
 * @brief disassembly details dumping function
 * 
 * @param[in,out] out     output file
 * @param[in]     status  assembly operation status
 * @param[in]     details corresponding assembly details (non-null)
 */
void cfDisassemblyDetailsDump( FILE *out, CfDisassemblyStatus status, const CfDisassemblyDetails *details );

#endif // !defined(CF_ASM_H_)

// cf_asm.h
