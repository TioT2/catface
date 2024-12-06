/**
 * @brief AST main declaration file
 */

#ifndef CF_AST_H_
#define CF_AST_H_

#include <stdint.h>
#include <stddef.h>

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
    const char * name; ///< parameter name
    CfAstType    type; ///< parameter type
    CfAstSpan    span; ///< span function param located in
} CfAstFunctionParam;

/// @brief function declaration structure
typedef struct __CfAstFunction {
    const char         * name;       ///< name
    CfAstFunctionParam * params;     ///< parameter array (owned)
    size_t               paramCount; ///< parameter array size
    CfAstType            returnType; ///< returned function type
    CfAstBlock         * impl;       ///< implementation
    CfAstSpan            span;       ///< span function located in
} CfAstFunction;

/// @brief variable declaration structure
typedef struct __CfAstVariable {
    const char * name; ///< name
    CfAstType    type; ///< type
    CfAstExpr  * init; ///< initializer expression (may be null)
    CfAstSpan    span; ///< span variable declaration located in
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
const char * cfAstGetSourceFileName( const CfAst ast );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_H_)

// cf_ast.h
