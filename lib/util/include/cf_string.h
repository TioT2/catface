/**
 * @brief string-related utilities declaration file
 */

#ifndef CF_STRING_H_
#define CF_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <cf_ordering.h>

/// @brief string slice representation structure
typedef struct CfStr_ {
    const char *begin; ///< slice begin
    const char *end;   ///< slice end (non-inclusive, may not point on null)
} CfStr;

/// @brief zero-terminated string to str conversion macro
#define CF_STR(literal) ((CfStr){ (literal), (literal) + strlen((literal)) })

/**
 * @brief string slice length getting function
 * 
 * @param[in] str string slice to get length of
 * 
 * @return string slice length
 */
size_t cfStrLength( CfStr str );

/**
 * @brief string slice printing function
 * 
 * @param[in] file  file to print slice to
 * @param[in] slice slice to print
 * 
 * @return count of characters printed.
 */
void cfStrWrite( FILE *file, CfStr slice );

/**
 * @brief shielded string slice printing function
 * 
 * @param[in] file  file to print slice to
 * @param[in] slice slice to print
 */
void cfStrWriteShielded( FILE *file, CfStr slice );

/**
 * @brief slice starting with zero-terminated string checking function
 * 
 * @param[in] slice slice to perform check for
 * @param[in] start string to check by
 * 
 * @return true if slice starts from string, false otherwise.
 */
bool cfStrStartsWith( CfStr slice, const char *start );

/**
 * @brief check if string if start of another one
 * 
 * @param[in] string string to check start of (non-null)
 * @param[in] start  starting string (non-null)
 * 
 * @return true if string starts from start, false otherwise
 */
bool cfRawStrStartsWith( const char *string, const char *start );

/**
 * @brief hexadecimal integer from slice parsing function
 * 
 * @param[in]  slice slice to parse integer from
 * @param[out] dst   parsing destination (nullable)
 * 
 * @return subslice of slice with all data except of characters that belongs to integer.
 * @note resulting subslice lies in same pointer range with 'slice' parameter.
 */
CfStr cfStrParseHexadecmialInteger( CfStr slice, uint64_t *dst );

/**
 * @brief decimal integer from slice parsing function
 * 
 * @param[in]  slice slice to parse integer from
 * @param[out] dst   parsing destination (nullable)
 * 
 * @return subslice of slice with all data except of characters that belongs to integer.
 * @note returned subslice IS subslice of input slice.
 */
CfStr cfStrParseDecimalInteger( CfStr slice, uint64_t *dst );

/**
 * @brief string comparison function
 * 
 * @param[in] lhs left hand side
 * @param[in] rhs right hand side
 * 
 * @return true if lhs and rhs str-s contains same characters, false otherwise.
 */
bool cfStrIsSame( CfStr lhs, CfStr rhs );

/// @brief parsed decimal number representation structure
typedef struct CfParsedDecimal_ {
    uint64_t integer;           ///< intefger part
    bool     fractionalStarted; ///< fractional part started ('.' occured)
    double   fractional;        ///< fractional part (MUST BE zero if not occured, MUST BE < 1.0)
    bool     exponentStarted;   ///< exponential part started ('e' occured)
    int64_t  exponent;          ///< exponential part (MUST BE zero if not occured)
} CfParsedDecimal;

/**
 * @brief decimal number from slice parsing function
 * 
 * @param[in]  slice slice to parse string from
 * @param[out] dst   parsing destination
 * 
 * @result subslice with all data except of data that belongs to decimal.
 * @note returned subslice IS subslice of original slice.
 */
CfStr cfStrParseDecimal( CfStr slice, CfParsedDecimal *dst );

/**
 * @brief decimal into double composition function
 * 
 * @param[in] decimal decimal to compose double from
 * 
 * @return decimal composed into double
 */
double cfParsedDecimalCompose( const CfParsedDecimal *decimal );

/**
 * @brief owned slice copy getting function
 * 
 * @param[in]  str slice to get owned copy of
 * 
 * @return copied zero-terminated string
 * @note resulting stirng is zero-terminated, so it's ok to use it with C functions
 * @note it is ok to delete return value just by free() of it's .begin
 */
char * cfStrOwnedCopy( CfStr str );

/// @brief string span representation structure
typedef struct __CfStrSpan {
    uint32_t begin; ///< start index
    uint32_t end;   ///< end index
} CfStrSpan;

/**
 * @brief get substring from string by span
 * 
 * @param[in] str  string to get substring from
 * @param[in] span span to get substring by
 * 
 * @return substring
 */
CfStr cfStrSubstr( CfStr str, CfStrSpan span );

/**
 * @brief str comparator
 * 
 * @param[in] lhs left hand side (non-null)
 * @param[in] rhs right hand side (non-null)
 */
CfOrdering cfStrComparator( const void *lhs, const void *rhs );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_STRING_H_)

// cf_string.h
