/**
 * @brief assembler implemetnation file
 */

#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include <cf_darr.h>

#include "cf_assembler.h"

/// @brief assembler representation structure
typedef struct __CfAssembler {
    CfStr             textRest;     ///< rest of text to be processed
    CfStr             lineRest;     ///< rest of line to be processed

    CfStr             line;         ///< current line itself
    size_t            lineIndex;    ///< index of line

    CfDarr            output;       ///< assembler output data
    CfDarr            links;        ///< set of links to labels in this file
    CfDarr            labels;       ///< set of labels declared in file

    CfAssemblyDetails details;      ///< details
    CfAssemblyStatus  status;       ///< assembling status (**must not** be accessed directly)
    jmp_buf           finishBuffer; ///< finishing buffer
} CfAssembler;

/// @brief token type (actually, token tag)
typedef enum __CfAssemblerTokenType {
    CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET,  ///< '['
    CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET, ///< ']'
    CF_ASSEMBLER_TOKEN_TYPE_PLUS,                 ///< '+'
    CF_ASSEMBLER_TOKEN_TYPE_COLON,                ///< ':'
    CF_ASSEMBLER_TOKEN_TYPE_EQUAL,                ///< '='
    CF_ASSEMBLER_TOKEN_TYPE_IDENT,                ///< ident
    CF_ASSEMBLER_TOKEN_TYPE_FLOATING,             ///< floating point number
    CF_ASSEMBLER_TOKEN_TYPE_INTEGER,              ///< integer number
} CfAssemblerTokenType;

/// @brief assembler single token representation tagged union
typedef struct __CfAssemblerToken {
    CfAssemblerTokenType type; ///< token type

    union {
        CfStr   ident;    ///< ident
        double  floating; ///< floating point number
        int64_t integer;  ///< integer number
    };
} CfAssemblerToken;

/**
 * @brief checking for current character is space
 * 
 * @param[in] ch character to perform check for
 * 
 * @return true if is, false if not.
 */
static bool cfAssemblerIsInlineSpace( const char ch ) {
    return false
        || ch == ' '
        || ch == '\t'
        || ch == '\r'
    ;
} // cfAssemblerIsInlineSpace

/**
 * @brief execution finishing function
 * 
 * @param[in,out] self   assembler pointer
 * @param[in]     status assembling status
 * 
 * @note this function will not return.
 * 
 * @note this function sets 'status' and 'details.line' fields automatically,
 * so user should fill status-specific fields of detail structure only.
 */
void cfAssemblerFinish( CfAssembler *const self, const CfAssemblyStatus status ) {
    assert(self != NULL);

    self->status = status;
    self->details.line = self->lineIndex;
    self->details.contents = self->line;

    longjmp(self->finishBuffer, true);
} // cfAssemblerFinish

/**
 * @brief next line parsing function
 * 
 * @param[in,out] self assembler pointer
 * 
 * @note throws ok if next line is not present
 * (it's assumed that all instructions are located at exactly one line)
 */
void cfAssemblerNextLine( CfAssembler *const self ) {
    CfStr slice;

    do {
        if (self->textRest.begin >= self->textRest.end)
            cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_OK);

        // find line end
        const char *lineEnd = self->textRest.begin;
        while (lineEnd < self->textRest.end && *lineEnd != '\n')
            lineEnd++;

        // find comment start
        const char *commentStart = self->textRest.begin;
        while (commentStart < lineEnd && *commentStart != ';')
            commentStart++;

        slice.begin = self->textRest.begin;
        slice.end = commentStart;

        // trim leading spaces
        while (slice.begin < slice.end && cfAssemblerIsInlineSpace(*slice.begin))
            slice.begin++;

        // trim trailing spaces
        while (slice.begin < slice.end && cfAssemblerIsInlineSpace(*slice.begin))
            slice.end--;

        self->textRest.begin = lineEnd + 1;
        self->lineIndex++;
    } while (slice.end == slice.begin);

    self->lineRest = slice;
    self->line = slice;
} // cfAssemblerNextLine

/**
 * @brief check if character can belong to ident
 * 
 * @param[in] ch character to check
 * 
 * @return true if can, false otherwise
 */
