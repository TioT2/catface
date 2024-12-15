/**
 * @brief TIR (Typed Intermediate Representation) main implementation file
 */

#include "cf_tir_builder.h"

CfTirType cfTirTypeFromAstType( CfAstType type ) {
    switch (type) {
    case CF_AST_TYPE_I32  : return CF_TIR_TYPE_I32  ;
    case CF_AST_TYPE_U32  : return CF_TIR_TYPE_U32  ;
    case CF_AST_TYPE_F32  : return CF_TIR_TYPE_F32  ;
    case CF_AST_TYPE_VOID : return CF_TIR_TYPE_VOID ;
    }
} // cfTirTypeFromAstType

CfAstType cfAstTypeFromTirType( CfTirType type ) {
    switch (type) {
    case CF_TIR_TYPE_I32  : return CF_AST_TYPE_I32  ;
    case CF_TIR_TYPE_U32  : return CF_AST_TYPE_U32  ;
    case CF_TIR_TYPE_F32  : return CF_AST_TYPE_F32  ;
    case CF_TIR_TYPE_VOID : return CF_AST_TYPE_VOID ;
    }
} // cfTirTypeFromAstType

CfTirBinaryOperator cfTirBinaryOperatorFromAstBinaryOperator( CfAstBinaryOperator op ) {
    switch (op) {
    case CF_AST_BINARY_OPERATOR_ADD : return CF_TIR_BINARY_OPERATOR_ADD ;
    case CF_AST_BINARY_OPERATOR_SUB : return CF_TIR_BINARY_OPERATOR_SUB ;
    case CF_AST_BINARY_OPERATOR_MUL : return CF_TIR_BINARY_OPERATOR_MUL ;
    case CF_AST_BINARY_OPERATOR_DIV : return CF_TIR_BINARY_OPERATOR_DIV ;
    case CF_AST_BINARY_OPERATOR_EQ  : return CF_TIR_BINARY_OPERATOR_EQ  ;
    case CF_AST_BINARY_OPERATOR_NE  : return CF_TIR_BINARY_OPERATOR_NE  ;
    case CF_AST_BINARY_OPERATOR_LT  : return CF_TIR_BINARY_OPERATOR_LT  ;
    case CF_AST_BINARY_OPERATOR_GT  : return CF_TIR_BINARY_OPERATOR_GT  ;
    case CF_AST_BINARY_OPERATOR_LE  : return CF_TIR_BINARY_OPERATOR_LE  ;
    case CF_AST_BINARY_OPERATOR_GE  : return CF_TIR_BINARY_OPERATOR_GE  ;
    }
} // cfTirBinaryOperatorFromAstBinaryOperator

bool cfTirBinaryOperatorIsComparison( CfTirBinaryOperator op ) {
    switch (op) {
    case CF_TIR_BINARY_OPERATOR_ADD : return false ;
    case CF_TIR_BINARY_OPERATOR_SUB : return false ;
    case CF_TIR_BINARY_OPERATOR_MUL : return false ;
    case CF_TIR_BINARY_OPERATOR_DIV : return false ;
    case CF_TIR_BINARY_OPERATOR_LT  : return true  ;
    case CF_TIR_BINARY_OPERATOR_GT  : return true  ;
    case CF_TIR_BINARY_OPERATOR_LE  : return true  ;
    case CF_TIR_BINARY_OPERATOR_GE  : return true  ;
    case CF_TIR_BINARY_OPERATOR_EQ  : return true  ;
    case CF_TIR_BINARY_OPERATOR_NE  : return true  ;
    }
} // cfTirBinaryOperatorIsComparison


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
 * @return function id
 */
