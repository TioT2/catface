/**
 * @brief compiler utility implementation file
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "compiler.h"

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

void printHelp( void ) {
    puts(
        "Usage: cf_compiler [options] [input file name]\n"
        "\n"
        "Options:\n"
        "    -h             Display help menu\n"
        "    -o <filename>  Write executable to certain file\n"
    );
} // printHelp

/**
 * @brief main program function
 * 
 * @param[in] argc input argument count
 * @param[in] argv argument values
 * 
 * @return exit status (0 on success)
 */
int main( int argc_, const char **argv_ ) {
    // int argc = argc_;
    // const char **argv = argv_;
    int argc = 4;
    const char *argv[] = {
        "amogus",
        "-o",
        "examples/lang/main.cfexe",
        "examples/lang/main.cf",
    };
    
    // arguments: cf_compiler -o main.cfexe -l main.cf example1.cf example2.cf

    struct {
        const char *outName;
    } options = {
        .outName = "out.cfexe",
    };

    int argumentIndex = 1;
    for (; argumentIndex < argc; argumentIndex++) {
        if (strcmp(argv[argumentIndex], "-o") == 0) {
            if (argumentIndex + 1 >= argc) {
                printf("Invalid flag: \"-o\" key must be followed with output file name\n");
                return 0;
            }
            options.outName = argv[++argumentIndex];
            continue;
        }

        if (argv[argumentIndex][0] == '-') {
            printf("Unknown flag: \"%s\"\n", argv[argumentIndex]);
            return 0;
        }

        break;
    }

    Compiler *compiler = compilerCtor();

    if (compiler == NULL) {
        printf("Internal error (cannot create compiler instance)\n");
        return 0;
    }

    // add input files
    for (; argumentIndex < argc; argumentIndex++) {
        const char *fileName = argv[argumentIndex];

        FILE *file = fopen(fileName, "rt");

        if (file == NULL) {
            printf("Cannot open file \"%s\" (%s)\n", fileName, strerror(errno));
            compilerDtor(compiler);
            return 0;
        }

        char *src = NULL;
        size_t srcLen = 0;
        if (!readFile(file, &src, &srcLen)) {
            printf("Internal error ()\n");
            compilerDtor(compiler);
            return 1;
        }

        fclose(file);

        CompilerAddCfFileResult result = compilerAddCfFile(compiler, argv[argumentIndex], src);
        free(src);

        if (result.status != COMPILER_ADD_CF_FILE_STATUS_OK) {
            // do fancy error output, there's no time for(
            printf("Compilation error occured.\n");
            compilerDtor(compiler);
            return 0;
        }
    }

    // build'em in executable
    CompilerBuildResult buildResult = compilerBuildExecutable(compiler);

    if (buildResult.status != COMPILER_BUILD_STATUS_OK) {
        printf("Building error occured.\n");
        compilerDtor(compiler);
        return 0;
    }
    compilerDtor(compiler);

    // write executable to output file

    CfExecutable executable = buildResult.ok;

    // write executable to output file
    FILE *outFile = fopen(options.outName, "wb");
    if (outFile == NULL) {
        cfExecutableDtor(&executable);
        printf("Can't open file \"%s\" for writing (%s)\n", options.outName, strerror(errno));
        return 0;
    }

    cfExecutableWrite(outFile, &executable);
    fclose(outFile);

    cfExecutableDtor(&executable);

    return 0;
} // main

// main.c
