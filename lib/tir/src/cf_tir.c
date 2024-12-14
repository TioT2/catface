/**
 * @brief TIR (Typed Intermediate Representation) main implementation file
 */

#include <assert.h>
#include <setjmp.h>

#include <cf_deque.h>

#include "cf_tir.h"

/// @brief TIR, actually
struct CfTir_ {
    CfArena dataArena; ///< TIR actual content storage arena
}; // struct CfTir

/// @brief build-time internal function structure
typedef struct CfTirBuilderFunction_ {
    CfStr                 name;         ///< function name
    CfDeque             * argNameDeque; ///< function parameter names
    CfTirFunction         function;     ///< tir-level function
    CfTirFunctionId       id;           ///< identifier
    const CfAstFunction * astFunction;  ///< AST function
} CfTirBuilderFunction;

/// @brief build-time internal prototype structure
typedef struct CfTirBuilderFunctionPrototype_ {
    CfTirFunctionPrototype   prototype; ///< tir-level prototype
    CfTirFunctionPrototypeId id;        ///< id
} CfTirBuilderFunctionPrototype;

/// @brief TIR builder
typedef struct CfTirBuilder_ {
    CfArena dataArena;
    CfArena tempArena;

    CfDeque *functionPrototypes;     ///< set of function prototypes
    CfDeque *functions;              ///< set of defined functions

    jmp_buf             errorBuffer; ///< error buffer
    CfTirBuildingResult error;       ///< error itself
} CfTirBuilder;

/**
 * @brief immediately finish building process
 * 
 * @param[in] self   building pointer
 * @param[in] result building result (must be error)
 * 
 * @note this function would never return
 */
void cfTirBuilderFinish( CfTirBuilder *const self, CfTirBuildingResult result ) {
    assert(self != NULL);

    self->error = result;
    longjmp(self->errorBuffer, 1);
} // cfTirBuilderFinish

/**
 * @brief assertion (finishes with INTERNAL_ERROR if condition is false)
 * 
 * @param[in] self builder pointer
 * @param[in] cond condition
 */
void cfTirBuilderAssert( CfTirBuilder *const self, bool cond ) {
    if (!cond)
        cfTirBuilderFinish(self, (CfTirBuildingResult) { CF_TIR_BUILDING_STATUS_INTERNAL_ERROR });
} // cfTirBuilderAssert

/**
 * @brief get function prototype by id from builder
 * 
 * @param[in] self builder pointer
 * @param[in] id   prototype id (MUST BE valid)
 * 
 * @return function prototype pointer
 */
const CfTirFunctionPrototype * cfTirBuilderGetFunctionPrototype(
    CfTirBuilder             *const self,
    CfTirFunctionPrototypeId        id
) {
    CfDequeCursor prototypeCursor;

    cfTirBuilderAssert(self, cfDequeFrontCursor(self->functionPrototypes, &prototypeCursor));
    cfTirBuilderAssert(self, cfDequeCursorAdvance(&prototypeCursor, id));

    const CfTirBuilderFunctionPrototype *prototype = (const CfTirBuilderFunctionPrototype *)cfDequeCursorGet(&prototypeCursor);

    cfTirBuilderAssert(self, prototype->id == id);

    cfDequeCursorDtor(&prototypeCursor);

    return &prototype->prototype;
} // cfTirBuilderGetFunctionPrototype

/**
 * @brief TIR type from ast one building function
 * 
 * @param[in] type ast type
 * 
 * @return corresponding TIR type
 */
CfTirType cfTirTypeFromAstType( CfAstType type ) {
    switch (type) {
    case CF_AST_TYPE_I32  : return CF_TIR_TYPE_I32  ;
    case CF_AST_TYPE_U32  : return CF_TIR_TYPE_U32  ;
    case CF_AST_TYPE_F32  : return CF_TIR_TYPE_F32  ;
    case CF_AST_TYPE_VOID : return CF_TIR_TYPE_VOID ;
    }
} // cfTirTypeFromAstType

/**
 * @brief check for function matches prototype
 * 
 * @param[in] self      builder pointer
 * @param[in] function  AST function to perform check for
 * @param[in] prototype prototype to check
 * 
 * @return true if matches, false if not
 */
bool cfTirBuilderAstFunctionMatchesPrototype(
    CfTirBuilder                 * const self,
    const CfAstFunction          * function,
    const CfTirFunctionPrototype * prototype
) {
    if (prototype->outputType != cfTirTypeFromAstType(function->outputType))
        return false;

    if (function->inputCount != prototype->inputTypeArrayLength)
        return false;

    for (size_t i = 0; i < prototype->inputTypeArrayLength; i++)
        if (prototype->inputTypeArray[i] != cfTirTypeFromAstType(function->inputs[i].type))
            return false;
    return true;
} // cfTirBuilderAstFunctionMatchesPrototype

/**
 * @brief function prototype building function
 * 
 * @param[in] self     builder pointer
 * @param[in] function function to build prototype for
 * 
 * @return prototype id
 * 
 * @note prototype is allocated on temp arena
 */
