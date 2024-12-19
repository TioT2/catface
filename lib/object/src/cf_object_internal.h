/**
 * @brief object internal declaration file
 */

#ifndef CF_OBJECT_INTERNAL_H_
#define CF_OBJECT_INTERNAL_H_

#include "cf_object.h"

#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)

/// @brief neon-genesis object structure
struct CfObject2_ {
    CfLabel * labelArray;       ///< label array
    size_t    labelArrayLength; ///< label array length
    CfLink  * linkArray;        ///< link array
    size_t    linkArrayLength;  ///< link array length
    void    * code;             ///< object code
    size_t    codeLength;       ///< object length
}; // struct CfObject2_

#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#endif // !defined(CF_OBJECT_INTERNAL_H_)

// cf_object_internal.h
