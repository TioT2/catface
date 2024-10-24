/**
 * @brief sandbox implementation file
 */

#include <assert.h>

#include <cf_vm.h>

#include "sandbox.h"

static void sandboxWriteCharacter(
    uint32_t *dst,
    const size_t dstPitch,
    uint64_t letter,
    const uint32_t foregroundColor,
    const uint32_t backgroundColor
) {
    uint8_t ly = 8;
    uint8_t lx;

    while (ly--) {
        lx = 8;
        while (lx--) {
            *dst++ = letter & 0x80 ? ~0 : 0;
            letter <<= 1;
        }
        letter >>= 16;
        dst = (uint32_t *)((uint8_t *)dst + dstPitch - CF_VIDEO_FONT_WIDTH * sizeof(uint32_t));
    }
}

/**
 * @brief sandbox SDL therad function
 * 
 * @param userContext user-provided context pointer. points to SandboxContext
 * 
 * @return exit status, at least now value is ignored.
 */
static int SDLCALL sandboxThreadFn( void *userContext ) {
    SandboxContext *context = (SandboxContext *)userContext;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window *window = SDL_CreateWindow("CATFACE", 30, 30, 640, 400, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (window == NULL) {
        // set terminate flag to true
        SDL_AtomicSet(&context->isTerminated, true);
        SDL_Quit();
        return 0;
    }

    // create surface for custom (such as text or palette-based) rendering
    SDL_Surface *targetSurface = SDL_CreateRGBSurface(0, 320, 200, 32, 0, 0, 0, 0);

    if (targetSurface == NULL) {
        SDL_AtomicSet(&context->isTerminated, true);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    SDL_Surface *trueColorSurface = SDL_CreateRGBSurfaceWithFormatFrom(
        context->memory,
        CF_VIDEO_SCREEN_WIDTH,
        CF_VIDEO_SCREEN_HEIGHT,
        32,
        CF_VIDEO_SCREEN_WIDTH * 4,
        SDL_PIXELFORMAT_RGBX32
    );

    SDL_Surface *paletteSurface = SDL_CreateRGBSurfaceWithFormatFrom(
        context->memory,
        CF_VIDEO_SCREEN_WIDTH,
        CF_VIDEO_SCREEN_HEIGHT,
        8,
        CF_VIDEO_SCREEN_WIDTH,
        SDL_PIXELFORMAT_INDEX8
    );
    SDL_SetPaletteColors(paletteSurface->format->palette, (SDL_Color *)((CfVideoMemory *)context->memory)->colorPalette.palette, 0, 256);

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

            SDL_Surface *blitSurface = NULL;

            // remove update request
            SDL_AtomicSet(&context->manualUpdateRequested, false);

            // perform different actions for different storage formats
            switch (storageFormat) {
            case CF_VIDEO_STORAGE_FORMAT_TEXT: {
                if (0 != SDL_LockSurface(targetSurface))
                    break;
                uint8_t *const pixels = (uint8_t *)targetSurface->pixels;
                const uint8_t *const memory = (const uint8_t *)context->memory;
                const size_t pitch = targetSurface->pitch;

                for (size_t y = 0; y < CF_VIDEO_TEXT_HEIGHT; y++) {
                    uint32_t *lineStart = (uint32_t *)(pixels + y * pitch * CF_VIDEO_FONT_HEIGHT);

                    for (size_t x = 0; x < CF_VIDEO_TEXT_WIDTH; x++)
                        sandboxWriteCharacter(
                            lineStart + x * CF_VIDEO_FONT_WIDTH,
                            pitch,
                            context->font[memory[y * CF_VIDEO_TEXT_WIDTH + x]],
                            ~0,
                            0
                        );
                }

                SDL_UnlockSurface(targetSurface);
                blitSurface = targetSurface;
                break;
            }

            case CF_VIDEO_STORAGE_FORMAT_COLORED_TEXT: {
                if (0 != SDL_LockSurface(targetSurface))
                    break;
                uint8_t *const pixels = (uint8_t *)targetSurface->pixels;
                const CfColoredCharacter *const memory = (const CfColoredCharacter *)context->memory;
                const size_t pitch = targetSurface->pitch;

                const uint32_t *foregroundPalette = ((const CfVideoMemory *)context->memory)->foregroundPalette;
                const uint32_t *backgroundPalette = ((const CfVideoMemory *)context->memory)->backgroundPalette;

                for (size_t y = 0; y < CF_VIDEO_TEXT_HEIGHT; y++) {
                    uint32_t *lineStart = (uint32_t *)(pixels + y * pitch * CF_VIDEO_FONT_HEIGHT);

                    for (size_t x = 0; x < CF_VIDEO_TEXT_WIDTH; x++) {
                        const CfColoredCharacter ch = memory[y * CF_VIDEO_TEXT_WIDTH + x];

                        sandboxWriteCharacter(
                            lineStart + x * CF_VIDEO_FONT_WIDTH,
                            pitch,
                            context->font[ch.character],
                            foregroundPalette[ch.foregroundColor],
                            backgroundPalette[ch.backgroundColor]
                        );
                    }
                }

                SDL_UnlockSurface(targetSurface);
                blitSurface = targetSurface;
                break; // not supported yet
            }
            case CF_VIDEO_STORAGE_FORMAT_COLOR_PALETTE: {
                const SDL_Rect src = {0, 0, CF_VIDEO_SCREEN_WIDTH, CF_VIDEO_SCREEN_HEIGHT};
                SDL_Rect dst = src;

                SDL_BlitSurface(paletteSurface, &src, targetSurface, &dst);
                blitSurface = targetSurface;
                break; // not supported yet
            }

            // just cop
            case CF_VIDEO_STORAGE_FORMAT_TRUE_COLOR: {
                blitSurface = trueColorSurface;
                break;
            }
            }

            SDL_Surface *windowSurface = SDL_GetWindowSurface(window);
            // yay code nesting
            if (windowSurface != NULL) {
                if (blitSurface != NULL) {
                    const SDL_Rect srcRect = { 0, 0, CF_VIDEO_SCREEN_WIDTH, CF_VIDEO_SCREEN_HEIGHT };
                    SDL_Rect dstRect = { 0, 0, windowSurface->w, windowSurface->h };
                    SDL_BlitScaled(blitSurface, &srcRect, windowSurface, &dstRect);
                }
                SDL_UpdateWindowSurface(window);
            }
        }
    }

    SDL_FreeSurface(targetSurface);

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
 * 
 * @note matches prototype of 'CfSandbox::getExecutionTime' function pointer
 */
static bool sandboxGetExecutionTime( void *userContext, float *dst ) {
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
 * 
 * @note matches prototype of 'CfSandbox::initialize' function pointer
 */
static bool sandboxInitialize( void *userContext, const CfExecContext *execContext ) {
    SandboxContext *context = (SandboxContext *)userContext;

    // definetely not sh*tcode
    static uint8_t font[2048] = {
        #include "sandbox_font.inc"
    };

    memcpy(context->font, font, sizeof(context->font));

    context->initialPerformanceCounter = SDL_GetPerformanceCounter();
    context->performanceFrequency = SDL_GetPerformanceFrequency();

    // initialize VM -> SDL variables

    SDL_AtomicSet(&context->pixelStorageFormat, CF_VIDEO_STORAGE_FORMAT_TEXT);
    SDL_AtomicSet(&context->alwaysUpdate, true);
    SDL_AtomicSet(&context->manualUpdateRequested, false);
    SDL_AtomicSet(&context->shouldTerminate, false);

    // initialize SDL -> VM variables
    SDL_AtomicSet(&context->isTerminated, false);

    // initialize sandbox memory
    context->memory = execContext->memory;
    context->memorySize = execContext->memorySize;

    // try to initialize
    context->sandboxThread = SDL_CreateThread(sandboxThreadFn, "Sandbox SDL Thread", context);
    if (context->sandboxThread == NULL)
        return false;

    return true;
} // sandboxInitialize

/**
 * @brief vm panic handling function
 * 
 * @param context   sandbox user context
 * @param termInfo describes occured panic (non-null)
 * 
 * @note matches prototype of 'CfSandbox::terminate' function pointer
 */
static void sandboxTerminate( void *userContext, const CfTermInfo *termInfo ) {
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
 * 
 * @note matches prototype of 'CfSandbox::refreshScreen' function pointer
 */
static bool sandboxRefreshScreen( void *userContext ) {

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
 * 
 * @note matches prototype of 'CfSandbox::setVideoMode' function pointer
 */
static bool sandboxSetVideoMode(
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
} // sandboxSetVideoMode

/**
 * @brief int64 from stdin reading function
 * 
 * @param context context that user can send to internal functions from sandbox, in this case value ignore
 * 
 * @return parsed int64 if succeeded and -1 otherwise. (This function is debug-only and will be eliminated after keyboard supoprt adding)
 */
static double readFloat64( void *context ) {
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
static void writeFloat64( void *context, double number ) {
    printf("%lf\n", number);
} // writeFloat64

void sandboxConfigure( CfSandbox *vmSandbox, SandboxContext *context ) {
    vmSandbox->userContext = context;

    vmSandbox->initialize = sandboxInitialize;
    vmSandbox->terminate = sandboxTerminate;

    vmSandbox->setVideoMode = sandboxSetVideoMode;
    vmSandbox->refreshScreen = sandboxRefreshScreen;
    vmSandbox->getExecutionTime = sandboxGetExecutionTime;

    vmSandbox->readFloat64 = readFloat64;
    vmSandbox->writeFloat64 = writeFloat64;
} // sandboxConfigure

// sandbox.c
