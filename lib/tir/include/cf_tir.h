/**
 * @brief TIR (Typed Intermediate Representation) main declaration file
 */

#ifndef CF_TIR_H_
#define CF_TIR_H_

#ifdef __cplusplus
extern "C" {
#endif

/// @brief TIR structure
typedef struct CfTir_ CfTir;

/// @brief primitive
typedef enum __CfTirType {
    CF_TIR_TYPE_I32,  ///< 32-bit signed integer primitive type
    CF_TIR_TYPE_U32,  ///< 32-bit unsigned integer primitive type
    CF_TIR_TYPE_F32,  ///< 32-bit floating-point primitive type
    CF_TIR_TYPE_VOID, ///< zero-sized primitive type
} CfTirType;

typedef enum __CfCfTirExpressionType {
    CF_TIR_EXPRESSION_TYPE_I32, ///< 
    CF_TIR_EXPRESSION_TYPE_F32, ///< 
    CF_TIR_EXPRESSION_TYPE_U32, ///< 
} CfTirExpressionType;

typedef struct CfTirExpression {
    CfTirExpressionType type;
    CfTirType           resultingType;
} CfTirExpression;

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_TIR_H_)

// cf_tir.h
