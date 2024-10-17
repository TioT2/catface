/**
 * @brief CfModule to CfAsm and CfAsm to CfModule converters declaratoin files
 */

#ifndef CF_ASM_H_
#define CF_ASM_H_

#include <cf_module.h>
#include <cf_string.h>

#ifdef __cplusplus
extern "C" {
#endif

// registers:
// r0 zero 
// r1 ax   
// r2 bx   
// r3 cx   
// r4 dx   
// r5 vid  
// r6 
// r7 

/// @brief push and pop compressed instruction representation structure
typedef struct __CfInstructionPushPop {
    uint8_t opcode    : 8;     ///< instruciton opcode
    struct {
        uint8_t action    : 1; ///< push then 1, pop then 0
        uint8_t imm       : 2; ///< immediate space requirement, 0 => 0, 1 => 4, 2 => 8
        uint8_t reg_index : 3; ///< target register index, 0 => zero, 1 => ax, 2 => bx, 3 => cx, 4 => dx
        uint8_t access    : 1; ///< register index
        uint8_t is_next   : 1; ///< if true next desc is valid, if not - not.
    } desc[3];
} CfInstructionPushPop;

/// Assembling status
typedef enum __CfAssemblyStatus {
    CF_ASSEMBLY_STATUS_OK,                  ///< all's ok
    CF_ASSEMBLY_STATUS_INTERNAL_ERROR,      ///< internal error (e.g. error that shouldn't occur in normal situation, such as memory allocation failure etc.)
    CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION, ///< unknown instruction occured
    CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END, ///< unexpected text end (missing something)
    CF_ASSEMBLY_STATUS_UNKNOWN_DECLARATION, ///< unknown declaration
    CF_ASSEMBLY_STATUS_UNKNOWN_LABEL,       ///< unknown label
    CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER,    ///< unknown register
} CfAssemblyStatus;

/// @brief detailed info about assembling process
typedef union __CfAssemblyDetails {
    CfStringSlice unknownInstruction;
    CfStringSlice unknownDeclaration;
    CfStringSlice unknownLabel;
    CfStringSlice unknownRegister;
} CfAssemblyDetails;

/**
 * @brief text slice assembling function
 * 
 * @param[in]  text       code to assembly (valid)
 * @param[out] dst        assembling destination (non-null)
 * @param[out] details    detailed info about assembling process (nullable)
 * 
 * @return assembling status
 */
CfAssemblyStatus cfAssemble( CfStringSlice text, CfModule *dst, CfAssemblyDetails *details );

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

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_ASM_H_)

// cf_asm.h
