/**
 * @brief AST main declaration file
 */

#ifndef CF_AST_H_
#define CF_AST_H_

#include <cf_string.h>
#include <cf_arena.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief declcaration forward-declaration
typedef struct CfAstDecl_ CfAstDecl;

/// @brief statement forward-declaration
typedef struct CfAstStmt_ CfAstStmt;

/// @brief expression forward-declaration
typedef struct CfAstExpr_ CfAstExpr;

/// @brief block forward-declaration
typedef struct CfAstBlock_ CfAstBlock;

/// @brief source text region representaiton structure
typedef struct CfAstSpan_ {
    size_t begin; ///< offset from file start (in characters) to span start
    size_t end;   ///< offset from file start (in characters) to span end (exclusive)
} CfAstSpan;

/**
 * @brief substr from string by span getting function
 * 
 * @param[in] span span to cut str by
 * @param[in] str  str to cut
 * 
 * @return strnig cut by span
 */
CfStr cfAstSpanCutStr( CfAstSpan span, CfStr str );

/**
 * @brief span in json format dumping function
 * 
 * @param[in] out  destination file
 * @param[in] span span to dump
 */
void cfAstSpanDumpJson( FILE *out, CfAstSpan span );

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
typedef enum CfAstDeclType_ {
    CF_AST_DECL_TYPE_FN,  ///< function declaration
    CF_AST_DECL_TYPE_LET, ///< variable declaration
} CfAstDeclType;

/**
 * @brief declaration type name getting function
 * 
 * @param[in] declType declaration type
 * 
 * @return declaration type name
 */
const char * cfAstDeclTypeStr( CfAstDeclType declType );

/// @brief function parameter representation structure
typedef struct CfAstFunctionParam_ {
    CfStr     name; ///< parameter name
    CfAstType type; ///< parameter type
    CfAstSpan span; ///< span function param located in
} CfAstFunctionParam;

/// @brief function declaration structure
typedef struct CfAstFunction_ {
    CfStr                name;          ///< name
    CfAstFunctionParam * params;        ///< parameter array (owned)
    size_t               paramCount;    ///< parameter array size
    CfAstType            returnType;    ///< returned function type
    CfAstSpan            signatureSpan; ///< signature span
    CfAstSpan            span;          ///< span function located in
    CfAstBlock         * impl;          ///< implementation
} CfAstFunction;

/// @brief variable declaration structure
typedef struct CfAstVariable_ {
    CfStr       name; ///< name
    CfAstType   type; ///< type
    CfAstExpr * init; ///< initializer expression (may be null)
    CfAstSpan   span; ///< span variable declaration located in
} CfAstVariable;

/// @brief declaration structure
struct CfAstDecl_ {
    CfAstDeclType type; ///< declaration union tag
    CfAstSpan     span; ///< span declaration located in

    union {
        CfAstFunction fn;  ///< function
        CfAstVariable let; ///< global variable
    };
}; // struct CfAstDecl

/// @brief statement union tag
typedef enum CfAstStmtType_ {
    CF_AST_STMT_TYPE_EXPR,  ///< expression
    CF_AST_STMT_TYPE_DECL,  ///< statement that declares something
    CF_AST_STMT_TYPE_BLOCK, ///< block statement (curly brace enclosed sequence)
    CF_AST_STMT_TYPE_IF,    ///< if/else statement
    CF_AST_STMT_TYPE_WHILE, ///< while loop statement

} CfAstStmtType;

/// @brief statement repersentation structure
struct CfAstStmt_ {
    CfAstStmtType type; ///< statement type
    CfAstSpan     span; ///< span statement located in

    union {
        CfAstExpr  * expr;  ///< expression statament (non-null)
        CfAstDecl    decl;  ///< declarative expression
        CfAstBlock * block; ///< block expression (non-null)

        struct {
            CfAstExpr  * condition; ///< condition (non-null)
            CfAstBlock * codeThen;  ///< code to execute then condition returned true (non-null)
            CfAstBlock * codeElse;  ///< code to execute then condition returned false (nulllable)
        } if_; ///< if statement