static bool cfAssemblerIsIdentCharacter( const char ch ) {
    return
        (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        (ch == '_')
    ;
} // cfAssemblerIsIdentCharacter

/**
 * @brief next token from line parsing function
 * 
 * @param[in,out] self assembler reference
 * @param[out]    dst  parsing destination
 * 
 * @return true if parsed, false otherwise
 */
bool cfAssemblerNextToken( CfAssembler *const self, CfAssemblerToken *const dst ) {
    if (self->lineRest.begin >= self->lineRest.end || *self->lineRest.begin == ';')
        return false;

    const char first = *self->lineRest.begin;

    // handle single-char tokens
    {
        bool parsed = true;
        CfAssemblerTokenType tokenType;

        switch (first) {
        case '[': tokenType = CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET;  break;
        case ']': tokenType = CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET; break;
        case ':': tokenType = CF_ASSEMBLER_TOKEN_TYPE_COLON;                break;
        case '+': tokenType = CF_ASSEMBLER_TOKEN_TYPE_PLUS;                 break;
        case '=': tokenType = CF_ASSEMBLER_TOKEN_TYPE_EQUAL;                break;
        default:
            parsed = false;
        }

        if (parsed) {
            self->lineRest.begin++;
            dst->type = tokenType;
            goto cfAssemblerNextToken__ok;
        }
    }

    // parse number
    if (first >= '0' && first <= '9') {
        if (self->lineRest.begin + 1 < self->lineRest.end && self->lineRest.begin[1] == 'x') {
            // parse hexadecimal
            uint64_t res;
            self->lineRest = cfStrParseHexadecmialInteger(
                (CfStr){self->lineRest.begin + 2, self->lineRest.end},
                &res
            );

            dst->type = CF_ASSEMBLER_TOKEN_TYPE_INTEGER;
            dst->integer = res;
        } else {
            // parse decimal
            CfParsedDecimal res;

            self->lineRest = cfStrParseDecimal(self->lineRest, &res);
            if (res.exponentStarted || res.fractionalStarted) {
                dst->type = CF_ASSEMBLER_TOKEN_TYPE_FLOATING;
                dst->floating = cfParsedDecimalCompose(&res);
            } else {
                dst->type = CF_ASSEMBLER_TOKEN_TYPE_INTEGER;
                dst->integer = res.integer;
            }
        }

        goto cfAssemblerNextToken__ok;
    }

    if (first >= 'a' && first <= 'z' || first >= 'A' && first <= 'Z' || first == '_') {
        CfStr ident = {self->lineRest.begin, self->lineRest.begin};

        while (ident.end < self->lineRest.end && cfAssemblerIsIdentCharacter(*ident.end))
            ident.end++;

        dst->type = CF_ASSEMBLER_TOKEN_TYPE_IDENT;
        dst->ident = ident;
        self->lineRest.begin = ident.end;

        goto cfAssemblerNextToken__ok;
    }

    // throw error if unknown token occured

    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_TOKEN);
    return false;

    cfAssemblerNextToken__ok:

    // skip spaces
    while (true
        && self->lineRest.begin < self->lineRest.end
        && cfAssemblerIsInlineSpace(*self->lineRest.begin)
    )
        self->lineRest.begin++;

    return true;
} // cfAssemblerNextToken

/**
 * @brief assembler opcode getting function
 * 
 * @param[in]     ident ident to parse opcode from
 * @param[out]    dst   opcode parsing destination (non-null)
 * 
 * @return true if parsed, false if not
 */
