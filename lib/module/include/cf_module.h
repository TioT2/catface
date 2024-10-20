/**
 * @brief CF binary module basic functions declaration file
 * 
 * @note actually, this file contains full 'specification' of CF binary executable format.
 */

#ifndef CF_MODULE_H_
#define CF_MODULE_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief count of registers used in vm
#define CF_REGISTER_COUNT ((size_t)8)

/// @brief screen width
#define CF_VIDEO_SCREEN_WIDTH  ((size_t)320)

/// @brief screen height
#define CF_VIDEO_SCREEN_HEIGHT ((size_t)200)

/// @brief flag register layout
typedef struct __CfRegisterFlags {
    // comparison flag family
    uint8_t cmpIsLt            : 1; ///< is less in last comparison
    uint8_t cmpIsEq            : 1; ///< is equal in last comparison

    // video flag family
    uint8_t videoStorageFormat : 3; ///< buffer output mode (text/colored text/256-color palette/RGBX TrueColor)
    uint8_t videoUpdateMode    : 1; ///< manual (kind of synchronization)/immediate (rewrite in parallel thread)

    uint8_t _placeholder0: 2;    // placeholder
    uint8_t _placeholder1: 8;    // placeholder
    uint8_t _placeholder2: 8;    // placeholder
    uint8_t _placeholder3: 8;    // placeholder
} CfRegisterFlags;

/// @brief video mode representation enumeration
typedef enum __CfVideoStorageFormat {
    CF_VIDEO_STORAGE_FORMAT_TEXT          = 0, ///< (default) just black text
    CF_VIDEO_STORAGE_FORMAT_COLORED_TEXT  = 1, ///< text with 16 colors
    CF_VIDEO_STORAGE_FORMAT_COLOR_PALETTE = 2, ///< colored, with 256-color palette
    CF_VIDEO_STORAGE_FORMAT_TRUE_COLOR    = 3, ///< RGBX mode (fourth coordinate ignored)
} CfVideoStorageFormat;

/// @brief video update mode representation enumeration
typedef enum __CfVideoUpdateMode {
    CF_VIDEO_UPDATE_MODE_IMMEDIATE = 0, ///< (default) update image on screen immediately
    CF_VIDEO_UPDATE_MODE_MANUAL    = 1, ///< update image on screen after certain only instruction call. During video memory update it's ok for image in actual window to change.
} CfVideoUpdateMode;

/// @brief register set representation structure
typedef union __CfRegisters {
    uint32_t indexed[8];    ///< registers as array

    struct {
        // read-only registers (write isn't restricted, but don't affect anything)
        uint32_t        cz; ///< constant zero register
        CfRegisterFlags fl; ///< flag register

        // general-purpose registers
        uint32_t        ax; ///< general-purpose register 'a'
        uint32_t        bx; ///< general-purpose register 'b'
        uint32_t        cx; ///< general-purpose register 'c'
        uint32_t        dx; ///< general-purpose register 'd'
        uint32_t        ex; ///< general-purpose register 'e'
        uint32_t        fx; ///< general-purpose register 'f'
    };
} CfRegisters;

/// @brief pushpop instruciton additional data layout
typedef struct __CfInstructionPushPop {
    uint8_t isMemoryAccess : 1; ///< do access memory
    uint8_t regIndex       : 3; ///< register index
    uint8_t useImm         : 1; ///< add 4-byte immediate after number
} CfInstructionPushPop;