CfTirFunctionId cfTirBuilderExploreFunction(
    CfTirBuilder *const self,
    const CfAstFunction *function
) {
    // check for function being already added
    CfDequeCursor fnCursor = {};
    if (cfDequeFrontCursor(self->functions, &fnCursor)) {
        // check for name duplicates

        do {
            CfTirBuilderFunction *tirFunction = (CfTirBuilderFunction *)cfDequeCursorGet(&fnCursor);

            // check if function matches prototype of previous declaration

            if (true
                && cfStrIsSame(function->name, tirFunction->function.name)
                && !cfTirBuilderAstFunctionMatchesPrototype(
                    self,
                    function,
                    &tirFunction->function.prototype
                )
            ) {
                cfTirBuilderFinish(self, (CfTirBuildingResult) {
                    .status = CF_TIR_BUILDING_STATUS_UNMATCHED_FUNCTION_PROTOTYPES,
                    .unmatchedFunctionPrototypes = {
                        .firstDeclaration = tirFunction->astFunction,
                        .secondDeclaration = function,
                    }
                });
            }

            // return it's id
            if (cfStrIsSame(tirFunction->astFunction->name, function->name))
                return tirFunction->id;

        } while (cfDequeCursorAdvance(&fnCursor, 1));

        cfDequeCursorDtor(&fnCursor);
    }

    // acquire function id
    CfTirFunctionId id = cfDequeLength(self->functions);

    // build prototype
    CfTirFunctionPrototype prototype = {
        .inputTypeArray = (CfTirType *)cfTirBuilderAllocData(
            self,
            sizeof(CfTirType) * function->inputCount
        ),
        .inputTypeArrayLength = function->inputCount,
        .outputType = cfTirTypeFromAstType(function->outputType),
    };

    for (size_t i = 0; i < prototype.inputTypeArrayLength; i++)
        prototype.inputTypeArray[i] = cfTirTypeFromAstType(function->inputs[i].type);

    // build function
    CfTirBuilderFunction fn = {
        .function     = (CfTirFunction) {
            .prototype = prototype,
            .name = function->name,
            .impl = NULL,
        },
        .id           = id,
        .astFunction  = function,
    };

    // add to function deque
    cfTirBuilderAssert(self, cfDequePushBack(self->functions, &fn));

    return id;
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

    // compile function implementations
    cfTirBuildFunctions(self);

    // allocate tir itself
    CfTir *tir = (CfTir *)cfTirBuilderAllocData(self, sizeof(CfTir));

    // build function array
    CfTirFunction *functionArray = (CfTirFunction *)cfTirBuilderAllocData(
        self,
        sizeof(CfTirFunction) * cfDequeLength(self->functions)
    );

    CfDequeCursor functionCursor = {};
    if (cfDequeFrontCursor(self->functions, &functionCursor)) {
        size_t index = 0;
        do {
            functionArray[index] = ((CfTirBuilderFunction *)cfDequeCursorGet(&functionCursor))->function;
        } while (cfDequeCursorAdvance(&functionCursor, 1));
        cfDequeCursorDtor(&functionCursor);
    }

    // initialize build TIR structure
    *tir = (CfTir) {
        .dataArena           = self->dataArena,
        .functionArray       = functionArray,
        .functionArrayLength = cfDequeLength(self->functions),
        .sourceName          = cfAstGetSourceFileName(ast),
    };

    return tir;
} // cfTirBuildFromAst


CfTirBuildingResult cfTirBuild( const CfAst *ast, CfArena *tempArena ) {
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

void cfTirDtor( CfTir *tir ) {
    if (tir != NULL)
        cfArenaDtor(tir->dataArena);
} // cfTirDtor

const CfTirFunction * cfTirGetFunctionArray( const CfTir *tir ) {
    assert(tir != NULL);
    return tir->functionArray;
} // cfTirGetFunctionArray

size_t cfTirGetFunctionArrayLength( const CfTir *tir ) {
    assert(tir != NULL);
    return tir->functionArrayLength;
} // cfTirGetFunctionArrayLength

const CfTirFunction * cfTirGetFunctionById( const CfTir *tir, CfTirFunctionId functionId ) {
    assert(tir != NULL);

    if (tir->functionArrayLength >= functionId)
        return NULL;
    return &tir->functionArray[(size_t)functionId];
} // cfTirGetFunctionById

CfStr cfTirGetSourceName( const CfTir *tir ) {
    assert(tir != NULL);
    return tir->sourceName;
} // cfTirGetSourceName

// cf_tir.c
