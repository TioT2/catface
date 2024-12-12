/**
 * @brief hasher declaration file
 */

#ifndef CF_HASH_H_
#define CF_HASH_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief SHA256 hash representation structure
typedef struct CfHash_ {
    uint32_t hash[8]; ///< hash numbers
} CfHash;

/// @brief iterative hasher representation structure
typedef struct CfHasher_ {
    CfHash  hash;      ///< hash itself
    uint8_t batch[64]; ///< data buffer
    size_t  batchSize; ///< current batch size
    size_t  totalSize; ///< total hashed byte count
} CfHasher;

/**
 * @brief hasher initialization function
 * 
 * @param[in,out] hasher hasher to initialize pointer (non-null)
 */
void cfHasherInitialize( CfHasher *hasher );

/**
 * @brief hasher termination function
 * 
 * @param[in,out] hasher hasher to terminate pointer (non-null)
 * 
 * @return resulting hash
 */
CfHash cfHasherTerminate( CfHasher *hasher );

/**
 * @brief iterative hasher step preferring function
 * 
 * @param[in,out] hasher hasher to perform step in (non-null)
 * @param[in]     data   data block to hash (non-null)
 * @param[in]     size   data block size (in bytes)
 */
void cfHasherStep( CfHasher *hasher, const void *data, size_t size );

/**
 * @brief hash calculation function
 * 
 * @param[in] data data to calculate hash of pointer (non-null)
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

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_HASH_H_)

// cf_hash.h
