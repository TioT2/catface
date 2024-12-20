/**
 * @brief CLI-related utils declaration file
 */

#ifndef CF_CLI_H_
#define CF_CLI_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Info about command line option
typedef struct CfCommandLineOptionInfo_ {
    const char * shortName;  ///< short command name (used starting with '-', nullable)
    const char * longName;   ///< long command name (used starting with '--', nullable)
    uint8_t      paramCount; ///< consumed command parameter count
} CfCommandLineOptionInfo;

/**
 * @brief command line options parsing function
 * 
 * @param[in]  argc          command line argument count
 * @param[in]  argv          command line argument values (non-null)
 * @param[in]  optionCount   count of options to parse from argc/argv (non-null)
 * @param[in]  optionInfos   infos about command line options
 * @param[out] optionIndices indices of first argument of first occurrence of command line option (non-null, must have 'optionCount' size)
 * 
 * @return true if parsed, false if not
 */
bool cfParseCommandLineOptions(
    const int argc,
    const char **argv,
    size_t optionCount,
    const CfCommandLineOptionInfo *optionInfos,
    int *optionIndices
);

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_CLI_H_)

// cf_cli.h
