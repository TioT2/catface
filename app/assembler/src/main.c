/**
 * @brief cfasm file assembler utility
 */

#include <cf_asm.h>
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
        "Usage:  cf_assembler [options] input\n"
        "\n"
        "Options:\n"
        "    -h              Display this message\n"
        "    -o <filename>   Write output to <filename>\n"
    );
} // printHelp

int main( const int _argc, const char **_argv ) {
    // const int argc = 4;
    // const char *argv[] = {
    //     "qq",
    //     "-o",
    //     "examples/fisqrt.cfmod",
    //     "examples/fisqrt.cfasm",
    // };
    const int argc = _argc;
    const char **argv = _argv;

    // quite strange solution, but it's ok because last argument is treated as input file name.
    if (argc < 2 || 0 == strcmp(argv[1], "-h")) {
        printHelp();
        return 0;
    }

    // it's obviously NOT overengineering
    const int optionCount = 2;
    const CfCommandLineOptionInfo optionInfos[optionCount] = {
        {"o", "output", 1},
        {"h", "help",   0},
    };
    int optionIndices[optionCount];
    if (!cfParseCommandLineOptions(argc - 2, argv + 1, optionCount, optionInfos, optionIndices)) {
        // it's ok for cli utils to display something in stdout, so corresponding error message is already displayed.
        return 0;
    }

    if (optionIndices[1] != -1) {
        printHelp();
        return 0;
    }

    struct {
        const char *inputFileName;
        const char *outputFileName;
    } options = {
        .inputFileName = argv[argc - 1],
        .outputFileName = "out.cfmod",
    };

    // read filename from options
    if (optionIndices[0] != -1)
        options.outputFileName = argv[optionIndices[0] + 1];

    char *initialText = NULL;
    size_t initialTextLen = 0;

    FILE *input = fopen(options.inputFileName, "r");
    if (input == NULL) {
        printf("input file opening error: %s\n", strerror(errno));
        return 0;
    }
    bool readSuccess = readFile(input, &initialText, &initialTextLen);
    fclose(input);

    if (!readSuccess) {
        printf("input file reading error occured\n");
        return 0;
    }

    CfModule module;
    CfAssemblyDetails assemblyDetails;
    CfAssemblyStatus assemblyStatus = cfAssemble(initialText, initialTextLen, &module, &assemblyDetails);

    if (assemblyStatus != CF_ASSEMBLY_STATUS_OK) {
        printf("assembling failed.\n");
        cfAssemblyDetailsDump(stdout, assemblyStatus, &assemblyDetails);
        printf("\n");
        free(initialText);
        return 0;
    }

    FILE *output = fopen(options.outputFileName, "wb");
    if (output == NULL) {
        printf("output file opening error: %s\n", strerror(errno));
        return 0;
    }
    CfModuleWriteStatus moduleWriteStatus = cfModuleWrite(&module, output);
    if (moduleWriteStatus != CF_MODULE_WRITE_STATUS_OK) {
        printf("module to output file writing error: %s\n", cfModuleWriteStatusStr(moduleWriteStatus));

        fclose(output);
        free(initialText);
        cfModuleDtor(&module);
        return 0;
    }
    fclose(output);

    free(initialText);
    cfModuleDtor(&module);

    return 0;
} // main

// main.cpp
