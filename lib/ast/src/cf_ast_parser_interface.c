/**
 * @brief parser interface implementation file
 */

#include "cf_ast_parser.h"

CfAstParseResult cfAstParse( CfStr fileName, CfStr fileContents, CfArena tempArena ) {
    // arena that contains actual AST data
    CfArena dataArena = NULL;
    CfAst ast = NULL;

    if (false
        || (dataArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED)) == NULL
        || (ast = (CfAst)cfArenaAlloc(dataArena, sizeof(CfAstImpl))) == NULL
    ) {
        cfArenaDtor(dataArena);
        return (CfAstParseResult) { .status = CF_AST_PARSE_STATUS_INTERNAL_ERROR };
    }

    bool tempArenaOwned = false;

    // allocate temporary arena if it's not already allocated
    if (tempArena == NULL) {
        tempArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED);
        if (tempArena == NULL) {
            cfArenaDtor(tempArena);
            return (CfAstParseResult) {
                .status = CF_AST_PARSE_STATUS_INTERNAL_ERROR,
            };
        }
        tempArenaOwned = true;
    }

    CfAstParser parser = {
        .tempArena = tempArena,
        .dataArena = dataArena,
    };

    int isError = setjmp(parser.errorBuffer);

    if (isError) {
        // perform cleanup
        if (tempArenaOwned)
            cfArenaDtor(tempArena);
        cfArenaDtor(dataArena);
        return parser.parseResult;
    }

    CfAstDecl *declArray = NULL;
    size_t declArrayLen = 0;

    cfAstParseDecls(&parser, fileContents, &declArray, &declArrayLen);

    // destroy temp arena in case if it's created in this function
    if (tempArenaOwned)
        cfArenaDtor(tempArena);

    // assemble AST from parts.
    *ast = (CfAstImpl) {
        .mem            = dataArena,
        .sourceName     = fileName,
        .sourceContents = fileContents,
        .declArray      = declArray,
        .declArrayLen   = declArrayLen,
    };

    return (CfAstParseResult) {
        .status = CF_AST_PARSE_STATUS_OK,
        .ok = ast,
    };
} // cfAstParse

// cf_ast_parser_interface.c