/// @brief instruction header representation enumeration
typedef enum __CfOpcode {
    // system instructions
    CF_OPCODE_UNREACHABLE, ///< unreachable instruction, calls panic
    CF_OPCODE_SYSCALL,     ///< function by pre-defined index from without of sandbox calling function
    CF_OPCODE_HALT,        ///< program forced stopping opcode

    // i32 common instructions
    CF_OPCODE_ADD,    ///< 32-bit integer
    CF_OPCODE_SUB,    ///< 32-bit integer
    CF_OPCODE_SHL,    ///< shift 32-bit integer left
    CF_OPCODE_SHR,    ///< shift signed   i32 right
    CF_OPCODE_SAR,    ///< shift unsigned i32 right

    // i32 signed/unsigned instructions
    CF_OPCODE_IMUL,   ///< signed   i32 multiplication
    CF_OPCODE_MUL,    ///< unsigned i32 multiplication
    CF_OPCODE_IDIV,   ///< signed   i32 division
    CF_OPCODE_DIV,    ///< unsigned i32 division

    // f32 arithmetical instructions
    CF_OPCODE_FADD,   ///< f32 addition
    CF_OPCODE_FSUB,   ///< f32 substraction
    CF_OPCODE_FMUL,   ///< f32 multiplication
    CF_OPCODE_FDIV,   ///< f32 division

    // unary 32-bit instructions
    CF_OPCODE_FTOI,   ///< f32 into signed i32 conversion
    CF_OPCODE_ITOF,   ///< signed i32 into f32 conversion

    // push-pop instructions
    CF_OPCODE_PUSH,   ///< 32-bit literal pushing opcode
    CF_OPCODE_POP,    ///< 32-bit value removing opcode

    // comparison instruction family
    CF_OPCODE_CMP,  ///< unsigned integer comparison instruction
    CF_OPCODE_ICMP, ///< signed integer comparison instruction
    CF_OPCODE_FCMP, ///< floating-point comparison instruction

    // jmp instruction family
    CF_OPCODE_JMP,  ///< unconditional jump
    CF_OPCODE_JLE,  ///< jump if comparison flags correspond to <= (Jump if Less or Equal)
    CF_OPCODE_JL,   ///< jump if comparison flags correspond to <  (Jump if Less)
    CF_OPCODE_JGE,  ///< jump if comparison flags correspond to >= (Jump if Greater or Equal)
    CF_OPCODE_JG,   ///< jump if comparison flags correspond to >  (Jump if Greater)
    CF_OPCODE_JE,   ///< jump if comparison flags correspond to == (Jump if Equal)
    CF_OPCODE_JNE,  ///< jump if comparison flags correspond to != (Jump if Not Equal)

    // call/ret instructions
    CF_OPCODE_CALL, ///< calling instruction
    CF_OPCODE_RET,  ///< returning instruction

    // // key state getting function
    // CF_OPCODE_KCHK, ///< Key CHecKing function. Stack value is used as scancode, 1 pushed if key is pressed, 0 otherwise.

    // update video mode
    CF_OPCODE_VSM,  ///< Video Set Mode
    CF_OPCODE_VRS,  ///< Video Refresh Screen
} CfOpcode;

/// @brief colored character representation structure (used in coloredText video mode)
typedef struct __CfColoredCharacter {
    uint8_t character; ///< character itself
    uint8_t color;     ///< character color
} CfColoredCharacter;

/// @brief video memory layout in different modes representation union
typedef union __CfVideoMemory {
    uint32_t           trueColor  [CF_VIDEO_SCREEN_WIDTH * CF_VIDEO_SCREEN_HEIGHT]; ///< trueColor pixels
    uint8_t            text       [CF_VIDEO_SCREEN_WIDTH * CF_VIDEO_SCREEN_HEIGHT]; ///< text characters
    CfColoredCharacter coloredText[CF_VIDEO_SCREEN_WIDTH * CF_VIDEO_SCREEN_HEIGHT]; ///< colored text characters

    struct {
        uint32_t palette[256];                                            ///< color palette itself
        uint8_t  colors [CF_VIDEO_SCREEN_WIDTH * CF_VIDEO_SCREEN_HEIGHT]; ///< pixels that refers to certain colors in palette
    } colorPalette;
} CfVideoMemory;

/// @brief bytecode module represetnation structure
typedef struct __CfModule {
    void    *code;       ///< module bytecode
    size_t   codeLength; ///< module bytecode length
} CfModule;

/// @brief module reading status
typedef enum __CfModuleReadStatus {
    CF_MODULE_READ_STATUS_OK,                   ///< succeeded
    CF_MODULE_READ_STATUS_INTERNAL_ERROR,       ///< some internal error occured
    CF_MODULE_READ_STATUS_UNEXPECTED_FILE_END,  ///< unexpected end of file
    CF_MODULE_READ_STATUS_INVALID_MODULE_MAGIC, ///< invalid module magic number
    CF_MODULE_READ_STATUS_CODE_INVALID_HASH,    ///< invalid module code hash
} CfModuleReadStatus;

/// @brief push and pop instruction additional data
typedef struct __CfPushPopInfo {
    uint8_t registerIndex   : 3; ///< index of register to get value from
    uint8_t isMemoryAccess  : 1; ///< true if destination is placed in memory
    uint8_t doReadImmediate : 1; ///< is this instruction followed by 4-byte immediate
} CfPushPopInfo;

/**
 * @brief module from file reading function
 * 
 * @param[in] file   file to read module from, should allow "rb" access
 * @param[in] module module to write to file (non-null)
 * 
 * @return module reading status
 */
CfModuleReadStatus cfModuleRead( FILE *file, CfModule *dst );

/**
 * @brief module to file writing function
 * 
 * @param[in]  module module to dump to file (non-null)
 * @param[out] file   destination file, should allow "wb" access
 * 
 * @return operation status
 */
bool cfModuleWrite( const CfModule *module, FILE *dst );

/**
 * @brief module destructor
 * 
 * @param[in] module module to destroy
 */
void cfModuleDtor( CfModule *module );


/**
 * @brief module read status corresponding string getting function
 * 
 * @param[in] status module read status
 * 
 * @return corresponding string. In case of invalid status returns "<invalid>"
 */
const char * cfModuleReadStatusStr( CfModuleReadStatus status );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_MODULE_H_)

// cf_module.h
