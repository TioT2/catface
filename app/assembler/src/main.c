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
 * @brief text from file reading function // TODO: _READ_ (verb) _TEXT_ (noun) from file, PLEASE
 *
 * @param[in]  file   file to read pointer // TODO: (pointer to file to read from)
 * @param[out] dst    reading destination, function will allocate space for the result (non-null) (TODO: add that it need to be deallocated with free)
 * @param[out] dstLen read line text (non-null) (TODO: what symbols, is NUL-character included?)
 *
 * @return true if succeeded, false otherwise // TODO: what happened if it failed, why?
 */
// TODO: here?
bool readFile( FILE *file, char **dst, size_t *dstLen ) { // TODO: static
    assert(dst != NULL);
    assert(dstLen != NULL);

    fseek(file, 0, SEEK_END); // TODO: extract to a function?
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)calloc(len + 1, 1);
    if (buffer == NULL)
        return false;

    fread(buffer, 1, len, file);
    // FREAD RETURNS ERROR IN ERRNO, DON'T CLOBBER IT

    *dst = buffer;
    if (dstLen != NULL) // TODO: non-null you say?
        *dstLen = len;

    return true;
} // readFile

/**
 * @brief help printing function // TODO: it's all the same, fix
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

int main( const int _argc, const char **_argv ) { // TODO: why use underscores before the names?
    // const int argc = 5;
    // const char *argv[] = {
    //     "qq",
    //     "-o",
    //     "examples/sin.cfexe",
    //     "-l",
    //     "examples/sin.cfasm", // TODO: make it easier to debug without editing main itself
    // };                        //       also use stashes for such additions and review your
    //                           //       patches before you commit them.
    const int argc = _argc;
    const char **argv = _argv;

    // TODO: can you move all stuff that is less important elsewhere?
    // quite strange solution, but it's ok because last argument is treated as input file name.
    if (argc < 2 || 0 == strcmp(argv[1], "-h")) { // TODO: 0 == FOO() why? Why not FOO() == 0 like a normal person
        printHelp();
        return 0;
    }

    //   TODO: imagine «cat» program
    //
    //   * What would you prefer?
    //
    //   ** This?
    //   +----+------------------------------------------------------------------+
    //   | 1  | int main() {                                                     |
    //   | 2  |     if (argv[0] blah blah) {                                     |
    //   | 3  |         blah blah                                                |
    //   | 4  |         if (argv[0][0] whatever is like argv[0][1] blah) {       |
    //   | 5  |             blah blah                                            |
    //   | 6  |             blah blah                                            |
    //   | 7  |         }                                                        |
    //   | 8  |     }                                                            |
    //   | 9  |     // ... 2000 YEARS LATER ...                                  |
    //   | 10 |     file = read_file()              /// <= most important logic  |
    //   | 11 |     // ... 2000 YEARS LATER ...                                  |
    //   | 12 |     output_file(file)               /// <= most important logic  |
    //   | 13 |     // ... 2000 YEARS LATER ...                                  |
    //   | 14 |     free(buffer0);                                               |
    //   | 15 |     free(buffer1);                                               |
    //   | 16 |     extra_shutting_down_logic();                                 |
    //   | 17 |     if (process blah blah blah) {                                |
    //   | 18 |         delete blah blah                                         |
    //   | 19 |         if (argv[0][0] whatever is like argv[0][1] blah) {       |
    //   | 20 |             perform blah blah                                    |
    //   | 21 |             blah blah                                            |
    //   | 22 |         }                                                        |
    //   | 23 |     }                                                            |
    //   | 24 | }                                                                |
    //   +----+------------------------------------------------------------------+
    //
    //   ** Or this?
    //   +----+------------------------------------------------------------------+
    //   | 1  | int main() {                                                     |
    //   | 2  |     extracted_out_bullshit();                                    |
    //   | 3  |     file = read_file()              /// <= most important logic  |
    //   | 4  |     extracted_out_bullshit();                                    |
    //   | 5  |     output_file(file)               /// <= most important logic  |
    //   | 6  |     extracted_out_bullshit();                                    |
    //   +----+------------------------------------------------------------------+

    // it's obviously NOT overengineering
    // TODO: it's possible, just not this way :)
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

    struct { // TODO: Huh?
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
        // TODO: perror?
        printf("input file opening error: %s\n", strerror(errno));
        return 0;
    }
    bool readSuccess = readFile(input, &text, &textLen);
    // TODO: errno?
    fclose(input);


    // TODO: I will just say this: refactor this mess, make it bearable.

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
