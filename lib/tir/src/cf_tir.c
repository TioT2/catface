/**
 * @brief TIR (Typed Intermediate Representation) main implementation file
 */

#include <assert.h>

#include "cf_tir.h"

/// @brief TIR, actually
struct CfTir_ {
    CfArena dataArena; ///< TIR actual content storage arena
}; // struct CfTir

CfTirBuildingResult cfTirBuild( const CfAst *ast, CfArena tempArena ) {
    assert(ast != NULL);

    bool tempArenaOwned = false;
    if (tempArena == NULL) {
        tempArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED);
        if (tempArena == NULL)
            return (CfTirBuildingResult) { CF_TIR_BUILDING_STATUS_INTERNAL_ERROR };
        tempArenaOwned = true;
    }

    if (tempArenaOwned)
        cfArenaDtor(tempArena);

    return (CfTirBuildingResult) { CF_TIR_BUILDING_STATUS_INTERNAL_ERROR };
} // cfTirFromAst

// cf_tir.c
