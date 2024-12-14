/**
 * @brief ordering declaration file
 */

#ifndef CF_ORDERING_H_
#define CF_ORDERING_H_

/// @brief ordering structure
typedef enum CfOrdering_ {
    CF_ORDERING_LESS    = -1, ///< less
    CF_ORDERING_EQUAL   =  0, ///< equal
    CF_ORDERING_GREATER = +1, ///< greater
} CfOrdering;

/**
 * @brief comparator function
 * 
 * @param[in] lhs left hand side (non-null)
 * @param[in] rhs right hand side (non-null)
 * 
 * @return lhs and rhs ordering
 */
typedef CfOrdering (*CfComparator)( const void *lhs, const void *rhs );

#endif // !defined(CF_ORDERING_H_)

// cf_ordering.h
