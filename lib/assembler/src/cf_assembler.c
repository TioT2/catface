#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <cf_darr.h>
#include <cf_string.h>

#include "cf_assembler.h"

// TODO: Rebuild assembler

bool cfAsmNextLine( CfStr *self, size_t *line, CfStr *dst ) {
    CfStr slice;

    do {
        if (self->begin >= self->end)
            return false;

        // find line end
        const char *lineEnd = self->begin;
        while (lineEnd < self->end && *lineEnd != '\n')
            lineEnd++;

        // find comment start
        const char *commentStart = self->begin;
        while (commentStart < lineEnd && *commentStart != ';')
            commentStart++;

        slice.begin = self->begin;
        slice.end = commentStart;

        // trim leading spaces
        while (slice.begin < slice.end && *slice.begin == ' ' || *slice.begin == '\t')
            slice.begin++;

        // trim trailing spaces
        while (slice.begin < slice.end && *slice.end == ' ' || *slice.end == '\t')
            slice.end--;

        self->begin = lineEnd + 1;
        (*line)++;
    } while (slice.end == slice.begin);

    *dst = slice;

    return true;
} // cfAsmNextLine

/**
 * @brief check if character can belong to ident
 */
static bool cfAsmIsIdentCharacter( const char ch ) {
    return
        (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        (ch == '_')
    ;
} // cfAsmIsIdentCharacter

typedef enum __CfAsmTokenType {
    CF_ASM_TOKEN_TYPE_IDENT,         ///< ident
    CF_ASM_TOKEN_TYPE_INTEGER,       ///< number
    CF_ASM_TOKEN_TYPE_FLOATING,      ///< floating
    CF_ASM_TOKEN_TYPE_COLON,         ///< ':' character
    CF_ASM_TOKEN_TYPE_LEFT_BRACKET,  ///< '[' character
    CF_ASM_TOKEN_TYPE_RIGHT_BRACKET, ///< ']' character
    CF_ASM_TOKEN_TYPE_PLUS,          ///< '+' character
} CfAsmTokenType;

/// @brief token tagged union
typedef struct __CfAsmToken {
    CfAsmTokenType type; ///< type

    union {
        CfStr    ident;    ///< ident slice
        uint64_t integer;  ///< integer number
        double   floating; ///< floating point number
    };
} CfAsmToken;


/**
 * @brief next token from line getting function
 * 
 * @param[in,out] line line to try to get next token from. At the function end is equal to line without token data.
 * @param[out]    dst  token parsing destination
 * 
 * @return true if token parsed, false if not
 */
static bool cfAsmNextToken( CfStr *line, CfAsmToken *dst ) {
    if (line->begin < line->end && *line->begin == ';')
        return false;

    while (line->begin < line->end && *line->begin == ' ' || *line->begin == '\t')
        line->begin++;

    if (line->begin >= line->end)
        return false;
    char first = *line->begin;

    if (first == '[') {
        line->begin++;
        dst->type = CF_ASM_TOKEN_TYPE_LEFT_BRACKET;
        return true;
    }

    if (first == ']') {
        line->begin++;
        dst->type = CF_ASM_TOKEN_TYPE_RIGHT_BRACKET;
        return true;
    }

    if (first == ':') {
        line->begin++;
        dst->type = CF_ASM_TOKEN_TYPE_COLON;
        return true;
    }

    if (first == '+') {
        line->begin++;
        dst->type = CF_ASM_TOKEN_TYPE_PLUS;
        return true;
    }

    // parse number
    if (first >= '0' && first <= '9') {
        if (line->begin + 1 < line->end && line->begin[1] == 'x') {
            // parse hexadecimal
            uint64_t res;
            *line = cfStrParseHexadecmialInteger(
                (CfStr){line->begin + 2, line->end},
                &res
            );

            dst->type = CF_ASM_TOKEN_TYPE_INTEGER;
            dst->integer = res;
            return true;
        } else {
            // parse decimal (or floating)
            CfParsedDecimal res;

            *line = cfStrParseDecimal(*line, &res);
            if (res.exponentStarted || res.fractionalStarted) {
                dst->type = CF_ASM_TOKEN_TYPE_FLOATING;
                dst->floating = cfParsedDecimalCompose(&res);
            } else {
                dst->type = CF_ASM_TOKEN_TYPE_INTEGER;
                dst->integer = res.integer;
            }

            return true;
        }
    }

    if (first >= 'a' && first <= 'z' || first >= 'A' && first <= 'Z' || first == '_') {
        CfStr ident = {line->begin, line->begin};

        while (ident.end < line->end && cfAsmIsIdentCharacter(*ident.end))
            ident.end++;

        dst->floating = CF_ASM_TOKEN_TYPE_IDENT;
        dst->ident = ident;

        line->begin = ident.end;
        return true;
    }

    // unknown token
    return false;
} // cfAsmNextToken





/**
 * @brief register from string slice parsing function
 * 
 * @param[in]  slice slice to parse register from
 * @param[out] dst   register index destination
 * 
 * @return true if succeeded, false otherwise
 */
static bool cfAsmParseRegister( CfStr slice, uint32_t *const dst ) {
    assert(dst != NULL);

    if (slice.begin + 2 != slice.end)
        return 0;

    uint16_t bs = *(const uint16_t *)slice.begin;

    *dst = 0xFFFFFFFF;

         if (bs == *(const uint16_t *)"cz") *dst = 0;
    else if (bs == *(const uint16_t *)"fl") *dst = 1;
    else if (bs == *(const uint16_t *)"ax") *dst = 2;
    else if (bs == *(const uint16_t *)"bx") *dst = 3;
    else if (bs == *(const uint16_t *)"cx") *dst = 4;
    else if (bs == *(const uint16_t *)"dx") *dst = 5;
    else if (bs == *(const uint16_t *)"ex") *dst = 6;
    else if (bs == *(const uint16_t *)"fx") *dst = 7;

    return *dst != 0xFFFFFFFF;
} // cfAsmParseRegister













/**
 * @brief push/pop info immediate value parsing function
 */
static bool cfAsmParsePushPopInfoImmediate( CfAsmToken *token, uint32_t *immDst ) {
    if (token->type != CF_ASM_TOKEN_TYPE_INTEGER && token->type != CF_ASM_TOKEN_TYPE_FLOATING)
        return false;
    if (token->type == CF_ASM_TOKEN_TYPE_INTEGER)
        *immDst = token->integer;
    else
        *(float *)immDst = token->floating;
    return true;
} // cfAsmParsePushPopInfoImmediate

/**
 * @brief push/pop info register parsing function
 */
static bool cfAsmParsePushPopInfoRegister( CfAsmToken *token, uint32_t *regIndexDst ) {
    if (token->type != CF_ASM_TOKEN_TYPE_IDENT)
        return false;
    return cfAsmParseRegister(token->ident, regIndexDst);
} // cfAsmParsePushPopInfoRegister

static bool cfAsmParsePushPopInfoRegisterAndImmediate(
    CfAsmToken *tokens,
    uint32_t *regDst,
    uint32_t *immDst
) {
    if (!cfAsmParsePushPopInfoRegister(tokens + 0, regDst))
        return false;
    if (tokens[1].type != CF_ASM_TOKEN_TYPE_PLUS)
        return false;
    if (!cfAsmParsePushPopInfoImmediate(tokens + 2, immDst))
        return false;
    return true;
} // cfAsmParsePushPopInfoRegisterAndImmediate

/**
 * @brief push and pop instructions additional data parsing function
 * 
 * @param[in]  tokenStr string to parse tokens from
 * @param[out] dst      parsing destination
 * @param[out] immDst   immediate value destination
 * 
 * @return true if succeeded, false otherwise
 */
bool cfAsmParsePushPopInfo(
    CfStr             * tokenStr,
    CfPushPopInfo     * dst,
    uint32_t          * immDst
) {
    assert(tokenStr != NULL);
    assert(dst != NULL);
    assert(immDst != NULL);

    memset(dst, 0, sizeof(CfPushPopInfo));

    CfAsmToken tokens[5] = {};
    const size_t maxTokens = 5;

    size_t tokenCount = 0;

    while (cfAsmNextToken(tokenStr, tokens + tokenCount)) {
        if (tokenCount >= maxTokens)
            return false;
        tokenCount++;
    }

    // case 1 - register / number only
    if (tokenCount == 1) {
        if (cfAsmParsePushPopInfoImmediate(&tokens[0], immDst)) {
            dst->doReadImmediate = true;
            return true;
        }

        uint32_t regIndex = 0;
        if (cfAsmParsePushPopInfoRegister(&tokens[0], &regIndex)) {
            dst->registerIndex = regIndex;
            return true;
        }

        return false;
    }

    if (tokenCount == 3) {
        if (tokens[0].type == CF_ASM_TOKEN_TYPE_LEFT_BRACKET) {
            if (tokens[2].type != CF_ASM_TOKEN_TYPE_RIGHT_BRACKET)
                return false;
            dst->isMemoryAccess = true;

            // parse register/immediate
            if (cfAsmParsePushPopInfoImmediate(&tokens[1], immDst)) {
                dst->doReadImmediate = true;
                return true;
            }

            uint32_t regDst = 0;
            if (cfAsmParsePushPopInfoRegister(&tokens[1], &regDst)) {
                dst->registerIndex = regDst;
                return true;
            }

            return false;
        }

        // parse register, plus and immediate
        uint32_t regDst;
        if (!cfAsmParsePushPopInfoRegisterAndImmediate(tokens + 0, &regDst, immDst))
            return false;
        dst->doReadImmediate = true;
        dst->registerIndex = regDst;
        return true;
    }

    if (tokenCount == 5) {
        if (tokens[0].type != CF_ASM_TOKEN_TYPE_LEFT_BRACKET || tokens[4].type != CF_ASM_TOKEN_TYPE_RIGHT_BRACKET)
            return false;

        uint32_t regDst;
        if (!cfAsmParsePushPopInfoRegisterAndImmediate(tokens + 1, &regDst, immDst))
            return false;

        dst->isMemoryAccess = true;
        dst->doReadImmediate = true;
        dst->registerIndex = regDst;
        return true;
    }

    return false;
} // cfAsmParsePushPopInfo


CfAssemblyStatus cfAssemble( CfStr text, CfStr sourceName, CfObject *dst, CfAssemblyDetails *details ) {
    assert(dst != NULL);

    CfStr line;
    size_t lineIndex = 0;
    CfDarr code = NULL;
    CfDarr links = NULL;
    CfDarr labels = NULL;

    uint8_t *resultCode;
    CfLink *resultLinks;
    CfLabel *resultLabels;
    char *resultSourceName;
    CfAssemblyStatus resultStatus = CF_ASSEMBLY_STATUS_OK;

    code = cfDarrCtor(sizeof(uint8_t));
    links = cfDarrCtor(sizeof(CfLink));
    labels = cfDarrCtor(sizeof(CfLabel));

    if (code == NULL || links == NULL || labels == NULL) {
        resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
        goto cfAssemble__end;
    }

    // parse code
    while (cfAsmNextLine(&text, &lineIndex, &line)) {
        uint8_t dataBuffer[16];
        size_t dataElementCount = 1;

        CfAsmToken token = {};
        CfStr tokenSlice = line;

        // There's no starting token in line
        if (!cfAsmNextToken(&tokenSlice, &token))
            continue;

        if (token.type != CF_ASM_TOKEN_TYPE_IDENT) {
            if (details != NULL) {

                details->unknownInstruction.instruction = line;
                details->unknownInstruction.line = lineIndex;
            }
            resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
            goto cfAssemble__end;
        }

        // then try to parse opcode
               if (cfStrStartsWith(token.ident, "add")) {
            dataBuffer[0] = CF_OPCODE_ADD;
        } else if (cfStrStartsWith(token.ident, "sub")) {
            dataBuffer[0] = CF_OPCODE_SUB;
        } else if (cfStrStartsWith(token.ident, "shl")) {
            dataBuffer[0] = CF_OPCODE_SHL;
        } else if (cfStrStartsWith(token.ident, "imul")) {
            dataBuffer[0] = CF_OPCODE_IMUL;
        } else if (cfStrStartsWith(token.ident, "mul")) {
            dataBuffer[0] = CF_OPCODE_MUL;
        } else if (cfStrStartsWith(token.ident, "idiv")) {
            dataBuffer[0] = CF_OPCODE_IDIV;
        } else if (cfStrStartsWith(token.ident, "div")) {
            dataBuffer[0] = CF_OPCODE_DIV;
        } else if (cfStrStartsWith(token.ident, "shr")) {
            dataBuffer[0] = CF_OPCODE_SHR;
        } else if (cfStrStartsWith(token.ident, "sar")) {
            dataBuffer[0] = CF_OPCODE_SAR;
        } else if (cfStrStartsWith(token.ident, "ftoi")) {
            dataBuffer[0] = CF_OPCODE_FTOI;
        } else if (cfStrStartsWith(token.ident, "fadd")) {
            dataBuffer[0] = CF_OPCODE_FADD;
        } else if (cfStrStartsWith(token.ident, "fsub")) {
            dataBuffer[0] = CF_OPCODE_FSUB;
        } else if (cfStrStartsWith(token.ident, "fmul")) {
            dataBuffer[0] = CF_OPCODE_FMUL;
        } else if (cfStrStartsWith(token.ident, "fdiv")) {
            dataBuffer[0] = CF_OPCODE_FDIV;
        } else if (cfStrStartsWith(token.ident, "itof")) {
            dataBuffer[0] = CF_OPCODE_ITOF;
        } else if (cfStrStartsWith(token.ident, "cmp")) {
            dataBuffer[0] = CF_OPCODE_CMP;
        } else if (cfStrStartsWith(token.ident, "icmp")) {
            dataBuffer[0] = CF_OPCODE_ICMP;
        } else if (cfStrStartsWith(token.ident, "fcmp")) {
            dataBuffer[0] = CF_OPCODE_FCMP;
        } else if (cfStrStartsWith(token.ident, "ret")) {
            dataBuffer[0] = CF_OPCODE_RET;
        } else if (cfStrStartsWith(token.ident, "unreachable")) {
            dataBuffer[0] = CF_OPCODE_UNREACHABLE;
        } else if (cfStrStartsWith(token.ident, "halt")) {
            dataBuffer[0] = CF_OPCODE_HALT;
        } else if (cfStrStartsWith(token.ident, "vsm")) {
            dataBuffer[0] = CF_OPCODE_VSM;
        } else if (cfStrStartsWith(token.ident, "vrs")) {
            dataBuffer[0] = CF_OPCODE_VRS;
        } else if (false
            || cfStrIsSame(token.ident, CF_STR("push"))
            || cfStrIsSame(token.ident, CF_STR("pop" ))
        ) {
            // yeah, functional programming
            dataBuffer[0] = token.ident.begin[1] == 'u'
                ? CF_OPCODE_PUSH
                : CF_OPCODE_POP
            ;

            const CfStr argumentStr = tokenSlice;

            uint32_t imm = 0;
            CfPushPopInfo info = {0};

            if (!cfAsmParsePushPopInfo(&tokenSlice, &info, &imm)) {
                if (details != NULL) {
                    details->invalidPushPopArgument.argument = argumentStr;
                    details->invalidPushPopArgument.line     = lineIndex;
                }
                resultStatus = CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT;
                goto cfAssemble__end;
            }

            // write info to data buffer
            *(CfPushPopInfo *)(dataBuffer + 1) = info;
            dataElementCount = 2;

            if (info.doReadImmediate) {
                *(uint32_t *)(dataBuffer + 2) = imm;
                dataElementCount = 6;
            }

        } else if (false
            || cfStrIsSame(token.ident, CF_STR("jmp" ))
            || cfStrIsSame(token.ident, CF_STR("jle" ))
            || cfStrIsSame(token.ident, CF_STR("jl"  ))
            || cfStrIsSame(token.ident, CF_STR("jge" ))
            || cfStrIsSame(token.ident, CF_STR("jg"  ))
            || cfStrIsSame(token.ident, CF_STR("jne" ))
            || cfStrIsSame(token.ident, CF_STR("je"  ))
            || cfStrIsSame(token.ident, CF_STR("call"))
        ) {
                 if (cfStrIsSame(token.ident, CF_STR("jmp" ))) dataBuffer[0] = CF_OPCODE_JMP;
            else if (cfStrIsSame(token.ident, CF_STR("jle" ))) dataBuffer[0] = CF_OPCODE_JLE;
            else if (cfStrIsSame(token.ident, CF_STR("jl"  ))) dataBuffer[0] = CF_OPCODE_JL;
            else if (cfStrIsSame(token.ident, CF_STR("jge" ))) dataBuffer[0] = CF_OPCODE_JGE;
            else if (cfStrIsSame(token.ident, CF_STR("jg"  ))) dataBuffer[0] = CF_OPCODE_JG;
            else if (cfStrIsSame(token.ident, CF_STR("jne" ))) dataBuffer[0] = CF_OPCODE_JNE;
            else if (cfStrIsSame(token.ident, CF_STR("je"  ))) dataBuffer[0] = CF_OPCODE_JE;
            else if (cfStrIsSame(token.ident, CF_STR("call"))) dataBuffer[0] = CF_OPCODE_CALL;

            // read label
            if (!cfAsmNextToken(&tokenSlice, &token) || (token.type != CF_ASM_TOKEN_TYPE_IDENT && token.type != CF_ASM_TOKEN_TYPE_INTEGER)) {
                if (details != NULL) {
                    details->unknownInstruction.instruction = line;
                    details->unknownInstruction.line = lineIndex;
                }
                resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
                goto cfAssemble__end;
            }

            if (token.type == CF_ASM_TOKEN_TYPE_INTEGER) {
                *(uint32_t *)(dataBuffer + 1) = token.integer;
            } else {
                CfLink fixup = {
                    .sourceLine = (uint32_t)lineIndex,
                    .codeOffset = (uint32_t)cfDarrSize(code) + 1,
                };

                if (token.ident.end - token.ident.begin > CF_LABEL_MAX) {
                    resultStatus = CF_ASSEMBLY_STATUS_TOO_LONG_LABEL;
                    if (details != NULL)
                        details->tooLongLabel = token.ident;
                    goto cfAssemble__end;
                }
                memcpy(fixup.label, token.ident.begin, token.ident.end - token.ident.begin);
                fixup.label[CF_LABEL_MAX - 1] = '\0';


                if (CF_DARR_OK != cfDarrPush(&links, &fixup)) {
                    resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
                    goto cfAssemble__end;
                }
                *(uint32_t *)(dataBuffer + 1) = ~0U; // for (kind of) safety
            }

            dataElementCount = 5;
        } else if (cfStrStartsWith(token.ident, "syscall")) {
            dataBuffer[0] = CF_OPCODE_SYSCALL;

            if (
                !cfAsmNextToken(&tokenSlice, &token) ||
                token.type != CF_ASM_TOKEN_TYPE_INTEGER
            ) {
                if (details != NULL) {
                    details->unknownInstruction.instruction = line;
                    details->unknownInstruction.line = lineIndex;

                }
                resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
                goto cfAssemble__end;
            }

            *(uint32_t *)(dataBuffer + 1) = token.integer;
            dataElementCount = 5;
        } else {
            // try to parse token
            CfStr labelName = token.ident;
            if (cfAsmNextToken(&tokenSlice, &token) && token.type == CF_ASM_TOKEN_TYPE_COLON) {
                // treat ident as label if parsed colon
                CfLabel newLabel = {
                    .sourceLine = (uint32_t)lineIndex,
                    .codeOffset = (uint32_t)cfDarrSize(code),
                };

                if (token.ident.end - token.ident.begin > CF_LABEL_MAX) {
                    resultStatus = CF_ASSEMBLY_STATUS_TOO_LONG_LABEL;
                    if (details != NULL)
                        details->tooLongLabel = token.ident;
                    goto cfAssemble__end;
                }
                memcpy(newLabel.label, token.ident.begin, token.ident.end - token.ident.begin);
                newLabel.label[CF_LABEL_MAX - 1] = '\0';

                if (CF_DARR_OK != cfDarrPush(&labels, &newLabel)) {
                    resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
                    goto cfAssemble__end;
                }
                dataElementCount = 0;
            } else {
                if (details != NULL) {
                    details->unknownInstruction.instruction = line;
                    details->unknownInstruction.line = lineIndex;
                }

                resultStatus = CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION;
                goto cfAssemble__end;
            }

        }

        // append element to code
        CfDarrStatus status = cfDarrPushArray(&code, &dataBuffer, dataElementCount);
        if (status != CF_DARR_OK) {
            resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
            goto cfAssemble__end;
        }
    }

    if (false
        || CF_DARR_OK != cfDarrIntoData(links, (void **)&resultLinks)
        || CF_DARR_OK != cfDarrIntoData(labels, (void **)&resultLabels)
        || CF_DARR_OK != cfDarrIntoData(code, (void **)&resultCode)
        || NULL == (resultSourceName = cfStrOwnedCopy(sourceName))
    ) {
        resultStatus = CF_ASSEMBLY_STATUS_INTERNAL_ERROR;
        goto cfAssemble__cleanResults;
    }

    dst->sourceName = resultSourceName;
    dst->code = resultCode;
    dst->codeLength = cfDarrSize(code);
    dst->labels = resultLabels;
    dst->labelCount = cfDarrSize(labels);
    dst->links = resultLinks;
    dst->linkCount = cfDarrSize(links);

cfAssemble__end:
    cfDarrDtor(code);
    cfDarrDtor(links);
    cfDarrDtor(labels);
    return resultStatus;

cfAssemble__cleanResults:
    free(resultCode);
    free(resultLabels);
    free(resultLinks);
    free(resultSourceName); // allowed by constructor specification

    goto cfAssemble__end;
} // cfAssemble

const char * cfAssemblyStatusStr( const CfAssemblyStatus status ) {
    switch (status) {
    case CF_ASSEMBLY_STATUS_OK                       : return "ok";
    case CF_ASSEMBLY_STATUS_INTERNAL_ERROR           : return "internal error";
    case CF_ASSEMBLY_STATUS_UNKNOWN_INSTRUCTION      : return "unknown instruction";
    case CF_ASSEMBLY_STATUS_UNEXPECTED_TEXT_END      : return "unexpected text end";
    case CF_ASSEMBLY_STATUS_UNKNOWN_REGISTER         : return "unknown register";
    case CF_ASSEMBLY_STATUS_INVALID_PUSHPOP_ARGUMENT : return "invalid push/pop argument";
    case CF_ASSEMBLY_STATUS_TOO_LONG_LABEL           : return "too long label";

    default                                          : return "<invalid>";
    }
} // cfAssemblyStatusStr

void cfAssemblyDetailsWrite(
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
        cfStrWrite(out, details->unknownInstruction.instruction);
        break;
    }

    default: {
        fprintf(out, "%s", str);
    }
    }
} // cfAssemblyDetailsWrite

// cf_assemble.c
