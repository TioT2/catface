/**
 * @brief linker utility implementation file
 */

#include <string.h>
#include <errno.h>

#include <cf_linker.h>
#include <cf_darr.h>

/**
 * @brief help printing function
 */
void printHelp( void ) {
    puts(
        "Usage:  cf_assembler [options] input1, input2, ... inputN\n"
        "\n"
        "Options:\n"
        "    -h              Display this message\n"
        "    -o <filename>   Write output to <filename>\n"
    );
} // printHelp

/**
 * @brief main program function
 */
int main( const int _argc, const char **_argv ) {
    // const int argc = 5;
    // const char *argv[] = {
    //     "qq",
    //     "-o",
    //     "examples/sqrt_repl/out.cfexe",
    //     "examples/sqrt_repl/math.cfobj",
    //     "examples/sqrt_repl/main.cfobj",
    // };
    const int argc = _argc;
    const char **argv = _argv;

    struct {
        bool printHelp;
        const char *outFileName;
    } options = {
        .printHelp = false,
        .outFileName = "out.cfexe",
    };

    size_t argIndex;
    for (argIndex = 1; argIndex < argc; argIndex++) {
        if (0 == strcmp(argv[argIndex], "-h")) {
            options.printHelp = true;
            continue;
        }

        if (0 == strcmp(argv[argIndex], "-o")) {
            if (argIndex + 1 >= argc) {
                printf("at least one argument for \"-o\" option required.");
                return 0;
            }
            options.outFileName = argv[argIndex + 1];
            argIndex++;
            continue;
        }

        break;
    }

    if (options.printHelp)
        printHelp();

    CfDarr objectArray = cfDarrCtor(sizeof(CfObject));

    if (objectArray == NULL) {
        printf("internal linker occured occured.\n");
        return 0;
    }

    bool isOk = true;

    // treat all another arguments as input file names
    for (;argIndex < argc; argIndex++) {
        CfObject object;

        FILE *file = fopen(argv[argIndex], "rb");
        if (file == NULL) {
            printf("\"%s\" input file opening error: %s\n", argv[argIndex], strerror(errno));
            isOk = false;
            break;
        }
        CfObjectReadStatus readStatus = cfObjectRead(file, &object);

        if (CF_OBJECT_READ_STATUS_OK != readStatus) {
            printf("object from file reading error: %s\n", cfObjectReadStatusStr(readStatus));
            isOk = false;
            break;
        }
        fclose(file);

        if (CF_DARR_OK != cfDarrPush(&objectArray, &object)) {
            printf("linker internal error occured.\n");
            isOk = false;
            break;
        }
    }

    while (isOk) {
        // a bit of crutches))
        CfExecutable executable;
        CfLinkDetails details;
        CfLinkStatus status = cfLink(
            (CfObject *)cfDarrData(objectArray),
            cfDarrLength(objectArray),
            &executable,
            &details
        );

        if (status != CF_LINK_STATUS_OK) {
            cfLinkDetailsWrite(stdout, status, &details);
            isOk = false;
            break;
        }

        FILE *file = fopen(options.outFileName, "wb");
        if (file == NULL) {
            printf("output file opening error: %s\n", strerror(errno));
            cfExecutableDtor(&executable);
            isOk = false;
            break;
        }

        if (!cfExecutableWrite(file, &executable))
            printf("executable write error occured.\n");
        fclose(file);

        cfExecutableDtor(&executable);
        break;
    }


    CfObject *objects = (CfObject *)cfDarrData(objectArray);
    for (size_t i = 0, n = cfDarrLength(objectArray); i < n; i++)
        cfObjectDtor(&objects[i]);

    cfDarrDtor(objectArray);

    return 0;
} // main

// main.c
