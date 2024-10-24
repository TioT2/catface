/**
 * @brief cfasm file assembler utility
 */

#include <cf_assembler.h>
#include <cf_linker.h>
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
        "    -l              Link result (emit executable)\n"
        "    -o <filename>   Write output to <filename>\n"
    );
} // printHelp

int main( const int _argc, const char **_argv ) {
    // const int argc = 5;
    // const char *argv[] = {
    //     "qq",
    //     "-o",
    //     "examples/sin.cfexe",
    //     "-l",
    //     "examples/sin.cfasm",
    // };
    const int argc = _argc;
    const char **argv = _argv;

    // quite strange solution, but it's ok because last argument is treated as input file name.
    if (argc < 2 || 0 == strcmp(argv[1], "-h")) {
        printHelp();
        return 0;
    }

    // it's obviously NOT overengineering
    const int optionCount = 3;
    CfCommandLineOptionInfo optionInfos[3] = {
        {"o", "output", 1},
        {"l", "link",   0},
        {"h", "help",   0},
    };
    int optionIndices[3];

    if (!cfParseCommandLineOptions(argc - 2, argv + 1, optionCount, optionInfos, optionIndices)) {
        // it's ok for cli utils to display something in stdout, so corresponding error message is already displayed.
        return 0;
    }

    if (optionIndices[2] != -1) {
        printHelp();
        return 0;
    }

    struct {
        const char *inputFileName;
        const char *outputFileName;
        bool linkOutput;
    } options = {
        .inputFileName = argv[argc - 1],
        .outputFileName = "out.cfexe",
        .linkOutput = (optionIndices[1] != -1),
    };

    // read filename from options
    if (optionIndices[0] != -1)
        options.outputFileName = argv[optionIndices[0] + 1];

    char *text = NULL;
    size_t textLen = 0;

    FILE *input = fopen(options.inputFileName, "r");
    if (input == NULL) {
        printf("input file opening error: %s\n", strerror(errno));
        return 0;
    }
    bool readSuccess = readFile(input, &text, &textLen);
    fclose(input);

    if (!readSuccess) {
        printf("input file reading error occured\n");
        return 0;
    }

    CfObject object;
    CfAssemblyDetails assemblyDetails;
    CfAssemblyStatus assemblyStatus = cfAssemble(
        (CfStr){text, text + textLen},
        CF_STR(options.inputFileName),
        &object,
        &assemblyDetails
    );

    if (assemblyStatus != CF_ASSEMBLY_STATUS_OK) {
        printf("assembling failed.\n");
        cfAssemblyDetailsWrite(stdout, assemblyStatus, &assemblyDetails);
        printf("\n");
        free(text);
        return 0;
    }

    if (options.linkOutput) {
        CfExecutable executable;
        CfLinkDetails linkDetails;
        CfLinkStatus linkStatus = cfLink(&object, 1, &executable, &linkDetails);

        if (linkStatus != CF_LINK_STATUS_OK) {
            printf("linking failed.\n");
            cfLinkDetailsWrite(stdout, linkStatus, &linkDetails);
            cfObjectDtor(&object);
            free(text);
            return 0;
        }

        FILE *output = fopen(options.outputFileName, "wb");
        if (output == NULL) {
            printf("output file opening error: %s\n", strerror(errno));
            return 0;
        }

        if (!cfExecutableWrite(output, &executable))
            printf("executable writing failed.\n");

        fclose(output);
        cfExecutableDtor(&executable);
    } else {
        FILE *output = fopen(options.outputFileName, "wb");
        if (output == NULL) {
            printf("output file opening error: %s\n", strerror(errno));
            return 0;
        }

        if (!cfObjectWrite(output, &object))
            printf("object writing failed.\n");

        fclose(output);
    }

    free(text);
    cfObjectDtor(&object);

    return 0;
} // main

// main.cpp