CfTirFunctionId cfTirBuilderExploreFunction(
    CfTirBuilder *const self,
    const CfAstFunction *function
) {
    // check for function being already added
    CfDequeCursor fnCursor = {};
    if (cfDequeFrontCursor(self->functions, &fnCursor)) {
        // check for name duplicates

        while (cfDequeCursorAdvance(&fnCursor, 1)) {
            CfTirBuilderFunction *fn = (CfTirBuilderFunction *)cfDequeCursorGet(&fnCursor);

            // check if function matches prototype of previous declaration
            if (!cfTirBuilderAstFunctionMatchesPrototype(
                self,
                function,
                cfTirBuilderGetFunctionPrototype(self, fn->function.prototypeId)
            )) {
                cfTirBuilderFinish(self, (CfTirBuildingResult) {
                    .status = CF_TIR_BUILDING_STATUS_UNMATCHED_FUNCTION_PROTOTYPES,
                    .unmatchedFunctionPrototypes = {
                        .firstDeclaration = fn->astFunction,
                        .secondDeclaration = function,
                    }
                });
            }

            // return it's id
            if (cfStrIsSame(fn->name, function->name))
                return fn->id;
        }

        cfDequeCursorDtor(&fnCursor);
    }

    // find function prototype
    CfDequeCursor prototypeCursor = {};
    if (cfDequeFrontCursor(self->functionPrototypes, &prototypeCursor)) {
        // check for some prototype match this function

        while (cfDequeCursorAdvance(&prototypeCursor, 1)) {
            const CfTirBuilderFunctionPrototype *prototype =
                (const CfTirBuilderFunctionPrototype *)cfDequeCursorGet(
                &prototypeCursor
            );

            if (cfTirBuilderAstFunctionMatchesPrototype(self, function, &prototype->prototype)) {
                // prototype found
            }
        }

        cfDequeCursorDtor(&prototypeCursor);
    }

    // add function to function pool
} // cfTirBuildPrototype

/**
 * @brief build tir from ast
 * 
 * @param[in] builder pointer
 * @param[in] ast     ast to build from (non-null)
 * 
 * @return created TIR (non-null)
 * 
 * @note TIR building entry function.
 */
CfTir * cfTirBuildFromAst( CfTirBuilder *const self, const CfAst *ast ) {
    const CfAstDeclaration *declarations = cfAstGetDeclarations(ast);
    size_t declarationCount = cfAstGetDeclarationCount(ast);

    // 'explore' functions
    for (size_t i = 0; i < declarationCount; i++) {
        const CfAstDeclaration *decl = &declarations[i];

        switch (decl->type) {
        case CF_AST_DECLARATION_TYPE_FN:
            break;

        case CF_AST_DECLARATION_TYPE_LET:
            cfTirBuilderFinish(self, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_GLOBAL_VARIABLES_NOT_ALLOWED,
                .globalVariablesNotAllowed = &decl->let,
            });
            break;
        }

        cfTirBuilderExploreFunction(self, &decl->fn);
    }

    // then compile'em
    for (size_t i = 0; i < declarationCount; i++) {

    }

    return NULL;
} // cfTirBuildFromAst

CfTirBuildingResult cfTirBuild( const CfAst *ast, CfArena tempArena ) {
    assert(ast != NULL);
    CfTir *tir = NULL;

    bool tempArenaOwned = false;
    if (tempArena == NULL) {
        tempArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED);
        if (tempArena == NULL)
            return (CfTirBuildingResult) { CF_TIR_BUILDING_STATUS_INTERNAL_ERROR };
        tempArenaOwned = true;
    }

    CfTirBuilder builder = {NULL};

    builder.tempArena = tempArena;
    builder.error = (CfTirBuildingResult) { CF_TIR_BUILDING_STATUS_INTERNAL_ERROR };

    // setup data arena and arrays
    if (false
        || (builder.dataArena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED)) == NULL
        || (builder.functionPrototypes = cfDequeCtor(
            sizeof(CfTirBuilderFunctionPrototype),
            CF_DEQUE_CHUNK_SIZE_UNDEFINED,
            builder.tempArena
        )) == NULL
        || (builder.functions = cfDequeCtor(
            sizeof(CfTirBuilderFunction),
            CF_DEQUE_CHUNK_SIZE_UNDEFINED,
            builder.tempArena
        )) == NULL
    )
        goto cfTirBuild__fail;

    if (setjmp(builder.errorBuffer) != 0)
        goto cfTirBuild__fail;

    // run TIR building
    tir = cfTirBuildFromAst(&builder, ast);

    // destroy temp arena if it's required
    if (tempArenaOwned)
        cfArenaDtor(tempArena);

    // finish
    return (CfTirBuildingResult) {
        .status = CF_TIR_BUILDING_STATUS_OK,
        .ok = tir,
    };

    cfTirBuild__fail:

    // common cleanup
    if (tempArenaOwned)
        cfArenaDtor(tempArena);
    cfArenaDtor(builder.dataArena);

    return builder.error;
} // cfTirBuild

// cf_tir.c
