#include <assert.h>

#include "cf_string.h"

bool cfStrStartsWith( const char *string, const char *start ) {
    assert(string != NULL);
    assert(start != NULL);

    for (;;) {
        const char stringChar = *string++;
        const char startChar = *start++;

        if (stringChar == '\0' || startChar == '\0')
            return true;
        if (stringChar != startChar)
            return false;
    }
} // cfStrStartsW

// cf_string.cpp