static bool cfAssemblerParseOpcode( CfStr ident, CfOpcode *dst ) {
    assert(dst != NULL);

#define OPCODE_HASH(name) (0        \
        | ((uint32_t)name[0] <<  0) \
        | ((uint32_t)name[1] <<  8) \
        | ((uint32_t)name[2] << 16) \
        | ((uint32_t)name[3] << 24) \
    )

    // 'switch' refactored to set of stuctures because by portability reasons
    static const struct __CfOpcodeTableElement {
        uint32_t opcodeHash; ///< hash that corresponds to some opcode
        CfOpcode opcode;     ///< opcode itself
    } opcodeHashTable[] = {
        {OPCODE_HASH("unreachable" ), CF_OPCODE_UNREACHABLE },
        {OPCODE_HASH("syscall"     ), CF_OPCODE_SYSCALL     },
        {OPCODE_HASH("halt"        ), CF_OPCODE_HALT        },
        {OPCODE_HASH("add"         ), CF_OPCODE_ADD         },
        {OPCODE_HASH("sub"         ), CF_OPCODE_SUB         },
        {OPCODE_HASH("shl"         ), CF_OPCODE_SHL         },
        {OPCODE_HASH("shr"         ), CF_OPCODE_SHR         },
        {OPCODE_HASH("sar"         ), CF_OPCODE_SAR         },
        {OPCODE_HASH("or\0"        ), CF_OPCODE_OR          },
        {OPCODE_HASH("xor"         ), CF_OPCODE_XOR         },
        {OPCODE_HASH("and"         ), CF_OPCODE_AND         },
        {OPCODE_HASH("imul"        ), CF_OPCODE_IMUL        },
        {OPCODE_HASH("mul"         ), CF_OPCODE_MUL         },
        {OPCODE_HASH("idiv"        ), CF_OPCODE_IDIV        },
        {OPCODE_HASH("div"         ), CF_OPCODE_DIV         },
        {OPCODE_HASH("fadd"        ), CF_OPCODE_FADD        },
        {OPCODE_HASH("fsub"        ), CF_OPCODE_FSUB        },
        {OPCODE_HASH("fmul"        ), CF_OPCODE_FMUL        },
        {OPCODE_HASH("fdiv"        ), CF_OPCODE_FDIV        },
        {OPCODE_HASH("ftoi"        ), CF_OPCODE_FTOI        },
        {OPCODE_HASH("itof"        ), CF_OPCODE_ITOF        },
        {OPCODE_HASH("fsin"        ), CF_OPCODE_FSIN        },
        {OPCODE_HASH("fcos"        ), CF_OPCODE_FCOS        },
        {OPCODE_HASH("fneg"        ), CF_OPCODE_FNEG        },
        {OPCODE_HASH("fsqrt"       ), CF_OPCODE_FSQRT       },
        {OPCODE_HASH("push"        ), CF_OPCODE_PUSH        },
        {OPCODE_HASH("pop"         ), CF_OPCODE_POP         },
        {OPCODE_HASH("cmp"         ), CF_OPCODE_CMP         },
        {OPCODE_HASH("icmp"        ), CF_OPCODE_ICMP        },
        {OPCODE_HASH("fcmp"        ), CF_OPCODE_FCMP        },
        {OPCODE_HASH("jmp"         ), CF_OPCODE_JMP         },
        {OPCODE_HASH("jle"         ), CF_OPCODE_JLE         },
        {OPCODE_HASH("jl\0"        ), CF_OPCODE_JL          },
        {OPCODE_HASH("jge"         ), CF_OPCODE_JGE         },
        {OPCODE_HASH("jg\0"        ), CF_OPCODE_JG          },
        {OPCODE_HASH("je\0"        ), CF_OPCODE_JE          },
        {OPCODE_HASH("jne"         ), CF_OPCODE_JNE         },
        {OPCODE_HASH("call"        ), CF_OPCODE_CALL        },
        {OPCODE_HASH("ret"         ), CF_OPCODE_RET         },
        {OPCODE_HASH("vsm"         ), CF_OPCODE_VSM         },
        {OPCODE_HASH("vrs"         ), CF_OPCODE_VRS         },
        {OPCODE_HASH("meow"        ), CF_OPCODE_MEOW        },
        {OPCODE_HASH("time"        ), CF_OPCODE_TIME        },
        {OPCODE_HASH("mgs"         ), CF_OPCODE_MGS         },
        {OPCODE_HASH("igks"        ), CF_OPCODE_IGKS        },
        {OPCODE_HASH("iwkd"        ), CF_OPCODE_IWKD        },
    };
    static const size_t opcodeHashTableSize = sizeof(opcodeHashTable) / sizeof(opcodeHashTable[0]);

    // TODO: a bit messy solution, because it's assumed that it's ok
    // to read 4 bytes from ident, that technically MAY BE incorrect.

    // calculate opcode hash
    const uint32_t opcodeHash = OPCODE_HASH(ident.begin)
        & (uint32_t)~((~(uint64_t)0) << ((ident.end - ident.begin) * 8))
    ;

    for (uint32_t i = 0; i < opcodeHashTableSize; i++)
        if (opcodeHash == opcodeHashTable[i].opcodeHash) {
            *dst = opcodeHashTable[i].opcode;
            return true;
        }

    return false;

