/**
 * @brief hash implementation function (SHA256 algorithm used)
 */

#include <assert.h>
#include <string.h>

#include "cf_hash.h"

/// @brief SHA256 constant table
const static uint32_t CF_constTable[64] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
}; // CF_constTable constant end

/**
 * @brief left bit rotation function
 * 
 * @param[in] n  number to rotate
 * @param[in] at count of bits to rotate at (should be less than 32, actually)
 * 
 * @return bit-rotated 32-bit number
 */
static inline uint32_t cfRotl32( const uint32_t n, const uint32_t at ) {
    assert(at <= 32);
    return (n << at) | (n >> (32 - at));
} // cfRotl32 function end

/**
 * @brief right bit rotation function
 * 
 * @param[in] n  number to rotate
 * @param[in] at count of bits to rotate at (should be less than 32, actually)
 * 
 * @return bit-rotated 32-bit number
 */
static inline uint32_t cfRotr32( const uint32_t n, const uint32_t at ) {
    assert(at <= 32);
    return (n >> at) | (n << (32 - at));
} // cfRotr32 function end

/**
 * @brief hash calculation step performing function
 * 
 * @param[in,out] hash  hash calculation destination
 * @param[in]     batch chunk of data to perform step on
 */
static void cfHashStep( CfHash *const hash, const uint32_t *const batch ) {
    uint32_t words[64] = {0};

    memcpy(words, batch, sizeof(uint32_t) * 16);

    // reverse word byteorder
    for (size_t wi = 0; wi < 16; wi++) {
        words[wi] = ((words[wi] >> 16) & 0x0000FFFF) | ((words[wi] << 16) & 0xFFFF0000);
        words[wi] = ((words[wi] >>  8) & 0x00FF00FF) | ((words[wi] <<  8) & 0xFF00FF00);
    }

    // fill words
    for (size_t i = 16; i < 64; i++) {
        uint32_t w0 = words[i - 15];
        uint32_t w1 = words[i -  2];

        uint32_t s0 = cfRotr32(w0,  7) ^ cfRotr32(w0, 18) ^ (w0 >>  3);
        uint32_t s1 = cfRotr32(w1, 17) ^ cfRotr32(w1, 19) ^ (w1 >> 10);

        words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }

    uint32_t a = hash->hash[0];
    uint32_t b = hash->hash[1];
    uint32_t c = hash->hash[2];
    uint32_t d = hash->hash[3];
    uint32_t e = hash->hash[4];
    uint32_t f = hash->hash[5];
    uint32_t g = hash->hash[6];
    uint32_t h = hash->hash[7];

    for (size_t i = 0; i < 64; i++) {
        uint32_t sum0 = cfRotr32(a, 2) ^ cfRotr32(a, 13) ^ cfRotr32(a, 22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = sum0 + majority;

        uint32_t sum1 = cfRotr32(e, 6) ^ cfRotr32(e, 11) ^ cfRotr32(e, 25);
        uint32_t choice = (e & f) ^ ((~e) & g);

        uint32_t temp1 = h + sum1 + choice + CF_constTable[i] + words[i];

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    hash->hash[0] += a;
    hash->hash[1] += b;
    hash->hash[2] += c;
    hash->hash[3] += d;
    hash->hash[4] += e;
    hash->hash[5] += f;
    hash->hash[6] += g;
    hash->hash[7] += h;
} // cfHashStep function end

void cfHasherInitialize( CfHasher *const hasher ) {
    assert(hasher != NULL);

    // setup hashing context
    hasher->hash = (CfHash){{
        0x6A09E667,
        0xBB67AE85,
        0x3C6EF372,
        0xA54FF53A,
        0x510E527F,
        0x9B05688C,
        0x1F83D9AB,
        0x5BE0CD19,
    }};

    hasher->batchSize = 0;
    hasher->totalSize = 0;

    // fill hash batch
    memset(hasher->batch, 0, 64);
} // cfHasherInitialize

CfHash cfHasherTerminate( CfHasher *const hasher ) {
    // batch size is always < 64, so it's ok.
    hasher->batch[hasher->batchSize] = 128;

    // check for case where size isn't fitting in last batch
    if (64 - hasher->batchSize < 9) {
        // handle 1st batch
        cfHashStep(&hasher->hash, (const uint32_t *)&hasher->batch);

        // start 2nd batch
        memset(hasher->batch, 0, 56);
    }

    // write finlal block

    uint64_t lastBatchRev = hasher->totalSize * 8;

    lastBatchRev = ((lastBatchRev >> 32) & 0x00000000FFFFFFFF) | ((lastBatchRev << 32) & 0xFFFFFFFF00000000);
    lastBatchRev = ((lastBatchRev >> 16) & 0x0000FFFF0000FFFF) | ((lastBatchRev << 16) & 0xFFFF0000FFFF0000);
    lastBatchRev = ((lastBatchRev >>  8) & 0x00FF00FF00FF00FF) | ((lastBatchRev <<  8) & 0xFF00FF00FF00FF00);

    ((uint64_t *)hasher->batch)[7] = lastBatchRev;
    cfHashStep(&hasher->hash, (const uint32_t *)&hasher->batch);

    return hasher->hash;
} // cfHasherTerminate

void cfHasherStep(
    CfHasher *const hasher,
    const void        *const data,
    const size_t             size
) {
    assert(hasher != NULL);
    assert(data != NULL);

    size_t sizeRest = size;
    uint8_t *dataRest = (uint8_t *)data;

    // increment total hashed data sizeo
    hasher->totalSize += size;

    do {
        // count of bytes to write to batch
        size_t writeCount = sizeRest < 64 - hasher->batchSize
            ? sizeRest
            : 64 - hasher->batchSize;

        // write data to batch
        memcpy(
            hasher->batch,
            dataRest,
            writeCount
        );

        dataRest += writeCount;
        sizeRest -= writeCount;

        // increment batch size
        hasher->batchSize += writeCount;

        // append to hash if hasher batch is full
        if (hasher->batchSize == 64) {
            cfHashStep(&hasher->hash, (const uint32_t *)hasher->batch);
            memset(hasher->batch, 0, 64);
            hasher->batchSize = 0;
        }
    } while (sizeRest > 0);
} // cfHasherStep

CfHash cfHash( const void *data, const size_t size ) {
    CfHasher hasher = {0};

    cfHasherInitialize(&hasher);
    cfHasherStep(&hasher, data, size);

    return cfHasherTerminate(&hasher);
} // cfHash

bool cfHashCompare( const CfHash *lhs, const CfHash *rhs ) {
    return 0 == memcmp(lhs, rhs, sizeof(CfHash));
} // cfHashCompare

// stk_hash.cpp file end
