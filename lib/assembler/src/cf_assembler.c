/**
 * @brief assembler implemetnation file
 */

#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include <cf_darr.h>

#include "cf_assembler.h"

/// @brief token type (actually, token tag)
typedef enum CfAssemblerTokenType_ {
    CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET,  ///< '['
    CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET, ///< ']'
    CF_ASSEMBLER_TOKEN_TYPE_PLUS,                 ///< '+'
    CF_ASSEMBLER_TOKEN_TYPE_COLON,                ///< ':'
    CF_ASSEMBLER_TOKEN_TYPE_EQUAL,                ///< '='
    CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER,           ///< identifier
    CF_ASSEMBLER_TOKEN_TYPE_FLOATING,             ///< floating point number
    CF_ASSEMBLER_TOKEN_TYPE_INTEGER,              ///< integer number
    CF_ASSEMBLER_TOKEN_TYPE_END,                  ///< '\0'
} CfAssemblerTokenType;

/// @brief assembler single token representation tagged union
typedef struct CfAssemblerToken_ {
    CfAssemblerTokenType type; ///< token type
    CfStrSpan            span; ///< token str

    union {
        CfStr   identifier; ///< identifier
        double  floating;   ///< floating point number
        int64_t integer;    ///< integer number
    };
} CfAssemblerToken;

/// @brief assembler representation structure
typedef struct CfAssembler_ {
    CfObjectBuilder  * objectBuilder;       ///< object builder

    CfStr              textRest;            ///< rest of text to be parsed
    size_t             lineIndex;           ///< current line index
    CfStr              lineContents;        ///< line contents

    CfDarr             tokenArray;          ///< token array
    CfAssemblerToken * currentToken;        ///< current parsed token

    CfDarr             errors;              ///< assembling errors
    jmp_buf            errorBuffer;         ///< error commiting buffer
    jmp_buf            internalErrorBuffer; ///< finishing buffer
} CfAssembler;

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
 * @brief check if character can belong to identifier
 * 
 * @param[in] ch character to check
 * 
 * @return true if can, false otherwise
 */