#undef OPCODE_HASH
} // cfAssemblerParseOpcode

/**
 * @brief assembler output writing function
 * 
 * @param[in,out] self assembler pointer
 * @param[in]     data data to write pointer (non-null)
 * @param[in]     size size of data block to write
 */
static void cfAssemblerWriteOutput( CfAssembler *const self, const void *data, const uint32_t size ) {
    if (cfDarrPushArray(&self->output, data, size) != CF_DARR_OK)
        cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INTERNAL_ERROR);
} // cfAssemblerWriteOutput

/**
 * @brief register from ident parsing function
 * 
 * @param[in,out] self   assembler pointer
 * @param[in]     ident  ident to parse register from
 * @param[out]    regDst register parsing destination
 * 
 * @return true if parsed, false if not
 */
static bool cfAssemblerParseRegister( CfAssembler *const self, CfStr ident, uint8_t *const regDst ) {
    if (ident.end - ident.begin != 2)
        return false;

    const uint16_t regHash = *(const uint16_t *)ident.begin;

#define _REG_CASE(name, value) \
    do { if (regHash == *(const uint16_t *)(name)) { *regDst = (value); return true; } } while (false)

    _REG_CASE("cz", 0);
    _REG_CASE("fl", 1);
    _REG_CASE("ax", 2);
    _REG_CASE("bx", 3);
    _REG_CASE("cx", 4);
    _REG_CASE("dx", 5);
    _REG_CASE("ex", 6);
    _REG_CASE("fx", 7);

#undef _REG_CASE

    return false;
} // cfAssemblerParseRegister

typedef enum __CfAssemblerReadValueStatus {
    CF_ASSEMBLER_READ_VALUE_STATUS_IMMEDIATE,       ///< immediate value
    CF_ASSEMBLER_READ_VALUE_STATUS_REGISTER,        ///< register index
    CF_ASSEMBLER_READ_VALUE_STATUS_LOCAL_REFERENCE,
} CfAssemblerReadValueStatus;

/**
 * @brief push/pop info parsing function
 *
 * @param[in] self    assembler poniter
 * @param[in] dstInfo push/pop info destination (non-null)
 * @param[in] dstImm  corresponding immediate value destination (non-null)
 */
