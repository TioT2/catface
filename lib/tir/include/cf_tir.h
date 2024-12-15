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
    CfTirFunctionId,          ///< function id
    CfTirLocalVariableId,     ///< local variable id
    CfTirGlobalVariableId;    ///< global variable id

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
    CF_TIR_STATEMENT_BLOCK,           ///< block
    CF_TIR_STATEMENT_TYPE_IF,         ///< if-else combination
    CF_TIR_STATEMENT_TYPE_LOOP,       ///< while loop
} CfTirStatementType;

/// @brief statement
typedef struct CfTirStatement_ {
    CfTirStatementType type; ///< statement type

    union {
        CfTirExpression * expression; ///< expression
        CfTirBlock      * block;      ///< block statement

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
    CfTirFunctionPrototype     prototype; ///< prototype
    CfStr                      name;      ///< name
    CfTirBlock               * impl;      ///< implementation (may be NULL)
} CfTirFunction;

/// @brief expression type
typedef enum CfTirExpressionType_ {
    CF_TIR_EXPRESSION_TYPE_CONST_I32,       ///< 32-bit signed integer constant
    CF_TIR_EXPRESSION_TYPE_CONST_F32,       ///< 32-bit unsigned integer constant
    CF_TIR_EXPRESSION_TYPE_CONST_U32,       ///< 32-bit float-point constant
    CF_TIR_EXPRESSION_TYPE_VOID,            ///< void value
    CF_TIR_EXPRESSION_TYPE_BINARY_OPERATOR, ///< binary operator
    CF_TIR_EXPRESSION_TYPE_CALL,            ///< non-functional object cannot be called yet
    CF_TIR_EXPRESSION_TYPE_LOCAL,           ///< variable
    CF_TIR_EXPRESSION_TYPE_GLOBAL,          ///< variable
    CF_TIR_EXPRESSION_TYPE_ASSIGNMENT,      ///< variable assignment
    CF_TIR_EXPRESSION_TYPE_CAST,            ///< type cast
} CfTirExpressionType;

/// @brief binary operator
typedef enum CfTirBinaryOperator_ {
    CF_TIR_BINARY_OPERATOR_ADD, ///< addition
    CF_TIR_BINARY_OPERATOR_SUB, ///< substraction
    CF_TIR_BINARY_OPERATOR_MUL, ///< multiplication
    CF_TIR_BINARY_OPERATOR_DIV, ///< division
    CF_TIR_BINARY_OPERATOR_LT,  ///< less
    CF_TIR_BINARY_OPERATOR_GT,  ///< greater
    CF_TIR_BINARY_OPERATOR_LE,  ///< less-equal
    CF_TIR_BINARY_OPERATOR_GE,  ///< greater-equal
    CF_TIR_BINARY_OPERATOR_EQ,  ///< equal
    CF_TIR_BINARY_OPERATOR_NE,  ///< not equal
} CfTirBinaryOperator;

/**
 * @brief returns true if operator is comparison operator
 * 
 * @param[in] op binary operator
 * 
 * @return true if operator is LT/GT/LE/GE/EQ/NE
 */
bool cfTirBinaryOperatorIsComparison( CfTirBinaryOperator op );

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
            CfTirFunctionId    functionId;       ///< function
            CfTirExpression ** inputArray;       ///< function input array
            size_t             inputArrayLength; ///< function input array length
        } call;

        struct {
            CfTirLocalVariableId   destination; ///< assignment destination
            CfTirExpression      * value;       ///< value to assign
        } assignment;

        struct {
            CfTirExpression * expression; ///< expression TO cast
            CfTirType         type;       ///< type to cast to
        } cast;
    };
}; // struct CfTirExpression_

/// @brief TIR structure
typedef struct CfTir_ CfTir;

/**
 * @brief TIR destructor
 * 
 * @param[in] tir TIR to destroy (nullable)
 */
void cfTirDtor( CfTir *tir );

/**
 * @brief TIR function array getter
 * 
 * @param[in] tir TIR to get functions of (non-null)
 * 
 * @return function array first element pointer
 */
const CfTirFunction * cfTirGetFunctionArray( const CfTir *tir );

/**
 * @brief TIR function array length getter
 * 
 * @param[in] tir TIR to get lenght of function array of (non-null)
 * 
 * @return function array size
 */
size_t cfTirGetFunctionArrayLength( const CfTir *tir );

