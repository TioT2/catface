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

#include <sandbox.h>

/**
 * @brief help displaying function
 */
void printHelp( void ) {
    printf("Usage: cf_exec executable\n");
} // printHelp

int main( const int _argc, const char **_argv ) {
    // const int argc = 2;
    // const char *argv[] = { "qq", "examples/ray_tracer/out/out.cfexe" };
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
    CfSandbox sandbox = {0};
    sandboxConfigure(&sandbox, &context);

    const CfExecuteInfo execInfo = {
        .executable = &executable,
        .sandbox    = &sandbox,
        .ramSize    = (1 << 24),   // 16MB
    };

    if (!cfExecute(&execInfo))
        printf("sandbox error occured.\n");

    cfExecutableDtor(&executable);

    return 0;
} // main

// main.cpp
