#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "cf_string.h"

size_t cfStrLength( CfStr str ) {
    return str.end - str.begin;
} // cfStrLength

void cfStrWrite( FILE *file, CfStr slice ) {
    fprintf(file, "%.*s", (int)(slice.end - slice.begin), slice.begin);
} // cfStrWrite

void cfStrWriteShielded( FILE *file, CfStr slice ) {
    for (const char *ch = slice.begin; ch < slice.end; ch++)
        switch (*ch) {
        case '\b': fputs("\\b",  file); break;
        case '\f': fputs("\\f",  file); break;
        case '\n': fputs("\\n",  file); break;
        case '\r': fputs("\\r",  file); break;
        case '\t': fputs("\\t",  file); break;
        case '\"': fputs("\\\"", file); break;
        case '\'': fputs("\\\'", file); break;
        case '\\': fputs("\\",   file); break;
        default  : fputc(*ch,    file);
        }
} // cfStrWriteShielded

bool cfStrStartsWith( CfStr slice, const char *start ) {
    if (slice.begin >= slice.end)
        return *start == '\0';

    for (;;) {
        const char sliceChar = *slice.begin;
        const char startChar = *start;

        if (startChar == '\0')
            return true;

        if (sliceChar != startChar)
            return false;
        slice.begin++;
        start++;
        if (slice.begin >= slice.end)
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


char * cfStrOwnedCopy( CfStr str ) {
    size_t len = str.end - str.begin;

    char *buffer = (char *)calloc(len + 1, sizeof(char));
    if (buffer == NULL)
        return NULL;
    memcpy(buffer, str.begin, len);
    buffer[len] = '\0';

    return buffer;
} // strOwnedCopy

CfStr cfStrSubstr( CfStr str, CfStrSpan span ) {
    CfStr result = {
        .begin = str.begin + span.begin,
        .end = str.begin + span.end,
    };

    return (CfStr) {
        .begin = result.begin < str.end
            ? result.begin
            : str.end,

        .end = result.end < str.end
            ? result.end
            : str.end,
    };
} // cfStrSubstr

// cf_string.cpp