static void cfAssemblerParsePushPopInfo(
    CfAssembler   *const self,
    CfPushPopInfo *const dstInfo,
    uint32_t      *const dstImm
) {
    CfAssemblerToken tokens[5] = {};
    uint32_t tokenCount = 0;

    for (uint32_t i = 0; i < 5; i++) {
        if (!cfAssemblerNextToken(self, &tokens[tokenCount]))
            break;
        tokenCount++;
    }

    if (tokenCount == 5) {
        // validate token types
        if (false
            || tokens[0].type != CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET
            || tokens[1].type != CF_ASSEMBLER_TOKEN_TYPE_IDENT
            || tokens[2].type != CF_ASSEMBLER_TOKEN_TYPE_PLUS
            || tokens[3].type != CF_ASSEMBLER_TOKEN_TYPE_INTEGER
            || tokens[4].type != CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET
        ) {
            cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
        }

        uint8_t regNum = 0;
        if (!cfAssemblerParseRegister(self, tokens[1].ident, &regNum))
            cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER);

        dstInfo->isMemoryAccess = true;
        dstInfo->doReadImmediate = true;
        dstInfo->registerIndex = regNum;

        *dstImm = tokens[3].integer;

        return;
    }

    if (tokenCount == 3) {
        if (true
            && tokens[0].type == CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET
            && tokens[2].type == CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET
        ) {
            dstInfo->isMemoryAccess = true;

            switch (tokens[1].type) {
            case CF_ASSEMBLER_TOKEN_TYPE_IDENT: {
                uint8_t registerIndex = 0;

                if (!cfAssemblerParseRegister(self, tokens[1].ident, &registerIndex))
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER);
                
                dstInfo->registerIndex = registerIndex;
                dstInfo->doReadImmediate = false;
                break;
            }

            case CF_ASSEMBLER_TOKEN_TYPE_INTEGER: {
                dstInfo->registerIndex = 0;
                dstInfo->doReadImmediate = true;

                *dstImm = tokens[1].integer;
                break;
            }

            default: {
                cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
            }
            }
        } else if (true
            && tokens[0].type == CF_ASSEMBLER_TOKEN_TYPE_IDENT
            && tokens[1].type == CF_ASSEMBLER_TOKEN_TYPE_PLUS
            && tokens[2].type == CF_ASSEMBLER_TOKEN_TYPE_INTEGER
        ) {
            uint8_t registerIndex = 0;

            if (!cfAssemblerParseRegister(self, tokens[0].ident, &registerIndex))
                cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER);

            dstInfo->isMemoryAccess = false;
            dstInfo->doReadImmediate = true;
            dstInfo->registerIndex = registerIndex;

            *dstImm = tokens[2].integer;
        } else {
            cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
        }

        return;
    }

    if (tokenCount == 1) {
        dstInfo->isMemoryAccess = false;

        switch (tokens[0].type) {
        case CF_ASSEMBLER_TOKEN_TYPE_IDENT: {
            uint8_t registerIndex = 0;

            if (!cfAssemblerParseRegister(self, tokens[0].ident, &registerIndex))
                cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER);
            
            dstInfo->registerIndex = registerIndex;
            dstInfo->doReadImmediate = false;
            break;
        }

        case CF_ASSEMBLER_TOKEN_TYPE_INTEGER: {
            dstInfo->registerIndex = 0;

            // now this assembler may be proudly called 'optimizing'
            if (tokens[0].integer == 0) {
                dstInfo->doReadImmediate = false;
            } else {
                dstInfo->doReadImmediate = true;
                *dstImm = tokens[0].integer;
            }
           break;
        }

        case CF_ASSEMBLER_TOKEN_TYPE_FLOATING: {
            dstInfo->registerIndex = 0;
            dstInfo->doReadImmediate = true;
            *(float *)dstImm = tokens[0].floating;
            break;
        }

        default: {
            cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
        }
        }
        return;
    }

    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
} // cfAssemblerParsePushPopInfo

/// @brief object-level immediate representation type
typedef enum __CfAssemblerImmType {
    CF_ASSEMBLER_IMM_TYPE_VALUE, ///< 32-bit constant
    CF_ASSEMBLER_IMM_TYPE_LINK,  ///< link to some named value
} CfAssemblerImmType;

/// @brief object-level immediate representation
typedef struct __CfAssemblerImm {
    CfAssemblerImmType type; ///< immediate type

    union {
        uint32_t value; ///< constant value
        CfStr    link;  ///< reference to some external value
    };
} CfAssemblerImm;

/**
 * @brief immediate parsing function
 * 
 * @param[in]  token token to parse imm from
 * @param[out] dst   parsing destination
 * 
 * @return true if parsed, false if not
 */
bool cfAssemblerParseImm( const CfAssemblerToken *const token, CfAssemblerImm *const dst ) {
    switch (token->type) {
    case CF_ASSEMBLER_TOKEN_TYPE_FLOATING: {
        dst->type = CF_ASSEMBLER_IMM_TYPE_VALUE;
        *(float *)&dst->value = token->floating;
        return true;
    }

    case CF_ASSEMBLER_TOKEN_TYPE_INTEGER: {
        dst->type = CF_ASSEMBLER_IMM_TYPE_VALUE;
        dst->value = token->integer;
        return true;
    }

    case CF_ASSEMBLER_TOKEN_TYPE_IDENT: {
        dst->type = CF_ASSEMBLER_IMM_TYPE_LINK;
        dst->link = token->ident;
        return true;
    }

    default:
        return false;
    }

    return false;
} // cfAssemblerParseImm

/**
 * @brief assembling starting function
 * 
 * @param[in,out] self assembler pointer
 */
