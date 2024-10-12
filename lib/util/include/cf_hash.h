/**
 * @brief hasher declaration file
 */

#ifndef CF_HASH_H_
#define CF_HASH_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/// @brief SHA256 hash representation structure
typedef struct __CfHash {
    uint32_t hash[8]; ///< hash numbers
} CfHash;

/**
 * @brief hash calculation function
 * 
 * @param[in] data data to calculate hash of pointer (non-nullable)
 * @param[in] size data size (in bytes)
 * 
 * @return hash
 */
CfHash cfHash( const void *data, const size_t size );

/**
 * @brief hash comparison function
 * 
 * @param[in] lhs first hash
 * @param[in] rhs second hash
 * 
 * @return true if lhs and rhs hashes are same
 */
bool cfHashCompare( const CfHash *lhs, const CfHash *rhs );

#endif // !defined(CF_HASH_H_)

// cf_hash.h
