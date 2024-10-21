/**
 * @brief CFEXE executor utility.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include <SDL2/SDL.h>

#include <cf_vm.h>
#include <cf_executable.h>
#include <cf_cli.h>

/**
 * @brief help displaying function
 */
void printHelp( void ) {
    printf("Usage: cf_exec executable\n");
} // printHelp

/// @brief font representation structure
typedef struct __SandboxFont {
    uint64_t letters[256]; ///< font 8x8 letters
} SandboxFont;

/// @brief sandbox context representation structure
typedef struct __SandboxContext {
    SandboxFont * font;                 ///< font

    SDL_Thread * sandboxThread;         ///< sandbox thread pointer

    // SDL -> VM
    SDL_atomic_t isTerminated;          ///< true if SDL thread terminated, FALSE otherwise

    // VM -> SDL
    SDL_atomic_t shouldTerminate;       ///< 
    SDL_atomic_t alwaysUpdate;          ///< 
    SDL_atomic_t manualUpdateRequested; ///< 
    SDL_atomic_t pixelStorageFormat;    ///< 

    uint64_t performanceFrequency;      ///< SDL timer frequency
    uint64_t initialPerformanceCounter; ///< initial time

    void *memory;                       ///< 
} SandboxContext;

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
 * @brief sandbox SDL therad function
 * 
 * @param userContext user-provided context pointer. points to SandboxContext
 * 
 * @return exit status, at least now value is ignored.
 */
int SDLCALL sandboxThreadFn( void *userContext ) {
    SandboxContext *context = (SandboxContext *)userContext;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window *window = SDL_CreateWindow("CATFACE",
        30,
        30,
        320,
        200,
        0
    );


    if (window == NULL) {
        // set terminate flag to true
        SDL_AtomicSet(&context->isTerminated, true);
        return 0;
    }

    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);

    if (windowSurface == NULL) {
        SDL_DestroyWindow(window);
        SDL_AtomicSet(&context->isTerminated, true);
        return 0;
    }

    SDL_Surface *trueColorSurface = SDL_CreateRGBSurfaceWithFormatFrom(
        context->memory,
        CF_VIDEO_SCREEN_WIDTH,
        CF_VIDEO_SCREEN_HEIGHT,
        1,
        CF_VIDEO_SCREEN_WIDTH * 4,
        SDL_PIXELFORMAT_RGBX32
    );


    bool continueExecution = true;
    while (continueExecution && !SDL_AtomicGet(&context->shouldTerminate)) {
        SDL_Event event = {0};

        // initialize SDL -> VM
        SDL_AtomicSet(&context->isTerminated, false);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                continueExecution = false;
                break;
            }
            }
        }

        if (SDL_AtomicGet(&context->alwaysUpdate) || SDL_AtomicGet(&context->manualUpdateRequested)) {
            const CfVideoStorageFormat storageFormat = (CfVideoStorageFormat)SDL_AtomicGet(&context->pixelStorageFormat);

            // remove update request
            SDL_AtomicSet(&context->manualUpdateRequested, false);

            // perform different actions for different storage formats
            switch (storageFormat) {
            case CF_VIDEO_STORAGE_FORMAT_TEXT: {
                if (0 != SDL_LockSurface(windowSurface))
                    break;
                uint8_t *const pixels = (uint8_t *)windowSurface->pixels;
                const uint8_t *const memory = (const uint8_t *)context->memory;
                const size_t pitch = windowSurface->pitch;
                const uint64_t *fontLetters = context->font->letters;

                for (size_t y = 0; y < CF_VIDEO_TEXT_HEIGHT; y++) {
                    uint32_t *lineStart = (uint32_t *)(pixels + y * pitch * CF_VIDEO_FONT_HEIGHT);

                    for (size_t x = 0; x < CF_VIDEO_TEXT_WIDTH; x++) {
                        uint64_t fontLetter = fontLetters[memory[y * CF_VIDEO_TEXT_WIDTH + x]];
                        uint32_t *pixel = lineStart + x * CF_VIDEO_FONT_WIDTH;
                        uint8_t ly = 8;
                        uint8_t lx;

                        while (ly--) {
                            lx = 8;
                            while (lx--) {
                                *pixel++ = fontLetter & 0x80 ? ~0 : 0;
                                fontLetter <<= 1;
                            }
                            fontLetter >>= 16;
                            pixel = (uint32_t *)((uint8_t *)pixel + pitch - CF_VIDEO_FONT_WIDTH * sizeof(uint32_t));
                        }
                    }
                }

                SDL_UnlockSurface(windowSurface);
                break;
            }

            case CF_VIDEO_STORAGE_FORMAT_COLORED_TEXT: {
                break; // not supported yet
            }
            case CF_VIDEO_STORAGE_FORMAT_COLOR_PALETTE:
                break; // not supported yet
            case CF_VIDEO_STORAGE_FORMAT_TRUE_COLOR: {
                const SDL_Rect srcRect = {
                    0,
                    0,
                    (int)CF_VIDEO_SCREEN_WIDTH,
                    (int)CF_VIDEO_SCREEN_HEIGHT,
                };
                SDL_Rect dstRect = srcRect;

                SDL_BlitSurface(trueColorSurface, &srcRect, windowSurface, &dstRect);
                break;
            }
            }

            SDL_UpdateWindowSurface(window);
        }
        // update screen
    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    SDL_AtomicSet(&context->isTerminated, true);
    return 0;
} // sandboxThreadFn