void cfAssemblerRun( CfAssembler *const self ) {
    for (;;) {
        // parse next line
        cfAssemblerNextLine(self);

        // token parsing function
        CfAssemblerToken opcodeToken;

        // unknown token
        if (!cfAssemblerNextToken(self, &opcodeToken))
            continue;

        CfOpcode opcode = CF_OPCODE_UNREACHABLE;

        if (cfAssemblerParseOpcode(opcodeToken.ident, &opcode)) {
            uint8_t instructionData[32] = {0};
            uint32_t instructionSize = 0;

            switch (opcode) {
            case CF_OPCODE_SYSCALL: {
                CfAssemblerToken argumentToken;

                if (!cfAssemblerNextToken(self, &argumentToken))
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_SYSCALL_ARGUMENT_MISSING);
                if (argumentToken.type != CF_ASSEMBLER_TOKEN_TYPE_INTEGER)
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_SYSCALL_ARGUMENT);

                const uint32_t argument = argumentToken.integer;
                instructionSize = 5;
                instructionData[0] = opcode;
                memcpy(instructionData + 1, &argument, 4);

                break;
            }

            case CF_OPCODE_PUSH:
            case CF_OPCODE_POP: {
                CfPushPopInfo pushPopInfo = {0};
                uint32_t imm = 0;

//                CfAssemblerToken tokens[5] = {{}};
//                uint32_t tokenCount = 0;
//
//                // switch tokenCount
//                while (cfAssemblerNextToken(self, &tokens[tokenCount]))
//                    tokenCount++;
//
//                switch (tokenCount) {
//                case 5: { // [reg+imm]
//                    CfAssemblerImm assemblerImm = {};
//
//                    if (false
//                        || tokens[0].type != CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET
//                        || tokens[1].type != CF_ASSEMBLER_TOKEN_TYPE_IDENT
//                        || tokens[2].type != CF_ASSEMBLER_TOKEN_TYPE_PLUS
//                        || !cfAssemblerParseImm(&tokens[3], &assemblerImm)
//                        || tokens[4].type != CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET
//                    )
//                        cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
//
//                    switch (assemblerImm.type) {
//                    case CF_ASSEMBLER_IMM_TYPE_LINK: {
//                        break;
//                    }
//                    }
//
//                    pushPopInfo.doReadImmediate = true;
//                    pushPopInfo.isMemoryAccess = true;
//                    break;
//                }
//
//                case 3: { // reg+imm | [reg] | [imm]
//                    break;
//                }
//
//                case 1: { // reg | imm
//                    break;
//                }
//
//
//                default:
//                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
//                }
//
//                cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT);
//
//                _cfAssemblerRun__parse_pushpop_info_ok:

                cfAssemblerParsePushPopInfo(self, &pushPopInfo, &imm);

                instructionData[0] = opcode;
                instructionData[1] = *(uint8_t *)&pushPopInfo;

                if (pushPopInfo.doReadImmediate) {
                    memcpy(instructionData + 2, &imm, 4);
                    instructionSize = 6;
                } else {
                    instructionSize = 2;
                }

                break;
            }

            case CF_OPCODE_JMP:
            case CF_OPCODE_JLE:
            case CF_OPCODE_JL:
            case CF_OPCODE_JGE:
            case CF_OPCODE_JG:
            case CF_OPCODE_JE:
            case CF_OPCODE_JNE:
            case CF_OPCODE_CALL: {
                CfAssemblerToken labelToken = {};

                if (!cfAssemblerNextToken(self, &labelToken))
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_JUMP_ARGUMENT_MISSING);
                
                if (labelToken.type != CF_ASSEMBLER_TOKEN_TYPE_IDENT)
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_JUMP_ARGUMENT);

                // separate in function?
                if (cfStrLength(labelToken.ident) >= CF_LABEL_MAX)
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_TOO_LONG_LABEL);
                CfLink link = {
                    .sourceLine = (uint32_t)self->lineIndex,
                    .codeOffset = (uint32_t)cfDarrSize(self->output) + 1,
                };
                memcpy(link.label, labelToken.ident.begin, cfStrLength(labelToken.ident));

                if (cfDarrPush(&self->links, &link) != CF_DARR_OK)
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INTERNAL_ERROR);

                instructionSize = 5;

                instructionData[0] = opcode;
                memset(instructionData + 1, 0xFF, 4);

                break;
            }

            case CF_OPCODE_UNREACHABLE:
            case CF_OPCODE_HALT:
            case CF_OPCODE_ADD:
            case CF_OPCODE_SUB:
            case CF_OPCODE_SHL:
            case CF_OPCODE_SHR:
            case CF_OPCODE_SAR:
            case CF_OPCODE_OR:
            case CF_OPCODE_XOR:
            case CF_OPCODE_AND:
            case CF_OPCODE_IMUL:
            case CF_OPCODE_MUL:
            case CF_OPCODE_IDIV:
            case CF_OPCODE_DIV:
            case CF_OPCODE_FADD:
            case CF_OPCODE_FSUB:
            case CF_OPCODE_FMUL:
            case CF_OPCODE_FDIV:
            case CF_OPCODE_FTOI:
            case CF_OPCODE_ITOF:
            case CF_OPCODE_FSIN:
            case CF_OPCODE_FCOS:
            case CF_OPCODE_FNEG:
            case CF_OPCODE_FSQRT:
            case CF_OPCODE_CMP:
            case CF_OPCODE_ICMP:
            case CF_OPCODE_FCMP:
            case CF_OPCODE_RET:
            case CF_OPCODE_VSM:
            case CF_OPCODE_VRS:
            case CF_OPCODE_MEOW:
            case CF_OPCODE_TIME:
            case CF_OPCODE_IGKS:
            case CF_OPCODE_IWKD:
            case CF_OPCODE_MGS: {
                instructionSize = 1;
                instructionData[0] = opcode;
                break;
            }
            }

            // write instruction to instruction stream
            cfAssemblerWriteOutput(self, instructionData, instructionSize);
        } else {

            CfAssemblerToken colonToken = {};

            // TODO uuugh
            if (!cfAssemblerNextToken(self, &colonToken))
                cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION);

            if (cfStrLength(opcodeToken.ident) >= CF_LABEL_MAX)
                cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_TOO_LONG_LABEL);

            switch (colonToken.type) {

            // jump label
            case CF_ASSEMBLER_TOKEN_TYPE_COLON: {
                CfLabel label = {
                    .sourceLine = (uint32_t)self->lineIndex,
                    .value      = (uint32_t)cfDarrSize(self->output),
                    .isRelative = true,
                };

                memcpy(label.label, opcodeToken.ident.begin, cfStrLength(opcodeToken.ident));

                if (cfDarrPush(&self->labels, &label) != CF_DARR_OK)
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INTERNAL_ERROR);
                break;
            }

            // constant
            case CF_ASSEMBLER_TOKEN_TYPE_EQUAL: {
                CfAssemblerToken valueToken = {};

                if (!cfAssemblerNextToken(self, &valueToken))
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_CONSTANT_VALUE);

                uint32_t value = 0;
                switch (valueToken.type) {
                case CF_ASSEMBLER_TOKEN_TYPE_INTEGER: {
                    value = valueToken.integer;
                    break;
                }

                case CF_ASSEMBLER_TOKEN_TYPE_FLOATING: {
                    *(float *)&value = valueToken.floating;
                    break;
                }

                default:
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INVALID_CONSTANT_VALUE);
                }

                CfLabel label = {
                    .sourceLine = (uint32_t)self->lineIndex,
                    .value      = value,
                    .isRelative = false,
                };

                memcpy(label.label, opcodeToken.ident.begin, cfStrLength(opcodeToken.ident));

                if (cfDarrPush(&self->labels, &label) != CF_DARR_OK)
                    cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_INTERNAL_ERROR);
                break;
            }

            default:
                cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION);
            }
        }

       // check for all line text being processed.
        if (self->lineRest.begin < self->lineRest.end)
            cfAssemblerFinish(self, CF_ASSEMBLY_STATUS_UNEXPECTED_CHARACTERS);
   }
} // cfAssemblerRun