        struct {
            CfAstExpr  * conditinon; ///< loop condition
            CfAstBlock * code;       ///< loop code
        } while_; ///< while statement
    };
}; // struct CfAstStmt_

/// @brief block (curly brace enclosed statement sequence) representation structure
struct CfAstBlock_ {
    CfAstSpan   span;      ///< span block located in
    size_t      stmtCount; ///< statement array size
    CfAstStmt   stmts[1];  ///< statement array (extends beyond struct memory for stmtCount - 1 elements)
}; // struct CfAstBlock_

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

/// @brief expression type (expression union tag)
typedef enum CfAstExprType_ {
    CF_AST_EXPR_TYPE_INTEGER,         ///< integer constant
    CF_AST_EXPR_TYPE_FLOATING,        ///< float-point constant
    CF_AST_EXPR_TYPE_IDENTIFIER,      ///< identifier
    CF_AST_EXPR_TYPE_CALL,            ///< function call
    CF_AST_EXPR_TYPE_ASSIGNMENT,      ///< assignment expression
    CF_AST_EXPR_TYPE_BINARY_OPERATOR, ///< binary operator expression
} CfAstExprType;

/// @brief binary operator used for assignment
typedef enum CfAstAssignmentOperator_ {
    CF_AST_ASSIGNMENT_OPERATOR_NONE, ///< do not perform additional actions during assignment
    CF_AST_ASSIGNMENT_OPERATOR_ADD,  ///< perform addition
    CF_AST_ASSIGNMENT_OPERATOR_SUB,  ///< perform substraction
    CF_AST_ASSIGNMENT_OPERATOR_MUL,  ///< perform multiplication
    CF_AST_ASSIGNMENT_OPERATOR_DIV,  ///< perform division
} CfAstAssignmentOperator;

// /// @brief call expression
// typedef struct CfAstExprCall_ {
//     CfAstExpr *  callee;              ///< called object
//     size_t       argumentArrayLength; ///< argArrayLength length
//     CfAstExpr ** argumentArray;       ///< arguments function called with
// } CfAstExprCall;

/// @brief expression representaiton structure
struct CfAstExpr_ {
    CfAstExprType type; ///< expression type
    CfAstSpan     span; ///< span expression piece located in

    union {
        uint64_t integer;    ///< integer expression
        double   floating;   ///< floating-point expression
        CfStr    identifier; ///< identifier

        struct {
            CfAstExpr  * callee;           ///< called expression
            size_t       paramArrayLength; ///< parameters function called by
            CfAstExpr ** paramArray;       ///< parameter array
        } call; ///< function call

        struct {
            CfAstAssignmentOperator   op;          ///< exact assignment operator
            CfStr                     destination; ///< variable to assign to name (variable assignment only is supported at least now)
            CfAstExpr               * value;       ///< value to assign to
        } assignment; ///< assignment

        struct {
            CfAstBinaryOperator   op;  ///< operator to perform
            CfAstExpr           * lhs; ///< left hand side
            CfAstExpr           * rhs; ///< right hand side
        } binaryOperator; ///< binary operator
    };
}; // struct CfAstExpr_

/// @brief AST handle representation structure
typedef struct CfAstImpl_ * CfAst;

/**
 * @brief AST destructor
 * 
 * @param[in] ast AST to destroy (nullable)
 */
void cfAstDtor( CfAst ast );

/**
 * @brief AST in JSON format writing function
 * 
 * @param[in] out output file to write ast to
 * @param[in] ast ast to write
 */
void cfAstDumpJson( FILE *out, const CfAst ast );

/**
 * @brief AST declarations getting function
 * 
 * @param[in] ast AST to get declarations of (non-null)
 * 
 * @return AST declaration array pointer
 */
const CfAstDecl * cfAstGetDecls( const CfAst ast );

/**
 * @brief AST declaration count getting function
 * 
 * @param[in] ast AST to get declarations off (non-null)
 * 
 * @return AST declaration count
 */
