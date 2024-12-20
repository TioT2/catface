/**
 * @brief AST main declaration file
 */

#ifndef CF_AST_H_
#define CF_AST_H_

#include <cf_string.h>
#include <cf_arena.h>
#include <cf_lexer.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief declcaration forward-declaration
typedef struct CfAstDeclaration_ CfAstDeclaration;

/// @brief statement forward-declaration
typedef struct CfAstStatement_ CfAstStatement;

/// @brief expression forward-declaration
typedef struct CfAstExpression_ CfAstExpression;

/// @brief block forward-declaration
typedef struct CfAstBlock_ CfAstBlock;

/**
 * @brief span in json format dumping function
 * 
 * @param[in] out  destination file
 * @param[in] span span to dump
 */
void cfAstSpanDumpJson( FILE *out, CfStrSpan span );

/// @brief primitive (e.g. builtin) type representation enumeration
/// @note this kind of type declaration is TMP solution
typedef enum CfAstType_ {
    CF_AST_TYPE_I32,  ///< 32-bit integer primitive type
    CF_AST_TYPE_U32,  ///< 32-bit unsigned primitive type
    CF_AST_TYPE_F32,  ///< 32-bit floating point primitive type
    CF_AST_TYPE_VOID, ///< zero-sized primitive type
} CfAstType;

/**
 * @brief AST type name getting function
 * 
 * @param[in] type type to get name of
 * 
 * @return type name
 */
const char * cfAstTypeStr( CfAstType type );

/// @brief declaration union tag
typedef enum CfAstDeclarationType_ {
    CF_AST_DECLARATION_TYPE_FN,  ///< function declaration
    CF_AST_DECLARATION_TYPE_LET, ///< variable declaration
} CfAstDeclarationType;

/**
 * @brief declaration type name getting function
 * 
 * @param[in] declType declaration type
 * 
 * @return declaration type name
 */
const char * cfAstDeclarationTypeStr( CfAstDeclarationType declType );

/// @brief function parameter representation structure
typedef struct CfAstFunctionParam_ {
    CfStr     name; ///< parameter name
    CfAstType type; ///< parameter type
    CfStrSpan span; ///< span function param located in
} CfAstFunctionParam;

/// @brief function declaration structure
typedef struct CfAstFunction_ {
    CfStr                name;          ///< name
    CfAstFunctionParam * inputs;        ///< parameter array (owned)
    size_t               inputCount;    ///< parameter array size
    CfAstType            outputType;    ///< returned function type
    CfStrSpan            signatureSpan; ///< signature span
    CfStrSpan            span;          ///< span function located in
    CfAstBlock         * impl;          ///< implementation
} CfAstFunction;

/// @brief variable declaration structure
typedef struct CfAstVariable_ {
    CfStr       name; ///< name
    CfAstType   type; ///< type
    CfAstExpression * init; ///< initializer expression (may be null)
    CfStrSpan   span; ///< span variable declaration located in
} CfAstVariable;

/// @brief declaration structure
struct CfAstDeclaration_ {
    CfAstDeclarationType type; ///< declaration union tag
    CfStrSpan     span; ///< span declaration located in

    union {
        CfAstFunction fn;  ///< function
        CfAstVariable let; ///< global variable
    };
}; // struct CfAstDeclaration

/// @brief statement union tag
typedef enum CfAstStatementType {
    CF_AST_STATEMENT_TYPE_EXPRESSION,  ///< expression
    CF_AST_STATEMENT_TYPE_DECLARATION, ///< statement that declares something
    CF_AST_STATEMENT_TYPE_BLOCK,       ///< block statement (curly brace enclosed sequence)
    CF_AST_STATEMENT_TYPE_IF,          ///< if/else statement
    CF_AST_STATEMENT_TYPE_WHILE,       ///< while loop statement
    CF_AST_STATEMENT_TYPE_RETURN,      ///< return from function
} CfAstStatementType;

/// @brief statement repersentation structure
struct CfAstStatement_ {
    CfAstStatementType type; ///< statement type
    CfStrSpan          span; ///< span statement located in

    union {
        CfAstExpression  * expression;  ///< expression statament (non-null)
        CfAstDeclaration   declaration; ///< declarative expression
        CfAstBlock       * block;       ///< block expression (non-null)

        struct {
            CfAstExpression  * condition; ///< condition (non-null)
            CfAstBlock       * blockThen;  ///< code to execute then condition returned true (non-null)
            CfAstBlock       * blockElse;  ///< code to execute then condition returned false (nulllable)
        } if_; ///< if statement

