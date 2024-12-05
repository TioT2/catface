/**
 * @brief AST main declaration file
 */

#ifndef CF_AST_H_
#define CF_AST_H_

#include <cf_string.h>

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

/// @brief span representation structure
typedef struct __CfAstSpan {
    CfStr  file;  ///< file span located in
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
    CfStr     name; ///< parameter name
    CfAstType type; ///< parameter type
    CfAstSpan span; ///< span function param located in
} CfAstFunctionParam;

/// @brief function declaration structure
typedef struct __CfAstFunction {
    CfStr                name;       ///< name
    CfAstFunctionParam * params;     ///< parameter array (owned)
    size_t               paramCount; ///< parameter array size
    CfAstType            returnType; ///< returned function type
    CfAstBlock         * impl;       ///< implementation
    CfAstSpan            span;       ///< span function located in
} CfAstFunction;

/// @brief variable declaration structure
typedef struct __CfAsttVariable {
    CfStr       name; ///< name
    CfAstType   type; ///< type
    CfAstExpr * init; ///< initializer expression (may be null)
    CfAstSpan   span; ///< span variable declaration located in
} CfAstVariable;

/// @brief declaration structure implementation
struct __CfAstDecl {
    CfAstDeclType type; ///< declaration union tag
    CfAstSpan     span; ///< span declaration located in

    union {
        CfAstFunction fn;  ///< function declaration
        CfAstVariable let; ///< 'let' expression
    };
}; // struct CfAstDecl

/// @brief block (curly brace enclosed statement sequence) representation structure
struct __CfAstBlock {
    CfAstStmt * stmts;     ///< statement array
    size_t      stmtCount; ///< statement array size
    CfAstSpan   span;      ///< span block located in
}; // struct __CfAstBlock

/// @brief statement union tag
typedef enum __CfAstStmtType {
    CF_AST_STMT_TYPE_EXPRESSION,  ///< expression
    CF_AST_STMT_TYPE_DECLARATION, ///< statement that declares something
    CF_AST_STMT_TYPE_BLOCK,       ///< block statement (curly brace enclosed sequence)
} CfAstStmtType;

/// @brief statement repersentation structure
struct __CfAstStmt {
    CfAstStmtType type; ///< statement type
    CfAstSpan     span; ///< span statement located in

    union {
        CfAstExpr  * expression;  ///< expression statament
        CfAstDecl  * declaration; ///< declarative expression
        CfAstBlock * block;       ///< block expression
    };
}; // struct __CfAstStmt

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

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_AST_H_)

// cf_ast.h