size_t cfAstGetDeclCount( const CfAst ast );

/**
 * @brief AST source file name getting function
 * 
 * @param[in] ast AST to get source file name of (non-null)
 * 
 * @return AST source file name slice
 */
CfStr cfAstGetSourceFileName( const CfAst ast );

/// @brief token type (union tag)
typedef enum CFAstTokenType_ {
    CF_AST_TOKEN_TYPE_INTEGER,         ///< integer constant
    CF_AST_TOKEN_TYPE_FLOATING,        ///< floating-point constant
    CF_AST_TOKEN_TYPE_IDENTIFIER,      ///< identifier

    CF_AST_TOKEN_TYPE_FN,              ///< "fn"    keyword
    CF_AST_TOKEN_TYPE_LET,             ///< "let"   keyword
    CF_AST_TOKEN_TYPE_I32,             ///< "i32"   keyword
    CF_AST_TOKEN_TYPE_U32,             ///< "u32"   keyword
    CF_AST_TOKEN_TYPE_F32,             ///< "f32"   keyword
    CF_AST_TOKEN_TYPE_VOID,            ///< "void"  keyword
    CF_AST_TOKEN_TYPE_IF,              ///< "if"    keyword
    CF_AST_TOKEN_TYPE_ELSE,            ///< "else"  keyword
    CF_AST_TOKEN_TYPE_WHILE,           ///< "while" keyword

    CF_AST_TOKEN_TYPE_ANGULAR_BR_OPEN_EQUAL,  ///< "<=" character combination
    CF_AST_TOKEN_TYPE_ANGULAR_BR_CLOSE_EQUAL, ///< ">=" character combination
    CF_AST_TOKEN_TYPE_EQUAL_EQUAL,            ///< "==" character combination
    CF_AST_TOKEN_TYPE_EXCLAMATION_EQUAL,      ///< "!=" character combination
    CF_AST_TOKEN_TYPE_PLUS_EQUAL,             ///< "+=" character combination
    CF_AST_TOKEN_TYPE_MINUS_EQUAL,            ///< "-=" character combination
    CF_AST_TOKEN_TYPE_ASTERISK_EQUAL,         ///< "*=" character combination
    CF_AST_TOKEN_TYPE_SLASH_EQUAL,            ///< "/=" character combination

    CF_AST_TOKEN_TYPE_ANGULAR_BR_OPEN,        ///< '<' symbol
    CF_AST_TOKEN_TYPE_ANGULAR_BR_CLOSE,       ///< '>' symbol

    CF_AST_TOKEN_TYPE_COLON,           ///< ':' symbol
    CF_AST_TOKEN_TYPE_SEMICOLON,       ///< ';' symbol
    CF_AST_TOKEN_TYPE_COMMA,           ///< ',' symbol
    CF_AST_TOKEN_TYPE_EQUAL,           ///< '=' symbol
    CF_AST_TOKEN_TYPE_PLUS,            ///< '+' symbol
    CF_AST_TOKEN_TYPE_MINUS,           ///< '-' symbol
    CF_AST_TOKEN_TYPE_ASTERISK,        ///< '*' symbol
    CF_AST_TOKEN_TYPE_SLASH,           ///< '/' symbol
    CF_AST_TOKEN_TYPE_CURLY_BR_OPEN,   ///< '{' symbol
    CF_AST_TOKEN_TYPE_CURLY_BR_CLOSE,  ///< '}' symbol
    CF_AST_TOKEN_TYPE_ROUND_BR_OPEN,   ///< '(' symbol
    CF_AST_TOKEN_TYPE_ROUND_BR_CLOSE,  ///< ')' symbol
    CF_AST_TOKEN_TYPE_SQUARE_BR_OPEN,  ///< '[' symbol
    CF_AST_TOKEN_TYPE_SQUARE_BR_CLOSE, ///< ']' symbol

    CF_AST_TOKEN_TYPE_COMMENT,         ///< comment
    CF_AST_TOKEN_TYPE_END,             ///< text ending token
} CfAstTokenType;