        struct {
            CfAstExpression  * condition; ///< loop condition
            CfAstBlock       * code;       ///< loop code
        } while_; ///< while statement

        CfAstExpression *return_; ///< returned value (nullable, actually)
    };
}; // struct CfAstStatement_

/// @brief block (curly brace enclosed statement sequence) representation structure
struct CfAstBlock_ {
    CfStrSpan        span;           ///< span block located in
    size_t           statementCount; ///< statement array size
    CfAstStatement   statements[1];  ///< statement array (extends beyond struct memory for statementCount - 1 elements)
}; // struct CfAstBlock_

/// @brief expression type (expression union tag)
typedef enum CfAstExpressionType_ {
    CF_AST_EXPRESSION_TYPE_INTEGER,         ///< integer constant
    CF_AST_EXPRESSION_TYPE_FLOATING,        ///< float-point constant
    CF_AST_EXPRESSION_TYPE_IDENTIFIER,      ///< identifier
    CF_AST_EXPRESSION_TYPE_CALL,            ///< function call
    CF_AST_EXPRESSION_TYPE_CONVERSION,      ///< type conversion
    CF_AST_EXPRESSION_TYPE_ASSIGNMENT,      ///< assignment expression
    CF_AST_EXPRESSION_TYPE_BINARY_OPERATOR, ///< binary operator expression
} CfAstExpressionType;

/// @brief type conversion expression
typedef struct CfAstExpressionConversion_ {
    CfAstExpression * expr; ///< expression convert
    CfAstType         type; ///< type to convert expression to
} CfAstExpressionConversion;

/// @brief call expression
typedef struct CfAstExpressionCall_ {
    CfAstExpression *  callee;              ///< called object
    size_t             argumentArrayLength; ///< argArrayLength length
    CfAstExpression ** argumentArray;       ///< arguments function called with
} CfAstExpressionCall;

/// @brief binary operator used for assignment
typedef enum CfAstAssignmentOperator_ {
    CF_AST_ASSIGNMENT_OPERATOR_NONE, ///< do not perform additional actions during assignment
    CF_AST_ASSIGNMENT_OPERATOR_ADD,  ///< perform addition
    CF_AST_ASSIGNMENT_OPERATOR_SUB,  ///< perform substraction
    CF_AST_ASSIGNMENT_OPERATOR_MUL,  ///< perform multiplication
    CF_AST_ASSIGNMENT_OPERATOR_DIV,  ///< perform division
} CfAstAssignmentOperator;

/// @brief assignment expression
typedef struct CfAstExprAssignment_ {
    CfAstAssignmentOperator   op;          ///< exact assignment operator
    CfStr                     destination; ///< variable to assign to name (variable assignment only is supported at least now)
    CfAstExpression         * value;       ///< value to assign to
} CfAstExprAssignment;

/// @brief binary operator enumeration
typedef enum CfAstBinaryOperator_ {
    CF_AST_BINARY_OPERATOR_ADD, ///< addition
    CF_AST_BINARY_OPERATOR_SUB, ///< substraction
    CF_AST_BINARY_OPERATOR_MUL, ///< multiplication
    CF_AST_BINARY_OPERATOR_DIV, ///< division
    CF_AST_BINARY_OPERATOR_EQ,  ///< equal
    CF_AST_BINARY_OPERATOR_NE,  ///< not equal
    CF_AST_BINARY_OPERATOR_LT,  ///< less than
    CF_AST_BINARY_OPERATOR_GT,  ///< greater than
    CF_AST_BINARY_OPERATOR_LE,  ///< less equal
    CF_AST_BINARY_OPERATOR_GE,  ///< greater equal
} CfAstBinaryOperator;

/// @brief binary operator expression
typedef struct CfAstExprBinaryOperator_ {
    CfAstBinaryOperator   op;  ///< operator to perform
    CfAstExpression     * lhs; ///< left hand side
    CfAstExpression     * rhs; ///< right hand side
} CfAstExprBinaryOperator;

/// @brief expression representaiton structure
struct CfAstExpression_ {
    CfAstExpressionType type; ///< expression type
    CfStrSpan           span; ///< span expression piece located in

    union {
        uint64_t                  integer;        ///< integer expression
        double                    floating;       ///< floating-point expression
        CfStr                     identifier;     ///< identifier
        CfAstExpressionCall       call;           ///< function call
        CfAstExpressionConversion conversion;     ///< type conversion
        CfAstExprAssignment       assignment;     ///< assignment
        CfAstExprBinaryOperator   binaryOperator; ///< binary operator
    };
}; // struct CfAstExpression_

