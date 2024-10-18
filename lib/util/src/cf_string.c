#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "cf_string.h"

int cfWriteStr( FILE *file, CfStr slice ) {
    return fprintf(file, "%*s", (int)(slice.end - slice.begin), slice.begin);
} // cfWriteStr

bool cfStrStartsWith( CfStr slice, const char *start ) {
    if (slice.begin >= slice.end)
        return *start == '\0';

    for (;;) {
        const char sliceChar = *slice.begin;
        const char startChar = *start;

        if (sliceChar != startChar)
            return false;
        slice.begin++;
        start++;
        if (slice.begin >= slice.end || startChar == '\0')
            return true;
    }
} // cfStrStartsWith

bool cfRawStrStartsWith( const char *string, const char *start ) {
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
} // cfStrStartsWith

CfStr cfStrParseHexadecmialInteger( CfStr slice, uint64_t *dst ) {
    uint64_t result = 0;

    while (slice.begin != slice.end) {
        const char ch = *slice.begin;
        uint64_t digit;

        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else if (ch >= 'A' && ch <= 'F') {
            digit = ch - 'A' + 10;
        } else if (ch >= 'a' && ch <= 'f') {
            digit = ch - 'a' + 10;
        } else {
            break;
        }

        slice.begin++;
        result = result * 16 + digit;
    }

    if (dst != NULL)
        *dst = result;

    return slice;
} // cfSliceParseHexadecimalInteger


CfStr cfStrParseDecimalInteger( CfStr slice, uint64_t *dst ) {
    uint64_t result = 0;

    while (slice.begin != slice.end) {
        const char ch = *slice.begin;
        uint64_t digit;

        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else {
            break;
        }

        result = result * 10 + digit;
        slice.begin++;
    }

    if (dst != NULL)
        *dst = result;

    return slice;
} // cfStrParseDecimalInteger

CfStr cfStrParseDecimal( CfStr slice, CfParsedDecimal *dst ) {
    CfParsedDecimal result = {0};
    CfStr newSlice;

    newSlice = cfStrParseDecimalInteger(slice, &result.integer);
    slice = newSlice;

    // parse floating part
    if (slice.begin != slice.end && *slice.begin == '.') {
        slice.begin++;
        result.fractionalStarted = true;

        uint64_t frac;
        // parse fractional
        newSlice = cfStrParseDecimalInteger(slice, &frac);
        result.fractional = frac * pow(10.0, -(double)(newSlice.begin - slice.begin));
        slice = newSlice;
    }

    // parse exponential part
    if (slice.begin != slice.end && *slice.begin == 'e') {
        int64_t sign = 1;
        result.exponentStarted = true;

        if (slice.begin != slice.end) {
            if (*slice.begin == '-')
                slice.begin++, sign = -1;
            else if (*slice.begin == '+')
                slice.begin++;
        }

        uint64_t unsignedExponent;
        newSlice = cfStrParseDecimalInteger(slice, &unsignedExponent);
        result.exponent = (int64_t)unsignedExponent * sign;
        slice = newSlice;
    }

    if (dst != NULL)
        *dst = result;

    return slice;
} // cfStrParseDecimal

/**
 * @brief decimal into double composition function
 * 
 * @param[in] decimal decimal to compose double from
 * 
 * @return decimal composed into double
 */
double cfParsedDecimalCompose( const CfParsedDecimal *decimal ) {
    assert(decimal != NULL);

    return (decimal->integer + decimal->fractional) * pow(10.0, decimal->exponent);
} // cfParsedDecimalCompose

bool cfStrIsSame( CfStr lhs, CfStr rhs ) {
    // compare lengths
    if (lhs.end - lhs.begin != rhs.end - rhs.begin)
        return false;

    // then compare contents
    while (lhs.begin < lhs.end)
        if (*lhs.begin++ != *rhs.begin++)
            return false;
    return true;
} // cfStrIsSame

// cf_string.cpp