/**
 * @brief program execution time (in seconds) getting function
 * 
 * @param[in]  userContext user-provided context
 * @param[out] dst         time destination
 * 
 * @return true if succeeded, false if something went wrong.
 */
bool sandboxGetExecutionTime( void *userContext, float *dst ) {
    SandboxContext *context = (SandboxContext *)userContext;
    if (SDL_AtomicGet(&context->isTerminated))
        return false;

    const uint64_t now = SDL_GetPerformanceCounter();

    *dst = (now - context->initialPerformanceCounter) / (float)context->performanceFrequency;
    return true;
} // sandboxGetExecutionTime

/**
 * @brief sandbox initialization function
 * 
 * @param[in] context     user context
 * @param[in] execContext execution context
 */
bool sandboxInitialize( void *userContext, const CfExecContext *execContext ) {
    SandboxContext *context = (SandboxContext *)userContext;

    // definetly not sh*tcode
    static uint8_t font[2048] = {
        #include "font.inc"
    };

    context->font = (SandboxFont *)font;
    context->initialPerformanceCounter = SDL_GetPerformanceCounter();
    context->performanceFrequency = SDL_GetPerformanceFrequency();

    // initialize VM -> SDL

    SDL_AtomicSet(&context->pixelStorageFormat, CF_VIDEO_STORAGE_FORMAT_TEXT);
    SDL_AtomicSet(&context->alwaysUpdate, true);
    SDL_AtomicSet(&context->manualUpdateRequested, false);
    SDL_AtomicSet(&context->shouldTerminate, false);

    // initialize SDL -> VM
    SDL_AtomicSet(&context->isTerminated, false);

    context->sandboxThread = SDL_CreateThread(sandboxThreadFn, "Sandbox SDL Thread", context);

    if (context->sandboxThread == NULL)
        return false;
    context->memory = execContext->memory;
    return true;
} // sandboxInitialize

/**
 * @brief vm panic handling function
 * 
 * @param context   sandbox user context
 * @param termInfo describes occured panic (non-null)
 */
