/**
 * @brief cfasm file assembler utility
 */

#include <cf_disassembler.h>
#include <cf_cli.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

/**
 * @brief text from file reading function
 * 
 * @param[in]  file   file to read pointer
 * @param[out] dst    reading destination, function will allocate space for result (non-null)
 * @param[out] dstLen readed line text (non-null)
 * 
 * @return true if succeeded, false otherwise
 */
bool readFile( FILE *file, char **dst, size_t *dstLen ) {
    assert(dst != NULL);
    assert(dstLen != NULL);

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)calloc(len + 1, 1);
    if (buffer == NULL)
        return false;

    fread(buffer, 1, len, file);

    *dst = buffer;
    if (dstLen != NULL)
        *dstLen = len;

    return true;
} // readFile

/**
 * @brief help printing function
 */
void printHelp( void ) {
    puts(
        "Usage:  cf_disassembler [options] input\n"
        "\n"
        "Options:\n"
        "    -h              Display this message\n"
    );
} // printHelp

int main( const int _argc, const char **_argv ) {
    // const int argc = 2;
    // const char *argv[] = { "qq", "examples/sqrt_repl.cfmod" };
    const int argc = _argc;
    const char **argv = _argv;

    // quite strange solution, but it's ok because last argument is treated as input file name.
    if (argc < 2 || 0 == strcmp(argv[1], "-h")) {
        printHelp();
        return 0;
    }

    // it's obviously NOT overengineering
    const CfCommandLineOptionInfo optionInfos[1] = {
        {"h", "help",   0},
    };
    int optionIndices[1];
    const size_t optionCount = 1;
    if (!cfParseCommandLineOptions(argc - 2, argv + 1, optionCount, optionInfos, optionIndices)) {
        // it's ok for cli utils to display something in stdout, so corresponding error message is already displayed.
        return 0;
    }

    if (optionIndices[0] != -1) {
        printHelp();
        return 0;
    }

    struct {
        const char *inputFileName;
    } options = {
        .inputFileName = argv[argc - 1],
    };

    CfExecutable executable;

    FILE *input = fopen(options.inputFileName, "rb");
    if (input == NULL) {
        printf("input file opening error: %s\n", strerror(errno));
        return 0;
    }
    CfExecutableReadStatus readStatus = cfExecutableRead(input, &executable);
    fclose(input);


    if (readStatus != CF_EXECUTABLE_READ_STATUS_OK) {
        printf("input executable file reading error: %s\n", cfExecutableReadStatusStr(readStatus));
        return 0;
    }

    char *text;
    CfDisassemblyDetails disassemblyDetails;
    CfDisassemblyStatus disassemblyStatus = cfDisassemble(&executable, &text, &disassemblyDetails);

    if (disassemblyStatus != CF_DISASSEMBLY_STATUS_OK) {
        printf("assembling failed.\n");
        cfDisassemblyDetailsDump(stdout, disassemblyStatus, &disassemblyDetails);
        printf("\n");

        cfExecutableDtor(&executable);
        return 0;
    }

    puts(text);
    cfExecutableDtor(&executable);

    return 0;
} // main

// main.cpp
