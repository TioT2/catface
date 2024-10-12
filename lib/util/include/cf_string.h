/**
 * @brief string-related utilities declaration file
 */

#ifndef CF_STRING_H_
#define CF_STRING_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief check if string if start of another one
 * 
 * @param[in] string string to check start of (non-null)
 * @param[in] start  starting string (non-null)
 * 
 * @return true if string starts from start, false otherwise
 */
bool cfStrStartsWith( const char *string, const char *start );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_STRING_H_)

// cf_string.h
