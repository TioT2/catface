/**
 * @brief CFEXE executor utility.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <cf_vm.h>
#include <cf_executable.h>
#include <cf_cli.h>

#include "sandbox.h"

/**
 * @brief help displaying function
 */
void printHelp( void ) {
    printf("Usage: cf_exec executable\n");
} // printHelp

/**
 * @brief int64 from stdin reading function
 * 
 * @param context context that user can send to internal functions from sandbox, in this case value ignore
 * 
 * @return parsed int64 if succeeded and -1 otherwise. (This function is debug-only and will be eliminated after keyboard supoprt adding)
 */
double readFloat64( void *context ) {
    double num;
    if (scanf("%lf", &num) != 1)
        return -1;
    return num;
} // readFloat64

/**
 * @brief int64 to stdin writing function
 * 
 * @param context sandbox user context
 * @param number  number to write
 */
void writeFloat64( void *context, double number ) {
    printf("%lf\n", number);
} // writeFloat64

int main( const int _argc, const char **_argv ) {
    // const int argc = 2;
    // const char *argv[] = { "qq", "examples/factorial.cfexe" };
    const int argc = _argc;
    const char **argv = _argv;

    if (argc < 2) {
        printHelp();
        return 0;
    }

    const char *execPath = argv[1];

    CfExecutable executable;
    FILE *inputFile = fopen(execPath, "rb");

    if (inputFile == NULL) {
        printf("input file opening error: %s\n", strerror(errno));
        return 0;
    }

    CfExecutableReadStatus readStatus = cfExecutableRead(inputFile, &executable);
    fclose(inputFile);

    if (readStatus != CF_EXECUTABLE_READ_STATUS_OK) {
        printf("input executable file reading error: %s\n", cfExecutableReadStatusStr(readStatus));
        return 0;
    }

    SandboxContext context = {0};
    CfSandbox sandbox = {
        .userContext = &context,

        // initialization/termination
        .initialize = sandboxInitialize,
        .terminate = sandboxTerminate,

        // video
        .refreshScreen = sandboxRefreshScreen,
        .setVideoMode = sandboxSetVideoMode,

        // time
        .getExecutionTime = sandboxGetExecutionTime,

        // outdated sh*t
        .readFloat64 = readFloat64,
        .writeFloat64 = writeFloat64,
    };

    cfExecute(&executable, &sandbox);
    cfExecutableDtor(&executable);

    return 0;
} // main

// main.cpp