/// @brief AST handle representation structure
typedef struct CfAst_ CfAst;

/**
 * @brief AST destructor
 * 
 * @param[in] ast AST to destroy (nullable)
 */
void cfAstDtor( CfAst *ast );

/**
 * @brief AST in JSON format writing function
 * 
 * @param[in] out output file to write ast to
 * @param[in] ast ast to write
 */
void cfAstDumpJson( FILE *out, const CfAst *ast );

/**
 * @brief AST declarations getting function
 * 
 * @param[in] ast AST to get declarations of (non-null)
 * 
 * @return AST declaration array pointer
 */
const CfAstDeclaration * cfAstGetDeclarations( const CfAst *ast );

/**
 * @brief AST declaration count getting function
 * 
 * @param[in] ast AST to get declarations off (non-null)
 * 
 * @return AST declaration count
 */
size_t cfAstGetDeclarationCount( const CfAst *ast );

/// @brief AST parsing status
typedef enum CfAstParseStatus_ {
    // general
    CF_AST_PARSE_STATUS_OK,                             ///< parsing succeeded
    CF_AST_PARSE_STATUS_INTERNAL_ERROR,                 ///< internal error occured
    CF_AST_PARSE_STATUS_UNEXPECTED_TOKEN_TYPE,          ///< function signature parsing error occured

    // expression parsing-related
    CF_AST_PARSE_STATUS_EXPR_BRACKET_INTERNALS_MISSING, ///< no contents in sub-expression
    CF_AST_PARSE_STATUS_EXPR_RHS_MISSING,               ///< right hand side missing
    CF_AST_PARSE_STATUS_EXPR_ASSIGNMENT_VALUE_MISSING,  ///< missing value to assign to expression

    // if-else-related
    CF_AST_PARSE_STATUS_IF_CONDITION_MISSING,           ///< 'if' statement condition missing
    CF_AST_PARSE_STATUS_IF_BLOCK_MISSING,               ///< 'if' statement code block is missing
    CF_AST_PARSE_STATUS_ELSE_BLOCK_MISSING,             ///< 'else' code block is missing

    // while-related
    CF_AST_PARSE_STATUS_WHILE_CONDITION_MISSING,        ///< 'while' statement condition missing
    CF_AST_PARSE_STATUS_WHILE_BLOCK_MISSING,            ///< 'while' statement code block is missing

    // variable declaration-related
    CF_AST_PARSE_STATUS_VARIABLE_TYPE_MISSING,          ///< no variable type
    CF_AST_PARSE_STATUS_VARIABLE_INIT_MISSING,          ///< variable initailizer missing
} CfAstParseStatus;

/// @brief AST parsing result (tagged union)
typedef struct CfAstParseResult_ {
    CfAstParseStatus status; ///< operation status

    union {
        CfAst *ok; ///< success case

        struct {
            const CfLexerToken * actualToken;  ///< actual token
            CfLexerTokenType     expectedType; ///< expected token type
        } unexpectedTokenType;

        // Do something with this situation...

        CfStrSpan variableTypeMissing;     ///< variable type missing
        CfStrSpan variableInitMissing;     ///< variable initializer missing
        CfStrSpan bracketInternalsMissing; ///< bracket internals missing
        CfStrSpan rhsMissing;              ///< right hand side missing
        CfStrSpan ifConditionMissing;      ///< 'if' condition missing
        CfStrSpan ifBlockMissing;          ///< 'if' code block missing
        CfStrSpan elseBlockMissing;        ///< 'else' code block missing
        CfStrSpan assignmentValueMissing;  ///< assignment value missing
        CfStrSpan whileConditionMissing;   ///< 'if' condition missing
        CfStrSpan whileBlockMissing;       ///< 'if' code block missing
    };
} CfAstParseResult;

/**
 * @brief AST from file contents parsing function
 * 
 * @param[in] tokenList list of tokens to build AST from. (non-null)
 * @param[in] tempArena arena to allocate temporary memory in (nullable)
 * 
 * @return operation result
 * 
 * @note
 * - tokenList **must** live longer, than AST.
 * 
 * - tokenList **must** have CF_LEXER_TOKEN_TYPE_END in the parsed region end
 */
CfAstParseResult cfAstParse( const CfLexerToken *tokenList, CfArena *tempArena );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_H_)

// cf_ast.h
