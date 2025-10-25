/**
 * @brief logger implemetnation file
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "cf_logger.h"

/// @brief logger instance
static struct {
    bool   isInitialized; ///< logger initialization flag
    FILE * out;           ///< out file
} CF_logger;

void cfLoggerInitialize( void ) {
    if (CF_logger.isInitialized)
        return;

    CF_logger.out = fopen("log.txt", "at");

    CF_logger.isInitialized = (CF_logger.out != NULL);
} // cfLoggerInitialize

void cfLoggerTerminate( void ) {
    if (!CF_logger.isInitialized)
        return;

    fclose(CF_logger.out);
} // cfLoggerTerminate

void cfLoggerPrint( const char *fstr, ... ) {
    if (!CF_logger.isInitialized)
        return;

    va_list list;

    va_start(list, fstr);
    vfprintf(CF_logger.out, fstr, list);
    va_end(list);

    va_start(list, fstr);
    vfprintf(stdout, fstr, list);
    va_end(list);
} // cfLoggerPrint

// cf_logger.c