/// @brief TIR from AST generation status
typedef enum CfTirBuildingStatus_ {
    CF_TIR_BUILDING_STATUS_OK,                            ///< ok

    CF_TIR_BUILDING_STATUS_INTERNAL_ERROR,                   ///< internal tir builder error occured
    CF_TIR_BUILDING_STATUS_GLOBAL_VARIABLES_NOT_ALLOWED,     ///< global variables aren't allowed (yet)
    CF_TIR_BUILDING_STATUS_LOCAL_FUNCTIONS_NOT_ALLOWED,      ///< local funciton declarations aren't allowed (yet)
    CF_TIR_BUILDING_STATUS_UNMATCHED_FUNCTION_PROTOTYPES,    ///< two distinct function declarations don't match
    CF_TIR_BUILDING_STATUS_CANNOT_DEDUCE_LITERAL_TYPE,       ///< cannot deduce literal type
    CF_TIR_BUILDING_STATUS_UNKNOWN_VARIABLE_REFERENCED,      ///< required variable does not exist
    CF_TIR_BUILDING_STATUS_FUNCTION_DOES_NOT_EXIST,          ///< unknown function called
    CF_TIR_BUILDING_STATUS_IMPOSSIBLE_CAST,                  ///< impossible to cast one type to another
    CF_TIR_BUILDING_STATUS_EXPRESSION_IS_NOT_CALLABLE,       ///< given expression result cannot be called as function
    CF_TIR_BUILDING_STATUS_UNEXPECTED_ARGUMENT_NUMBER,       ///< unexpected number of function arguments
    CF_TIR_BUILDING_STATUS_UNEXPECTED_ARGUMENT_TYPE,         ///< unexpected parameter type
    CF_TIR_BUILDING_STATUS_UNEXPECTED_ASSIGNMENT_VALUE_TYPE, ///< invalid assignment right hand side type
    CF_TIR_BUILDING_STATUS_OPERAND_TYPES_UNMATCHED,          ///< function operand types unmatched
    CF_TIR_BUILDING_STATUS_OPERATOR_IS_NOT_DEFINED,          ///< operator is not defined for certain operand type
    CF_TIR_BUILDING_STATUS_IF_CONDITION_TYPE_MUST_BE_U32,    ///< 'if' statement condition **must** have U32 type
    CF_TIR_BUILDING_STATUS_WHILE_CONDITION_TYPE_MUST_BE_U32, ///< 'while' statement condition **must** have U32 type
    CF_TIR_BUILDING_STATUS_UNEXPECTED_INITIALIZER_TYPE,      ///< unexpected variable initializer resulting type
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

        const CfAstExpression * cannotDeduceLiteralType;   ///< span literal occured in

        const CfAstExpression * unknownVariableReferenced; ///< variable expression

        const CfAstExpression * functionDoesNotExist;      ///< function expression

        struct {
            const CfAstExpression * expression; ///< cast itself
            CfAstType               srcType;    ///< source expression type
            CfAstType               dstType;    ///< destination expression type
        } impossibleCast;

        struct {
            const CfAstExpression * expression; ///< expression to call
        } expressionIsNotCallable;

        struct {
            const CfAstFunction   * calledFunction; ///< called function
            const CfAstExpression * call;           ///< call expression itself
        } unexpectedArgumentNumber;

        struct {
            const CfAstFunction   * calledFunction;      ///< called function
            const CfAstExpression * call;                ///< call expression
            const CfAstExpression * parameterExpression; ///< parameter expression
            size_t                  parameterIndex;      ///< parameter index
            CfAstType               requiredType;        ///< required type
            CfAstType               actualType;          ///< actual type
        } unexpectedArgumentType;

        struct {
            const CfAstExpression * assignmentExpression; ///< assignment expression itself
            CfAstType               requiredType;         ///< required value type
            CfAstType               actualType;           ///< actual value type
        } unexpectedAssignmentValueType;

        struct {
            const CfAstExpression * expression; ///< expression
            CfAstType               lhsType;    ///< LHS type
            CfAstType               rhsType;    ///< RHS type
        } operandTypesUnmatched;

        struct {
            const CfAstExpression * expression; ///< expression
            CfAstType               type;       ///< lhs and rhs type
        } operatorIsNotDefined;

        struct {
            const CfAstExpression * condition;  ///< conditional expression
            CfAstType               actualType; ///< actual expression type
        } ifConditionMustBeU32;

        struct {
            const CfAstExpression * condition;  ///< conditional expression
            CfAstType               actualType; ///< actual expression type
        } whileConditionMustBeU32;

        struct {
            const CfAstVariable * variableDeclaration; ///< declaration error happened in
            CfAstType             expectedType;        ///< expected type
            CfAstType             actualType;          ///< actual type
        } unexpectedInitializerType;
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