void sandboxTerminate( void *userContext, const CfTermInfo *termInfo ) {
    SandboxContext *context = (SandboxContext *)userContext;

    assert(termInfo != NULL);

    SDL_AtomicSet(&context->shouldTerminate, true);
    int sandboxThreadStatus = 0;
    SDL_WaitThread(context->sandboxThread, &sandboxThreadStatus);

    if (termInfo->reason == CF_TERM_REASON_HALT) {
        printf("program finished.");
        return;
    }

    if (termInfo->reason == CF_TERM_REASON_SANDBOX_ERROR) {
        printf("execution stopped because sandbox told to.");
        return;
    }

    printf("program panicked (by %zu offset). reason: ", termInfo->offset);
    switch (termInfo->reason) {
    case CF_TERM_REASON_HALT: break;
    case CF_TERM_REASON_SANDBOX_ERROR: break;

    case CF_TERM_REASON_INVALID_POP_INFO: {
        printf("invalid pop info (doReadImmediate: %d, isMemoryAccess: %d, registerIndex: %d)",
            (int)termInfo->invalidPopInfo.doReadImmediate,
            (int)termInfo->invalidPopInfo.isMemoryAccess,
            (int)termInfo->invalidPopInfo.registerIndex
        );
        break;
    }

    case CF_TERM_REASON_SEGMENTATION_FAULT: {
        printf("segmentation fault (addr: %08X, memory size: %08X).",
            termInfo->segmentationFault.addr,
            termInfo->segmentationFault.memorySize
        );
        break;
    }

    case CF_TERM_REASON_INVALID_VIDEO_MODE: {
        printf("invalid videoMode bits combination (storage format: %X, update mode: %X).",
            termInfo->invalidVideoMode.storageFormatBits,
            termInfo->invalidVideoMode.updateModeBits
        );
        break;
    }

    case CF_TERM_REASON_INVALID_IC: {
        printf("instruction counter invalidated.");
        break;
    }

    case CF_TERM_REASON_UNKNOWN_OPCODE      : {
        printf("unknown opcode (%04X).", (int)termInfo->unknownOpcode);
        break;
    }
    case CF_TERM_REASON_CALL_STACK_UNDERFLOW     : {
        printf("call stack underflow.");
        break;
    }
    case CF_TERM_REASON_UNREACHABLE         : {
        printf("unreachable executed.");
        break;
    }
    case CF_TERM_REASON_INTERNAL_ERROR      : {
        printf("internal vm error occured.");
        break;
    }
    case CF_TERM_REASON_NO_OPERANDS         : {
        printf("no operands on stack.");
        break;
    }
    case CF_TERM_REASON_UNKNOWN_SYSTEM_CALL : {
        printf("unknown system call (%u).", termInfo->unknownSystemCall);
        break;
    }
    case CF_TERM_REASON_UNEXPECTED_CODE_END : {
        printf("unexpected code end.");
        break;
    }
    case CF_TERM_REASON_UNKNOWN_REGISTER    : {
        printf("unknown register: %d.", termInfo->unknownRegister);
        break;
    }
    case CF_TERM_REASON_STACK_UNDERFLOW     : {
        printf("operand stack underflow.");
        break;
    }
    }
} // sandboxTerminate

/**
 * @brief screen update function
 * 
 * @param[in] userContext user context reference
 * 
 * @return true if succeeded, false otherwise.
 */
bool sandboxRefreshScreen( void *userContext ) {

    SandboxContext *context = (SandboxContext *)userContext;

    if (SDL_AtomicGet(&context->isTerminated))
        return false;

    SDL_AtomicSet(&context->manualUpdateRequested, true);
    return true;
} // sandboxRefreshScreen

/**
 * @brief video mode setting function
 * 
 * @param userContext   sandbox context pointer
 * @param storageFormat pixel storage format
 * @param updateMode    screen update mode
 */
bool sandboxSetVideoMode(
    void                       *userContext,
    const CfVideoStorageFormat  storageFormat,
    const CfVideoUpdateMode     updateMode
) {
    SandboxContext *context = (SandboxContext *)userContext;

    // check if SDL thread is running
    if (SDL_AtomicGet(&context->isTerminated))
        return false;

    bool alwaysUpdate = false;

    switch (updateMode) {
    case CF_VIDEO_UPDATE_MODE_IMMEDIATE: alwaysUpdate = true;  break;
    case CF_VIDEO_UPDATE_MODE_MANUAL:    alwaysUpdate = false; break;
    }

    SDL_AtomicSet(&context->pixelStorageFormat, storageFormat);
    SDL_AtomicSet(&context->alwaysUpdate, alwaysUpdate);

    return true;
}

int main( const int _argc, const char **_argv ) {
    // const int argc = 2;
    // const char *argv[] = { "qq", "examples/sin.cfexe" };
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