CfAssemblyStatus cfAssemble( CfStr text, CfStr sourceName, CfObject *dst, CfAssemblyDetails *details ) {
    assert(dst != NULL);

    CfAssembler assembler = {0};

    // setup finish buffer
    int jmp = setjmp(assembler.finishBuffer);
    if (jmp) {
        if (assembler.status != CF_ASSEMBLY_STATUS_OK)
            goto cfAssembler__cleanup;

        // should I actually do it?
        memset(dst, 0, sizeof(CfObject));

        dst->codeLength = cfDarrSize(assembler.output);
        dst->linkCount = cfDarrSize(assembler.links);
        dst->labelCount = cfDarrSize(assembler.labels);

        if (false
            || cfDarrIntoData(assembler.output, (void **)&dst->code)   != CF_DARR_OK
            || cfDarrIntoData(assembler.links, (void **)&dst->links)   != CF_DARR_OK
            || cfDarrIntoData(assembler.labels, (void **)&dst->labels) != CF_DARR_OK
            || (dst->sourceName = cfStrOwnedCopy(sourceName)) == NULL // allowed by function definition
        ) {
            free(dst->code);
            free(dst->links);
            free(dst->labels);
            free((char *)dst->sourceName);

            assembler.status = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
        }

        goto cfAssembler__cleanup;
    }

    // setup assembler fields
    assembler.textRest = text;
    assembler.output = cfDarrCtor(1);
    assembler.links = cfDarrCtor(sizeof(CfLink));
    assembler.labels = cfDarrCtor(sizeof(CfLabel));

    if (false
        || assembler.output == NULL
        || assembler.links  == NULL
        || assembler.labels == NULL
    )
        goto cfAssembler__cleanup;

    // start parsing
    cfAssemblerRun(&assembler);

    // cfAssemblerRun function MUST NOT finish it's execution
    assert(false && "assembler fatal internal error occured");

cfAssembler__cleanup:

    cfDarrDtor(assembler.output);
    cfDarrDtor(assembler.links);
    cfDarrDtor(assembler.labels);

    if (details != NULL)
        *details = assembler.details;
    return assembler.status;
} // cfAssemble

