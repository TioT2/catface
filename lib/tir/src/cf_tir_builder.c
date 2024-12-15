/**
 * @brief main TIR builder implementation file
 */

#include "cf_tir_builder.h"

void cfTirBuilderFinish( CfTirBuilder *const self, CfTirBuildingResult result ) {
    assert(self != NULL);

    self->error = result;
    longjmp(self->errorBuffer, 1);
} // cfTirBuilderFinish

void cfTirBuilderAssert( CfTirBuilder *const self, bool cond ) {
    if (!cond)
        cfTirBuilderFinish(self, (CfTirBuildingResult) { CF_TIR_BUILDING_STATUS_INTERNAL_ERROR });
} // cfTirBuilderAssert

void * cfTirBuilderAllocTemp( CfTirBuilder *const self, size_t size ) {
    void *mem = cfArenaAlloc(self->tempArena, size);
    cfTirBuilderAssert(self, mem != NULL);
    return mem;
} // cfTirBuilderAllocTemp

void * cfTirBuilderAllocData( CfTirBuilder *const self, size_t size ) {
    void *mem = cfArenaAlloc(self->dataArena, size);
    cfTirBuilderAssert(self, mem != NULL);
    return mem;
} // cfTirBuilderAllocData

CfTirBuilderFunction * cfTirBuilderFindFunction( CfTirBuilder *const self, CfStr functionName ) {
    CfDequeCursor cursor = {};

    if (cfDequeFrontCursor(self->functions, &cursor)) {
        do {
            CfTirBuilderFunction *function = (CfTirBuilderFunction *)cfDequeCursorGet(&cursor);

            if (cfStrIsSame(functionName, function->function.name))
                return function;
        } while (cfDequeCursorAdvance(&cursor, 1));
        cfDequeCursorDtor(&cursor);
    }

    return NULL;
} // cfTirBuilderFindFunction

CfTirBuilderFunction * cfTirBuilderGetFunction( CfTirBuilder *const self, CfTirFunctionId functionId ) {
    CfDequeCursor cursor = {};
    CfTirBuilderFunction *fn = NULL;

    if (cfDequeFrontCursor(self->functions, &cursor)) {
        if (cfDequeCursorAdvance(&cursor, functionId))
            fn = (CfTirBuilderFunction *)cfDequeCursorGet(&cursor);
        cfDequeCursorDtor(&cursor);
    }

    return fn;
} // cfTirBuilderGetFunction

// cf_tir_builder.c
