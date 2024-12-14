/**
 * @brief TIR main declaration file
 * 
 * TIR - Typed Intermediate Representation.
 * Whereas valid AST defines program, that is correct
 * syntactically only, valid TIR defines program that
 * is correct by CF language standard (haha).
 */

#ifndef CF_TIR_H_
#define CF_TIR_H_

#include <cf_ast.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief block forward declaration
typedef struct CfTirBlock_ CfTirBlock;

/// @brief expression type forward declaration
typedef struct CfTirExpression_ CfTirExpression;

/// @brief common Id type
typedef uint32_t CfTirId;

/// @brief 'NULL' id
#define CF_TIR_BAD_ID (~(CfTirId)0)

typedef CfTirId
    CfTirFunctionPrototypeId, ///< function prototype id
    CfTirFunctionId,          ///< function id
    CfTirLocalVariableId,     ///< local variable id
    CfTirGlobalVariableId;    ///< global variable id

/// @brief function index
typedef CfTirId CfTirFunctionId;

/// @brief local variable id
typedef CfTirId CfTirLocalVariableId;

/// @brief global variable id
typedef CfTirId CfTirGlobalVariableId;

/// @brief primitive
typedef enum CfTirType_ {
    CF_TIR_TYPE_I32,  ///< 32-bit signed integer primitive type
    CF_TIR_TYPE_U32,  ///< 32-bit unsigned integer primitive type
    CF_TIR_TYPE_F32,  ///< 32-bit floating-point primitive type
    CF_TIR_TYPE_VOID, ///< zero-sized primitive type
} CfTirType;

/// @brief variable structure
typedef struct CfTirLocalVariable_ {
    CfTirType type; ///< variable type
    CfStr     name; ///< TIR variable name
} CfTirLocalVariable;

/// @brief statement type
typedef enum CfTirStatementType_ {
    CF_TIR_STATEMENT_TYPE_EXPRESSION, ///< expression
    CF_TIR_STATEMENT_TYPE_IF,         ///< if-else combination
    CF_TIR_STATEMENT_TYPE_LOOP,       ///< while loop
} CfTirStatementType;

/// @brief statement
typedef struct CfTirStatement_ {
    CfTirStatementType type; ///< statement type

    union {
        CfTirExpression *expression; ///< expression

        struct {
            CfTirExpression * condition; ///< 'if' condition
            CfTirBlock      * blockThen; ///< block to execute then succeeded
            CfTirBlock      * blockElse; ///< block to execute then not succeeded
        } if_;

        struct {
            CfTirExpression * condition; ///< loop condition (may be NULL)
            CfTirBlock      * block;     ///< loop code block
        } loop;
    };
} CfTirStatement;

/// @brief statement list
struct CfTirBlock_ {
    CfTirLocalVariable  * locals;         ///< variables declared in block
    size_t                localCount;     ///< variable count
    CfTirStatement      * statements;     ///< statement array
    size_t                statementCount; ///< statement count
}; // struct CftirBlock_

/// @brief function prototype building function
typedef struct CfTirFunctionPrototype_ {
    CfTirType * inputTypeArray;       ///< input type array
    size_t      inputTypeArrayLength; ///< input array length
    CfTirType   outputType;           ///< input type
} CfTirFunctionPrototype;

/// @brief function structure
typedef struct CfTirFunction_ {
    CfTirFunctionPrototypeId   prototypeId; ///< prototype identifier (must NOT be CF_TIR_BAD_ID)
    CfTirBlock               * impl;        ///< function implementation (may be NULL)
} CfTirFunction;

/// @brief expression type
typedef enum CfTirExpressionType_ {
    CF_TIR_EXPRESSION_TYPE_CONST_I32,       ///< 32-bit signed integer constant
    CF_TIR_EXPRESSION_TYPE_CONST_F32,       ///< 32-bit unsigned integer constant
    CF_TIR_EXPRESSION_TYPE_CONST_U32,       ///< 32-bit float-point constant
    CF_TIR_EXPRESSION_TYPE_BINARY_OPERATOR, ///< binary operator
    CF_TIR_EXPRESSION_TYPE_CALL,            ///< non-functional object cannot be called yet
    CF_TIR_EXPRESSION_TYPE_LOCAL,           ///< variable
    CF_TIR_EXPRESSION_TYPE_GLOBAL,          ///< variable
} CfTirExpressionType;

