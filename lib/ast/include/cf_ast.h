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
typedef struct __CfAstDecl CfAstDecl;

/// @brief statement forward-declaration
typedef struct __CfAstStmt CfAstStmt;

/// @brief expression forward-declaration
typedef struct __CfAstExpr CfAstExpr;

/// @brief block forward-declaration
typedef struct __CfAstBlock CfAstBlock;

/// @brief source text region representaiton structure
typedef struct __CfAstSpan {
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

/// @brief primitive (e.g. builtin) type representation enumeration
/// @note this kind of type declaration is TMP solution
typedef enum __CfAstType {
    CF_AST_TYPE_I32,  ///< 32-bit integer primitive type
    CF_AST_TYPE_U32,  ///< 32-bit unsigned primitive type
    CF_AST_TYPE_F32,  ///< 32-bit floating point primitive type
    CF_AST_TYPE_VOID, ///< zero-sized primitive type
} CfAstType;

/// @brief declaration union tag
typedef enum __CfAstDeclType {
    CF_AST_DECL_TYPE_FN,  ///< function declaration
    CF_AST_DECL_TYPE_LET, ///< variable declaration
} CfAstDeclType;

/// @brief function parameter representation structure
typedef struct __CfAstFunctionParam {
    CfStr     name; ///< parameter name
    CfAstType type; ///< parameter type
    CfAstSpan span; ///< span function param located in
} CfAstFunctionParam;

/// @brief function declaration structure
typedef struct __CfAstFunction {
    CfStr                name;          ///< name
    CfAstFunctionParam * params;        ///< parameter array (owned)
    size_t               paramCount;    ///< parameter array size
    CfAstType            returnType;    ///< returned function type
    CfAstSpan            signatureSpan; ///< signature span
    CfAstSpan            span;          ///< span function located in
    CfAstBlock         * impl;          ///< implementation
} CfAstFunction;

/// @brief variable declaration structure
typedef struct __CfAstVariable {
    CfStr       name; ///< name
    CfAstType   type; ///< type
    CfAstExpr * init; ///< initializer expression (may be null)
    CfAstSpan   span; ///< span variable declaration located in
} CfAstVariable;

/// @brief declaration structure
struct __CfAstDecl {
    CfAstDeclType type; ///< declaration union tag
    CfAstSpan     span; ///< span declaration located in

    union {
        CfAstFunction fn;  ///< function
        CfAstVariable let; ///< global variable
    };
}; // struct CfAstDecl

/// @brief statement union tag
typedef enum __CfAstStmtType {
    CF_AST_STMT_TYPE_EXPR,  ///< expression
    CF_AST_STMT_TYPE_DECL,  ///< statement that declares something
    CF_AST_STMT_TYPE_BLOCK, ///< block statement (curly brace enclosed sequence)
} CfAstStmtType;

/// @brief statement repersentation structure
struct __CfAstStmt {
    CfAstStmtType type; ///< statement type
    CfAstSpan     span; ///< span statement located in

    union {
        CfAstExpr  * expr;  ///< expression statament
        CfAstDecl  * decl;  ///< declarative expression
        CfAstBlock * block; ///< block expression
    };
}; // struct __CfAstStmt

/// @brief block (curly brace enclosed statement sequence) representation structure
struct __CfAstBlock {
    CfAstSpan   span;      ///< span block located in
    size_t      stmtCount; ///< statement array size
    CfAstStmt   stmts[1];  ///< statement array (extends beyond struct memory for stmtCount - 1 elements)
}; // struct __CfAstBlock

/// @brief expression type (expression union tag)
typedef enum __CfAstExprType {
    CF_AST_EXPR_TYPE_INTEGER,  ///< integer constant
    CF_AST_EXPR_TYPE_FLOATING, ///< float-point constant
} CfAstExprType;

/// @brief expression representaiton structure
struct __CfAstExpr {
    CfAstExprType type; ///< expression type
    CfAstSpan     span; ///< span expression piece located in

    union {
        uint64_t integer;  ///< integer expression
        double   floating; ///< floating-point expression
    };
}; // struct __CfAstExpr

/// @brief AST handle representation structure
typedef struct __CfAstImpl * CfAst;

/**
 * @brief AST destructor
 * 
 * @param[in] ast AST to destroy (nullable)
 */
void cfAstDtor( CfAst ast );

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
typedef enum __CFAstTokenType {
    CF_AST_TOKEN_TYPE_INTEGER,         ///< integer constant
    CF_AST_TOKEN_TYPE_FLOATING,        ///< floating-point constant
    CF_AST_TOKEN_TYPE_IDENT,           ///< ident

    CF_AST_TOKEN_TYPE_FN,              ///< "fn"   keyword
    CF_AST_TOKEN_TYPE_LET,             ///< "let"  keyword
    CF_AST_TOKEN_TYPE_I32,             ///< "i32"  keyword
    CF_AST_TOKEN_TYPE_U32,             ///< "u32"  keyword
    CF_AST_TOKEN_TYPE_F32,             ///< "f32"  keyword
    CF_AST_TOKEN_TYPE_VOID,            ///< "void" keyword

    CF_AST_TOKEN_TYPE_COLON,           ///< ':' symbol
    CF_AST_TOKEN_TYPE_SEMICOLON,       ///< ';' symbol
    CF_AST_TOKEN_TYPE_COMMA,           ///< ',' symbol
    CF_AST_TOKEN_TYPE_EQUAL,           ///< '=' symbol
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
typedef struct __CfAstToken {
    CfAstTokenType type; ///< token kind
    CfAstSpan      span; ///< span this token occupies

    union {
        CfStr    ident;    ///< ident
        uint64_t integer;  ///< integer constant
        double   floating; ///< floating-point constant
        CfStr    comment;  ///< comment token
    };
} CfAstToken;

/// @brief AST parsing status
typedef enum __CfAstParseStatus {
    CF_AST_PARSE_STATUS_OK,                    ///< parsing succeeded
    CF_AST_PARSE_STATUS_INTERNAL_ERROR,        ///< internal error occured
    CF_AST_PARSE_STATUS_UNEXPECTED_SYMBOL,     ///< unexpected symbol occured (tokenization error)
    CF_AST_PARSE_STATUS_UNEXPECTED_TOKEN_TYPE, ///< function signature parsing error occured
    CF_AST_PARSE_STATUS_NOT_DECLARATION_START, ///< current token can't belong to ANY declaration start
    CF_AST_PARSE_STATUS_VARIABLE_TYPE_MISSING, ///< no variable type

    CF_AST_PARSE_STATUS_EXPR_VALUE_REQUIRED,   ///< expression 'value' (ident/number) required
} CfAstParseStatus;

/// @brief AST parsing result (tagged union)
typedef struct __CfAstParseResult {
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

        CfAstToken notDeclarationStart;
        CfAstToken variableTypeMissing;
        CfAstToken exprValueRequired;
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
