#include <string.h>
#include <stdio.h>

#include "cf_cli.h"
#include "cf_string.h"

bool cfParseCommandLineOptions(
    const int argc,
    const char **argv,
    const size_t optionCount,
    const CfCommandLineOptionInfo *optionInfos,
    int *optionIndices
) {
    size_t index = 0;

    // fill optionIndices array with -1's
    memset(optionIndices, 0xFF, sizeof(int) * optionCount);

    while (index < argc) {
        const char *argument = NULL;
        bool longNameRequired;

        if (cfStrStartsWith(argv[index], "--")) {
            argument = argv[index] + 2;
            longNameRequired = true;
        } else if (cfStrStartsWith(argv[index], "-")) {
            argument = argv[index] + 1;
            longNameRequired = false;
        } else {
            printf("'%s' occured, option expected\n", argv[index]);
            return false;
        }

        bool found = false;
        size_t paramCount = 0;

        size_t optionIndex;
        for (optionIndex = 0; optionIndex < optionCount; optionIndex++) {
            const char *name;

            if (longNameRequired)
                name = optionInfos[optionIndex].longName;
            else
                name = optionInfos[optionIndex].shortName;

            if (0 == strcmp(argument, name)) {
                found = true;
                paramCount = optionInfos[optionIndex].paramCount;
                break;
            }
        }

        if (!found) {
            printf("unknown command line option: '%s'\n", argv[index]);
            return false;
        }

        if (index + 1 + paramCount > argc) {
            printf("no enough arguments for '%s' option\n", argv[index]);
            return false;
        }

        if (optionIndices[optionIndex] == -1)
            optionIndices[optionIndex] = index + 1;
        index += paramCount + 1;
    }

    return true;
} // cfParseCommandLineOptions

// cf_cli.cpp