static bool cfAssemblerIsIdentifierCharacter( const char ch ) {
    return
        (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        (ch == '_')
    ;
} // cfAssemblerIsIdentifierCharacter


/**
 * @brief execution finishing function
 * 
 * @param[in,out] self   assembler pointer
 * @param[in]     status assembling status
 * 
 * @note this function will not return.
 */
void cfAssemblerInternalError( CfAssembler *const self ) {
    assert(self != NULL);
    longjmp(self->internalErrorBuffer, true);
} // cfAssemblerInternalError

/**
 * @brief assertion function
 * 
 * @param[in] self      assembler pointer
 * @param[in] condition condition
 * 
 * @note throws INTERNAL_ERROR in failure case.
 */
static void cfAssemblerAssert( CfAssembler *const self, bool condition ) {
    if (!condition)
        cfAssemblerInternalError(self);
} // cfAssemblerAssert

/**
 * @brief throw assembler error
 */
static void cfAssemblerError( CfAssembler *const self, CfAssemblyError error ) {
    assert(self != NULL);

    error.lineIndex = self->lineIndex;
    error.lineContents = self->lineContents;

    cfAssemblerAssert(self, cfDarrPush(&self->errors, &error) == CF_DARR_OK);

    longjmp(self->errorBuffer, 1);
} // cfAssemblerError

/**
 * @brief tokenize text
 * 
 * @param[in] text text to tokenize
 * @param[in] dst  tokenization destination
 * 
 * @note throws unexpected character error
 */
static void cfAssemblerTokenizeLine( CfAssembler *const self ) {
    CfStr rest = self->lineContents;

    for (;;) {
        // skip inline spaces
        while (rest.begin < rest.end && cfAssemblerIsInlineSpace(*rest.begin))
            rest.begin++;

        // skip comment
        if (rest.begin < rest.end && *rest.begin == ';')
            while (rest.begin < rest.end && *rest.begin != '\n')
                rest.begin++;

        CfAssemblerToken token = { CF_ASSEMBLER_TOKEN_TYPE_END };
        size_t spanStart = rest.begin - self->lineContents.begin;

        do {

            if (rest.begin == rest.end) {
                token.type = CF_ASSEMBLER_TOKEN_TYPE_END;
                break;
            }

            char first = *rest.begin;

            {
                bool parsed = true;

                switch (first) {
                case '['  : token.type = CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET;  break;
                case ']'  : token.type = CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET; break;
                case ':'  : token.type = CF_ASSEMBLER_TOKEN_TYPE_COLON;                break;
                case '+'  : token.type = CF_ASSEMBLER_TOKEN_TYPE_PLUS;                 break;
                case '='  : token.type = CF_ASSEMBLER_TOKEN_TYPE_EQUAL;                break;
                default:
                    parsed = false;
                }

                if (parsed) {
                    rest.begin++;
                    break;
                }
            }

            if (first >= '0' && first <= '9') {
                if (rest.begin + 1 < rest.end && rest.begin[1] == 'x') {
                    // parse hexadecimal
                    uint64_t res;
                    rest = cfStrParseHexadecmialInteger(
                        (CfStr){rest.begin + 2, rest.end},
                        &res
                    );

                    token.type = CF_ASSEMBLER_TOKEN_TYPE_INTEGER;
                    token.integer = res;
                } else {
                    // parse decimal
                    CfParsedDecimal res;

                    rest = cfStrParseDecimal(rest, &res);
                    if (res.exponentStarted || res.fractionalStarted) {
                        token.type = CF_ASSEMBLER_TOKEN_TYPE_FLOATING;
                        token.floating = cfParsedDecimalCompose(&res);
                    } else {
                        token.type = CF_ASSEMBLER_TOKEN_TYPE_INTEGER;
                        token.integer = res.integer;
                    }
                }

                break;
            }

            if (first >= 'a' && first <= 'z' || first >= 'A' && first <= 'Z' || first == '_') {
                CfStr identifier = {rest.begin, rest.begin};

                while (identifier.end < rest.end && cfAssemblerIsIdentifierCharacter(*identifier.end))
                    identifier.end++;

                token.type = CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER;
                token.identifier = identifier;
                rest.begin = identifier.end;

                break;
            }

            // unknown token
            cfAssemblerError(self, (CfAssemblyError) {
                .kind = CF_ASSEMBLY_ERROR_KIND_UNEXPECTED_CHARACTER,
                .unexpectedCharacter = rest.begin,
            });
        } while (false);

        token.span = (CfStrSpan) {
            (uint32_t)spanStart,
            (uint32_t)(rest.begin - self->lineContents.begin)
        };

        cfAssemblerAssert(self, cfDarrPush(&self->tokenArray, &token) == CF_DARR_OK);

        // end on end token
        if (token.type == CF_ASSEMBLER_TOKEN_TYPE_END)
            break;
    }
} // cfAssemblerTokenizeLine

/**
 * @brief assembler opcode getting function
 * 
 * @param[in]     identifier identifier to parse opcode from
 * @param[out]    dst        opcode parsing destination (non-null)
 * 
 * @return true if parsed, false if not
 */
static bool cfAssemblerParseOpcode( CfStr identifier, CfOpcode *dst ) {
    assert(dst != NULL);

#define OPCODE_HASH(name) (0        \
        | ((uint32_t)name[0] <<  0) \
        | ((uint32_t)name[1] <<  8) \
        | ((uint32_t)name[2] << 16) \
        | ((uint32_t)name[3] << 24) \
    )

    // 'switch' refactored to set of stuctures because by portability reasons
    static const struct CfOpcodeTableElement_ {
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
    // to read 4 bytes from identifier, that technically MAY BE incorrect.

    // calculate opcode hash
    const uint32_t opcodeHash = OPCODE_HASH(identifier.begin)
        & (uint32_t)~((~(uint64_t)0) << ((identifier.end - identifier.begin) * 8))
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
 * @brief register from identifier parsing function
 * 
 * @param[in]     identifier identifier to parse register from
 * @param[out]    regDst     register parsing destination
 * 
 * @return true if parsed, false if not
 */
static bool cfAssemblerParseRegister( CfStr identifier, uint8_t *const regDst ) {
    if (identifier.end - identifier.begin != 2)
        return false;

    const uint16_t regHash = *(const uint16_t *)identifier.begin;

#define _REG_CASE(name, value) \
    do { if (regHash == *(const uint16_t *)(name)) { *regDst = (value); return true; } } while (false)

    _REG_CASE("cz", CF_REGISTER_CZ);
    _REG_CASE("fl", CF_REGISTER_FL);
    _REG_CASE("ax", CF_REGISTER_AX);
    _REG_CASE("bx", CF_REGISTER_BX);
    _REG_CASE("cx", CF_REGISTER_CX);
    _REG_CASE("dx", CF_REGISTER_DX);
    _REG_CASE("ex", CF_REGISTER_EX);
    _REG_CASE("fx", CF_REGISTER_FX);

#undef _REG_CASE

    return false;
} // cfAssemblerParseRegister

/**
 * @brief assembler output writing function
 * 
 * @param[in,out] self assembler pointer
 * @param[in]     data data to write pointer (non-null)
 * @param[in]     size size of data block to write
 */
static void cfAssemblerWrite( CfAssembler *const self, const void *data, const uint32_t size ) {
    cfAssemblerAssert(self, cfObjectBuilderAddCode(self->objectBuilder, data, size));
} // cfAssemblerWrite

/// @brief pushPopInfo full description
typedef struct CfAssemblerPushPopInfoData_ {
    CfPushPopInfo info; ///< pushPopInfo

    ///< true if 'literal' value is valid in immediate, false if 'label'.
    ///< This field is ok to read only in case if pushPopInfo.doReadImmediate is set to 1.
    bool immediateIsLiteral;

    union {
        char     label[CF_LABEL_MAX]; ///< immediate is reference to some constant
        uint32_t literal;             ///< immedidate is literal
    } immediate; ///< immediate value storage representation union
} CfAssemblerPushPopInfoData;

void cfAssemblerValidateLabel( CfAssembler *const self, CfStr label ) {
    if (cfStrLength(label) >= CF_LABEL_MAX)
        cfAssemblerError(self, (CfAssemblyError) {
            .kind = CF_ASSEMBLY_ERROR_KIND_TOO_LONG_LABEL,
            .tooLongLabel = (CfStrSpan) {
                (uint32_t)(label.begin - self->lineContents.begin),
                (uint32_t)(label.end - self->lineContents.begin),
            },
        });
} // cfAssemblerCheckLabel

/**
 * @brief push/pop info immediate from token parsing function
 * 
 * @param[in]  self  assembler pointer
 * @param[in]  token token to parse immediate from
 * @param[out] data  parsing destination
 * 
 * @return true if parsed, false if not.
 */
static bool cfAssemblerParsePushPopInfoImmediate(
    CfAssembler                *const self,
    const CfAssemblerToken     *const token,
    CfAssemblerPushPopInfoData *const data
) {
    switch (token->type) {
    case CF_ASSEMBLER_TOKEN_TYPE_INTEGER: {
        data->immediateIsLiteral = true;
        data->immediate.literal = token->integer;

        return true;
    }

    case CF_ASSEMBLER_TOKEN_TYPE_FLOATING: {
        data->immediateIsLiteral = true;
        *(float *)&data->immediate.literal = token->floating;

        return true;
    }

    case CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER: {
        const size_t length = cfStrLength(token->identifier);

        cfAssemblerValidateLabel(self, token->identifier);

        data->immediateIsLiteral = false;

        memcpy(data->immediate.label, token->identifier.begin, length);
        memset(data->immediate.label + length, 0, CF_LABEL_MAX - length);

        return true;
    }

    default:
        return false;
    }
} // cfAssemblerParsePushPopInfoImmediate

/**
 * @brief push/pop info or register from token parsing function
 * 
 * @param[in]  self  assembler pointer
 * @param[in]  token token to read
 * @param[out] data  reading destination
 */
static void cfAssemblerParsePushPopInfoImmediateOrRegister(
    CfAssembler                *const self,
    const CfAssemblerToken     *const token,
    CfAssemblerPushPopInfoData *const data
) {
    uint8_t registerIndex = 0;

    if (true
        && token->type == CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER
        && cfAssemblerParseRegister(token->identifier, &registerIndex)
    ) {
        data->info.doReadImmediate = false;
        data->info.registerIndex = registerIndex;
    } else if (cfAssemblerParsePushPopInfoImmediate(self, token, data)) {
        data->info.doReadImmediate = true;
        data->info.registerIndex = 0;
    } else {
        cfAssemblerError(self, (CfAssemblyError) { CF_ASSEMBLY_ERROR_KIND_INVALID_PUSHPOP_ARGUMENT });
    }
} // cfAssemblerParsePushPopInfoImmediateOrRegister

/**
 * @brief push/pop info parsing function
 *
 * @param[in] self assembler poniter
 * @param[in] data push/pop info full description pointer
 */
static void cfAssemblerParsePushPopInfo(
    CfAssembler *const self,
    CfAssemblerPushPopInfoData *const data
) {
    CfAssemblerToken *tokens = self->currentToken;
    uint32_t tokenCount = (CfAssemblerToken *)cfDarrData(self->tokenArray) + cfDarrLength(self->tokenArray) - self->currentToken;

    if (tokenCount == 5) {
        // validate token types
        if (false
            || tokens[0].type != CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET
            || tokens[1].type != CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER
            || tokens[2].type != CF_ASSEMBLER_TOKEN_TYPE_PLUS
            || !cfAssemblerParsePushPopInfoImmediate(self, &tokens[3], data)
            || tokens[4].type != CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET
        ) {
            cfAssemblerError(self, (CfAssemblyError) {
                .kind = CF_ASSEMBLY_ERROR_KIND_INVALID_PUSHPOP_ARGUMENT,
            });
        }

        uint8_t regNum = 0;
        if (!cfAssemblerParseRegister(tokens[1].identifier, &regNum))
            cfAssemblerError(self, (CfAssemblyError) {
                .kind = CF_ASSEMBLY_ERROR_KIND_UNKNOWN_REGISTER,
                .unknownRegister = tokens[1].span,
            });

        data->info.isMemoryAccess = true;
        data->info.doReadImmediate = true;
        data->info.registerIndex = regNum;

        self->currentToken += 5;

        return;
    }

    if (tokenCount == 3) {
        if (true
            && tokens[0].type == CF_ASSEMBLER_TOKEN_TYPE_LEFT_SQUARE_BRACKET
            && tokens[2].type == CF_ASSEMBLER_TOKEN_TYPE_RIGHT_SQUARE_BRACKET
        ) {
            data->info.isMemoryAccess = true;
            cfAssemblerParsePushPopInfoImmediateOrRegister(self, &tokens[1], data);
        } else if (true
            && tokens[0].type == CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER
            && tokens[1].type == CF_ASSEMBLER_TOKEN_TYPE_PLUS
            && cfAssemblerParsePushPopInfoImmediate(self, &tokens[2], data)
        ) {
            uint8_t registerIndex = 0;

            if (!cfAssemblerParseRegister(tokens[0].identifier, &registerIndex))
                cfAssemblerError(self, (CfAssemblyError) {
                    .kind = CF_ASSEMBLY_ERROR_KIND_INVALID_PUSHPOP_ARGUMENT,
                });

            data->info.isMemoryAccess = false;
            data->info.doReadImmediate = true;
            data->info.registerIndex = registerIndex;
        } else {
            cfAssemblerError(self, (CfAssemblyError) {
                .kind = CF_ASSEMBLY_ERROR_KIND_INVALID_PUSHPOP_ARGUMENT,
            });
        }

        self->currentToken += 3;
        return;
    }

    if (tokenCount == 1) {
        data->info.isMemoryAccess = false;
        cfAssemblerParsePushPopInfoImmediateOrRegister(self, &tokens[0], data);
        self->currentToken += 1;
        return;
    }

    cfAssemblerError(self, (CfAssemblyError) {
        .kind = CF_ASSEMBLY_ERROR_KIND_INVALID_PUSHPOP_ARGUMENT,
    });
} // cfAssemblerParsePushPopInfo

/**
 * @brief parse instruction
 * 
 * @param[in] self   assembler pointer
 * @param[in] opcode opcode to parse instruction starting from
 */
void cfAssemblerParseInstruction( CfAssembler *const self, CfOpcode opcode ) {
    uint8_t instructionData[32] = {0};
    uint32_t instructionSize = 0;

    switch (opcode) {
    case CF_OPCODE_SYSCALL: {
        CfAssemblerToken argumentToken;

        if (self->currentToken->type != CF_ASSEMBLER_TOKEN_TYPE_INTEGER)
            cfAssemblerError(self, (CfAssemblyError) {
                .kind = CF_ASSEMBLY_ERROR_KIND_INVALID_SYSCALL_ARGUMENT,
                .invalidSyscallArgument = self->currentToken->span,
            });

        const uint32_t argument = argumentToken.integer;
        instructionSize = 5;
        instructionData[0] = opcode;
        memcpy(instructionData + 1, &argument, 4);

        self->currentToken++;

        break;
    }

    case CF_OPCODE_PUSH:
    case CF_OPCODE_POP: {
        CfAssemblerPushPopInfoData data = {0};

        cfAssemblerParsePushPopInfo(self, &data);

        instructionData[0] = opcode;
        instructionData[1] = *(uint8_t *)&data.info;

        if (data.info.doReadImmediate) {
            if (data.immediateIsLiteral) {
                memcpy(instructionData + 2, &data.immediate.literal, 4);
            } else {
                memset(instructionData + 2, 0xFF, 4);

                CfLink link = {
                    .sourceLine = (uint32_t)self->lineIndex,
                    .codeOffset = (uint32_t)cfObjectBuilderGetCodeLength(self->objectBuilder) + 2,
                };
                memcpy(link.label, data.immediate.label, CF_LABEL_MAX);

                cfAssemblerAssert(self, cfObjectBuilderAddLink(self->objectBuilder, &link));
            }

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
        instructionData[0] = opcode;
        instructionSize = 5;

        switch (self->currentToken->type) {
        case CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER: {
            cfAssemblerValidateLabel(self, self->currentToken->identifier);

            CfLink link = {
                .sourceLine = (uint32_t)self->lineIndex,
                .codeOffset = (uint32_t)cfObjectBuilderGetCodeLength(self->objectBuilder) + 1,
            };

            memcpy(
                link.label,
                self->currentToken->identifier.begin,
                cfStrLength(self->currentToken->identifier)
            );

            cfAssemblerAssert(self, cfObjectBuilderAddLink(self->objectBuilder, &link));
            memset(instructionData + 1, 0xFF, 4);
            self->currentToken++;
            break;
        }

        case CF_ASSEMBLER_TOKEN_TYPE_INTEGER: {
            int32_t int32 = self->currentToken->integer;
            memcpy(instructionData + 1, &int32, 4);
            self->currentToken++;
            break;
        }

        default:
            cfAssemblerError(self, (CfAssemblyError) {
                .kind = CF_ASSEMBLY_ERROR_KIND_INVALID_JUMP_ARGUMENT,
                .invalidJumpArgument = self->currentToken->span,
            });
        }


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
    cfAssemblerWrite(self, instructionData, instructionSize);
} // cfAssemblerParseInstruction

/**
 * @brief assembling starting function
 * 
 * @param[in,out] self assembler pointer
 */
void cfAssemblerRun( CfAssembler *const self_ ) {
    CfAssembler *const volatile self = self;

    for (;;) {
        // rewind for newline
        volatile int isError = setjmp(self->errorBuffer);

        if (isError) {
            // nothing to do, actually
        }

        // check for end
        if (self->textRest.begin == self->textRest.end)
            return;

        // parse new line
        CfStr newLine = { self->textRest.begin, self->textRest.begin };
        while (newLine.end < self->textRest.end && *newLine.end != '\n')
            newLine.end++;

        self->textRest.begin = newLine.end;

        self->lineContents = newLine;
        self->lineIndex++;

        // tokenize new line
        cfDarrClear(&self->tokenArray);
        cfAssemblerTokenizeLine(self);

        self->currentToken = (CfAssemblerToken *)cfDarrData(self->tokenArray);

        // no tokens in line
        if (self->currentToken->type == CF_ASSEMBLER_TOKEN_TYPE_END)
            continue;

        CfOpcode opcode = CF_OPCODE_UNREACHABLE;

        if (true
            && self->currentToken->type == CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER
            && cfAssemblerParseOpcode(self->currentToken->identifier, &opcode)
        ) {
            self->currentToken++;
            cfAssemblerParseInstruction(self, opcode);
        } else {
            if (self->currentToken->type != CF_ASSEMBLER_TOKEN_TYPE_IDENTIFIER)
                cfAssemblerError(self, (CfAssemblyError) {
                    .kind = CF_ASSEMBLY_ERROR_KIND_UNIDENTIFIED_CODE,
                });

            cfAssemblerValidateLabel(self, self->currentToken->identifier);
            self->currentToken++;

            switch (self->currentToken->type) {

            // jump label
            case CF_ASSEMBLER_TOKEN_TYPE_COLON: {
                CfLabel label = {
                    .sourceLine = (uint32_t)self->lineIndex,
                    .value      = (uint32_t)cfObjectBuilderGetCodeLength(self->objectBuilder),
                    .isRelative = true,
                };
                memcpy(label.label, self->currentToken->identifier.begin, cfStrLength(self->currentToken->identifier));

                cfAssemblerAssert(self, cfObjectBuilderAddLabel(self->objectBuilder, &label));
                self->currentToken++;
                break;
            }

            // constant
            case CF_ASSEMBLER_TOKEN_TYPE_EQUAL: {
                self->currentToken++;

                uint32_t value = 0;
                switch (self->currentToken->type) {
                case CF_ASSEMBLER_TOKEN_TYPE_INTEGER: {
                    value = self->currentToken->integer;
                    self->currentToken++;
                    break;
                }

                case CF_ASSEMBLER_TOKEN_TYPE_FLOATING: {
                    *(float *)&value = self->currentToken->floating;
                    self->currentToken++;
                    break;
                }

                default:
                    cfAssemblerError(self, (CfAssemblyError) {
                        .kind = CF_ASSEMBLY_ERROR_KIND_CONSTANT_NUMBER_EXPECTED,
                        .constantNumberExpected = self->currentToken->span,
                    });
                }

                CfLabel label = {
                    .sourceLine = (uint32_t)self->lineIndex,
                    .value      = value,
                    .isRelative = false,
                };

                memcpy(
                    label.label,
                    self->currentToken->identifier.begin,
                    cfStrLength(self->currentToken->identifier)
                );

                cfAssemblerAssert(self, cfObjectBuilderAddLabel(self->objectBuilder, &label));
                break;
            }

            default:
                cfAssemblerError(self, (CfAssemblyError) {
                    .kind = CF_ASSEMBLY_ERROR_KIND_UNIDENTIFIED_CODE,
                });
            }
        }

        if (self->currentToken->type != CF_ASSEMBLER_TOKEN_TYPE_END) {
            cfAssemblerError(self, (CfAssemblyError) {
                .kind = CF_ASSEMBLY_ERROR_KIND_NEWLINE_EXPECTED,
                .newlineExpected = (CfStrSpan) {
                    .begin = self->currentToken->span.begin,
                    .end   = (uint32_t)cfStrLength(self->lineContents),
                },
            });
        }
   }
} // cfAssemblerRun

CfAssemblyResult cfAssemble( CfStr text, CfStr sourceName ) {
    CfAssembler assembler = {
        .textRest = text,
    };

    if (false
        || (assembler.objectBuilder = cfObjectBuilderCtor()) == NULL
        || (assembler.errors = cfDarrCtor(sizeof(CfAssemblyError))) == NULL
        || (assembler.tokenArray = cfDarrCtor(sizeof(CfAssemblerToken))) == NULL
    ) {
        cfObjectBuilderDtor(assembler.objectBuilder);
        cfDarrDtor(assembler.errors);
        cfDarrDtor(assembler.tokenArray);
        return (CfAssemblyResult) { CF_ASSEMBLY_STATUS_INTERNAL_ERROR };
    }

    // setup finish buffer
    bool isInternalError = setjmp(assembler.internalErrorBuffer);

    // start parsing
    if (!isInternalError)
        cfAssemblerRun(&assembler);

    CfAssemblyError *errorArray = NULL;
    size_t errorArrayLength = cfDarrLength(assembler.errors);

    if (cfDarrLength(assembler.errors) != 0)
        if (cfDarrIntoData(assembler.errors, (void **)&errorArray) != CF_DARR_OK)
            isInternalError = true;

    CfObject *object = cfObjectBuilderEmit(assembler.objectBuilder, sourceName);
    if (object == NULL)
        isInternalError = true;

    cfObjectBuilderDtor(assembler.objectBuilder);
    cfDarrDtor(assembler.errors);
    cfDarrDtor(assembler.tokenArray);

    if (isInternalError) {
        cfObjectDtor(object);
        free(errorArray);
        return (CfAssemblyResult) { CF_ASSEMBLY_STATUS_INTERNAL_ERROR };
    }

    if (errorArrayLength != 0) {
        cfObjectDtor(object);
        return (CfAssemblyResult) {
            .status = CF_ASSEMBLY_STATUS_ERORRS_OCCURED,
            .errorsOccured = {
                .erorrArray = errorArray,
                .errorArrayLength = errorArrayLength,
            },
        };
    } else {
        free(errorArray);
        return (CfAssemblyResult) {
            .status = CF_ASSEMBLY_STATUS_OK,
            .ok = object,
        };
    }

} // cfAssemble



const char * cfAssemblyStatusStr( const CfAssemblyStatus status ) {
    switch (status) {
    case CF_ASSEMBLY_STATUS_OK                       : return "ok";
    case CF_ASSEMBLY_STATUS_INTERNAL_ERROR           : return "internal assembler error";
    case CF_ASSEMBLY_STATUS_ERORRS_OCCURED           : return "errors during assembling occured";
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