const char * cfAssemblyStatusStr( const CfAssemblyStatus status ) {
    switch (status) {
    case CF_ASSEMBLY_STATUS_OK                       : return "ok";
    case CF_ASSEMBLY_STATUS_INTERNAL_ERROR           : return "internal error";
    case CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION      : return "unknown instruction";
    case CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END      : return "unexpected text end";
    case CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER         : return "unknown register";
    case CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT : return "invalid pushpop argument";
    case CF_ASSEMBLY_STATUS_UNKNOWN_OPCODE           : return "unknown opcode";
    case CF_ASSEMBLY_STATUS_UNKNOWN_TOKEN            : return "unknown token";
    case CF_ASSEMBLY_STATUS_INVALID_SYSCALL_ARGUMENT : return "invalid syscall argument";
    case CF_ASSEMBLY_STATUS_SYSCALL_ARGUMENT_MISSING : return "syscall argument missing";
    case CF_ASSEMBLY_STATUS_INVALID_JUMP_ARGUMENT    : return "invalid jump argument";
    case CF_ASSEMBLY_STATUS_JUMP_ARGUMENT_MISSING    : return "jump argument missing";
    case CF_ASSEMBLY_STATUS_EMPTY_LABEL              : return "label is empty";
    case CF_ASSEMBLY_STATUS_TOO_LONG_LABEL           : return "label is too long";
    case CF_ASSEMBLY_STATUS_INVALID_CONSTANT_VALUE   : return "invalid constant value";
    case CF_ASSEMBLY_STATUS_UNEXPECTED_CHARACTERS    : return "unexpected characters";
    }

    return "<invalid>";
} // cfAssemblyStatusStr

void cfAssemblyDetailsWrite(
    FILE *const               out,
    const CfAssemblyStatus    status,
    const CfAssemblyDetails * details
) {
    assert(out != NULL);
    assert(details != NULL);

    const char *str = cfAssemblyStatusStr(status);
    fprintf(out, "%s at %zu: \"", cfAssemblyStatusStr(status), details->line);
    cfStrWrite(out, details->contents);
    fprintf(out, "\"");
} // cfAssemblyDetailsWrite

// cf_assembler.c
