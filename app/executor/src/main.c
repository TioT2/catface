/**
 * @brief CF module executor utility.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <cf_vm.h>
#include <cf_module.h>
#include <cf_cli.h>

/**
 * @brief help displaying function
 */
void printHelp( void ) {
    printf("Usage: cf_exec module\n");
} // printHelp

/**
 * @brief int64 from stdin reading function
 * 
 * @param context context that user can send to internal functions from sandbox, in this case value ignore
 * 
 * @return parsed int64 if succeeded and -1 otherwise
 * (there is only one reason for this - user functions cannot propagate panic yet).
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

/**
 * @brief vm panic handling function
 * 
 * @param context   sandbox user context
 * @param panicInfo describes occured panic (non-null)
 */
void handlePanic( void *context, const CfPanicInfo *panicInfo ) {
    assert(panicInfo != NULL);

    printf("module panicked (by %zu offset). reason: ", panicInfo->offset);
    switch (panicInfo->reason) {
    case CF_PANIC_REASON_UNKNOWN_OPCODE      : {
        printf("unknown opcode (%04X).", (int)panicInfo->unknownOpcode.opcode);
        break;
    }
    case CF_PANIC_REASON_CALL_STACK_UNDERFLOW     : {
        printf("call stack underflow.");
        break;
    }
    case CF_PANIC_REASON_UNREACHABLE         : {
        printf("unreachable executed.");
        break;
    }
    case CF_PANIC_REASON_INTERNAL_ERROR      : {
        printf("internal vm error occured.");
        break;
    }
    case CF_PANIC_REASON_NO_OPERANDS         : {
        printf("no operands on stack.");
        break;
    }
    case CF_PANIC_REASON_UNKNOWN_SYSTEM_CALL : {
        printf("unknown system call (%u).", panicInfo->unknownSystemCall.index);
        break;
    }
    case CF_PANIC_REASON_UNEXPECTED_CODE_END : {
        printf("unexpected code end.");
        break;
    }
    case CF_PANIC_REASON_UNKNOWN_REGISTER    : {
        printf("unknown register: %d.", panicInfo->unknownRegister.index);
        break;
    }
    case CF_PANIC_REASON_STACK_UNDERFLOW     : {
        printf("operand stack underflow.");
        break;
    }
    }
} // handlePanic

int main( const int _argc, const char **_argv ) {
    // const int argc = 2;
    // const char *argv[] = { "qq", "examples/sqrt_repl.cfmod" };
    const int argc = _argc;
    const char **argv = _argv;

    if (argc < 2) {
        printHelp();
        return 0;
    }

    const char *modulePath = argv[1];

    CfModule module;
    FILE *inputFile = fopen(modulePath, "rb");

    if (inputFile == NULL) {
        printf("input file opening error: %s\n", strerror(errno));
        return 0;
    }

    CfModuleReadStatus readStatus = cfModuleRead(inputFile, &module);
    fclose(inputFile);

    if (readStatus != CF_MODULE_READ_STATUS_OK) {
        printf("input module file reading error: %s\n", cfModuleReadStatusStr(readStatus));
        return 0;
    }

    CfSandbox sandbox = {
        .userContextPtr = &module,
        .readFloat64 = readFloat64,
        .writeFloat64 = writeFloat64,
        .handlePanic = handlePanic,
    };

    cfModuleExec(&module, &sandbox);
    cfModuleDtor(&module);
    printf("\n");

    return 0;
} // main

// main.cpp