/// @brief binary operator
typedef enum CfTirBinaryOperator_ {
    CF_TIR_BINARY_OPERATOR_ADD, ///< addition
    CF_TIR_BINARY_OPERATOR_SUB, ///< substraction
    CF_TIR_BINARY_OPERATOR_MUL, ///< multiplication
    CF_TIR_BINARY_OPERATOR_DIV, ///< division
    CF_TIR_BINARY_OPERATOR_LE,  ///< less-equal
    CF_TIR_BINARY_OPERATOR_LT,  ///< less
    CF_TIR_BINARY_OPERATOR_GE,  ///< greater-equal
    CF_TIR_BINARY_OPERATOR_EQ,  ///< equal
    CF_TIR_BINARY_OPERATOR_NE,  ///< not equal
} CfTirBinaryOperator;

/// @brief expression structure
struct CfTirExpression_ {
    CfTirExpressionType type;
    CfTirType           resultingType; ///< expression resulting type (non-void)

    union {
        uint32_t              constU32; ///< unsigned integer 32-bit constant
        int32_t               constI32; ///< signed integer 32-bit constant
        float                 constF32; ///< floating-point 32-bit constant
        CfTirLocalVariableId  local;    ///< local variable id
        CfTirGlobalVariableId global;   ///< global variable id

        struct {
            CfTirBinaryOperator   op;  ///< operator itself
            CfTirExpression     * lhs; ///< left hand side
            CfTirExpression     * rhs; ///< right hand side
        } binaryOperator;

        struct {
            CfTirFunctionId   functionId;       ///< function
            CfTirExpression * inputArray;       ///< function input array
            size_t            inputArrayLength; ///< function input array length
        } call;
    };
}; // struct CfTirExpression_

/// @brief TIR structure
typedef struct CfTir_ CfTir;

/// @brief TIR from AST generation status
typedef enum CfTirBuildingStatus_ {
    CF_TIR_BUILDING_STATUS_OK,                            ///< ok
    CF_TIR_BUILDING_STATUS_INTERNAL_ERROR,                ///< internal tir builder error occured
    CF_TIR_BUILDING_STATUS_GLOBAL_VARIABLES_NOT_ALLOWED,  ///< global variables aren't allowed (yet)
    CF_TIR_BUILDING_STATUS_LOCAL_FUNCTIONS_NOT_ALLOWED,   ///< local funciton declarations aren't allowed (yet)
    CF_TIR_BUILDING_STATUS_UNMATCHED_FUNCTION_PROTOTYPES, ///< two distinct function declarations don't match
} CfTirBuildingStatus;

/// @brief TIR from AST generation result
typedef struct CfTirBuildingResult_ {
    CfTirBuildingStatus status; ///< building status

    union {
        CfTir *ok; ///< generation succeeded
        const CfAstVariable *globalVariablesNotAllowed; ///< global variables aren't allowed.

        struct {
            const CfAstFunction *function;         ///< function this expression
            const CfAstFunction *externalFunction; ///< function this function is declared in
        } localFunctionsNotAllowed;

        struct {
            const CfAstFunction *firstDeclaration;  ///< first function declaration
            const CfAstFunction *secondDeclaration; ///< second function declaration
        } unmatchedFunctionPrototypes;
    };
} CfTirBuildingResult;

/**
 * @brief build TIR from AST
 * 
 * @param[in] ast       AST to build TIR from (non-nulll)
 * @param[in] tempArena arena to allocate temporary objects in (nullable)
 * 
 * @return TIR building result
 */
CfTirBuildingResult cfTirBuild( const CfAst *ast, CfArena tempArena );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_TIR_H_)

// cf_tir.h
