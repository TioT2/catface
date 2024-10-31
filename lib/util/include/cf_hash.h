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
typedef struct __CfHash {
    uint16_t hash[8]; ///< hash numbers
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

/// @brief hashing context
typedef struct __CfHashContext {
    uint32_t h0; ///< value #0
    uint32_t h1; ///< value #1
    uint32_t h2; ///< value #2
    uint32_t h3; ///< value #3
    uint32_t h4; ///< value #4
    uint32_t h5; ///< value #5
    uint32_t h6; ///< value #6
    uint32_t h7; ///< value #7
} CfHashContext;

/// @brief iterative hasher representation structure
typedef struct __CfIterativeHasher {
    CfHashContext context;   ///< hashing context
    uint8_t       batch[64]; ///< data batch
    size_t        batchSize; ///< current batch size
    size_t        totalSize; ///< total hashed byte count
} CfIterativeHasher;

/**
 * @brief iterative hasher initialization function
 * 
 * @param[in,out] hasher hasher to initialize pointer (non-null)
 */
void cfIterativeHasherInitialize( CfIterativeHasher *hasher );

/**
 * @brief iterative hasher termination function
 * 
 * @param[in,out] hasher hasher to terminate pointer (non-null)
 * 
 * @return resulting hash
 */
CfHash cfIterativeHasherTerminate( CfIterativeHasher *hasher );

/**
 * @brief iterative hasher step preferring function
 * 
 * @param[in,out] hasher hasher to perform step in (non-null)
 * @param[in]     data   data block to hash (non-null)
 * @param[in]     size   data block size
 */
void cfIterativeHasherStep( CfIterativeHasher *hasher, const void *data, size_t size );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_HASH_H_)

// cf_hash.h