/// @brief token representation structure (tagged union, actually)
typedef struct CfAstToken_ {
    CfAstTokenType type; ///< token kind
    CfAstSpan      span; ///< span this token occupies

    union {
        CfStr    identifier; ///< identifier
        uint64_t integer;    ///< integer constant
        double   floating;   ///< floating-point constant
        CfStr    comment;    ///< comment token
    };
} CfAstToken;

/// @brief AST parsing status
typedef enum CfAstParseStatus_ {
    CF_AST_PARSE_STATUS_OK,                             ///< parsing succeeded
    CF_AST_PARSE_STATUS_INTERNAL_ERROR,                 ///< internal error occured
    CF_AST_PARSE_STATUS_UNEXPECTED_SYMBOL,              ///< unexpected symbol occured (tokenization error)
    CF_AST_PARSE_STATUS_UNEXPECTED_TOKEN_TYPE,          ///< function signature parsing error occured
    CF_AST_PARSE_STATUS_EXPR_BRACKET_INTERNALS_MISSING, ///< no contents in sub-expression
    CF_AST_PARSE_STATUS_EXPR_RHS_MISSING,               ///< right hand side missing
    CF_AST_PARSE_STATUS_EXPR_ASSIGNMENT_VALUE_MISSING,  ///< missing value to assign to expression

    CF_AST_PARSE_STATUS_IF_CONDITION_MISSING,           ///< 'if' statement condition missing
    CF_AST_PARSE_STATUS_IF_BLOCK_MISSING,               ///< 'if' statement code block is missing
    CF_AST_PARSE_STATUS_ELSE_BLOCK_MISSING,             ///< 'else' code block is missing

    CF_AST_PARSE_STATUS_WHILE_CONDITION_MISSING,        ///< 'while' statement condition missing
    CF_AST_PARSE_STATUS_WHILE_BLOCK_MISSING,            ///< 'while' statement code block is missing

    CF_AST_PARSE_STATUS_VARIABLE_TYPE_MISSING,          ///< no variable type
    CF_AST_PARSE_STATUS_VARIABLE_INIT_MISSING,          ///< variable initailizer missing
} CfAstParseStatus;

/// @brief AST parsing result (tagged union)
typedef struct CfAstParseResult_ {
    CfAstParseStatus status; ///< operation status

    union {
        CfAst ok; ///< success case

        struct {
            char   symbol; ///< unexpected symbol itself
            size_t offset; ///< offset to the symbol in source text
        } unexpectedSymbol;

        struct {
            CfAstToken     actualToken;  ///< actual token
            CfAstTokenType expectedType; ///< expected token type
        } unexpectedTokenType;

        // Do something with this situation...

        CfAstSpan variableTypeMissing;     ///< variable type missing
        CfAstSpan variableInitMissing;     ///< variable initializer missing
        CfAstSpan bracketInternalsMissing; ///< bracket internals missing
        CfAstSpan rhsMissing;              ///< right hand side missing
        CfAstSpan ifConditionMissing;      ///< 'if' condition missing
        CfAstSpan ifBlockMissing;          ///< 'if' code block missing
        CfAstSpan elseBlockMissing;        ///< 'else' code block missing
        CfAstSpan assignmentValueMissing;  ///< assignment value missing
        CfAstSpan whileConditionMissing;   ///< 'if' condition missing
        CfAstSpan whileBlockMissing;       ///< 'if' code block missing
    };
} CfAstParseResult;

/**
 * @brief AST from file contents parsing function
 * 
 * @param[in] fileName     input file name
 * @param[in] fileContents text to parse, actually
 * @param[in] tempArena    arena to allocate temporary memory in (nullable)
 * 
 * @return operation result
 * 
 * @note
 * - 'fileName' and 'fileContents' parameter contents **must** live longer, than resulting AST in case if this function execution succeeded.
 */
CfAstParseResult cfAstParse( CfStr fileName, CfStr fileContents, CfArena tempArena );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_H_)

// cf_ast.h
