/**
 * @brief sandbox implementation file
 */

#include <assert.h>

#include <cf_vm.h>

#include "sandbox.h"

/**
 * @brief character to framebuffer writing function
 * 
 * @param[out] dst             write destination
 * @param[in]  dstPitch        destination framebuffer pitch (in bytes)
 * @param[in]  letter          letter 'picture' (8x8 1 bit picture)
 * @param[in]  foregroundColor foreground color
 * @param[in]  backgroundColor background color
 */
static void sandboxWriteCharacter(
    uint32_t       * dst,
    const size_t     dstPitch,
    uint64_t         letter,
    const uint32_t   foregroundColor,
    const uint32_t   backgroundColor
) {
    uint8_t ly = 8;
    uint8_t lx;

    while (ly--) {
        lx = 8;
        while (lx--) {
            *dst++ = letter & 1 ? foregroundColor : backgroundColor;
            letter >>= 1;
        }

        // set new value to dst
        dst = (uint32_t *)((uint8_t *)dst + dstPitch - CF_VIDEO_FONT_WIDTH * sizeof(uint32_t));
    }
} // sandboxWriteCharacter

/**
 * @brief SDL scancode into CF key transformation function
 * 
 * @param[in]  scancode SDL scancode
 * 
 * @return CF_KEY_NULL if no corresponding key found, valid key if not.
 */
static CfKey sandboxKeyFromSdlScancode( const SDL_Scancode scancode ) {
    switch (scancode) {
    case SDL_SCANCODE_A            : return CF_KEY_A             ;
    case SDL_SCANCODE_B            : return CF_KEY_B             ;
    case SDL_SCANCODE_C            : return CF_KEY_C             ;
    case SDL_SCANCODE_D            : return CF_KEY_D             ;
    case SDL_SCANCODE_E            : return CF_KEY_E             ;
    case SDL_SCANCODE_F            : return CF_KEY_F             ;
    case SDL_SCANCODE_G            : return CF_KEY_G             ;
    case SDL_SCANCODE_H            : return CF_KEY_H             ;
    case SDL_SCANCODE_I            : return CF_KEY_I             ;
    case SDL_SCANCODE_J            : return CF_KEY_J             ;
    case SDL_SCANCODE_K            : return CF_KEY_K             ;
    case SDL_SCANCODE_L            : return CF_KEY_L             ;
    case SDL_SCANCODE_M            : return CF_KEY_M             ;
    case SDL_SCANCODE_N            : return CF_KEY_N             ;
    case SDL_SCANCODE_O            : return CF_KEY_O             ;
    case SDL_SCANCODE_P            : return CF_KEY_P             ;
    case SDL_SCANCODE_Q            : return CF_KEY_Q             ;
    case SDL_SCANCODE_R            : return CF_KEY_R             ;
    case SDL_SCANCODE_S            : return CF_KEY_S             ;
    case SDL_SCANCODE_T            : return CF_KEY_T             ;
    case SDL_SCANCODE_U            : return CF_KEY_U             ;
    case SDL_SCANCODE_V            : return CF_KEY_V             ;
    case SDL_SCANCODE_W            : return CF_KEY_W             ;
    case SDL_SCANCODE_X            : return CF_KEY_X             ;
    case SDL_SCANCODE_Y            : return CF_KEY_Y             ;
    case SDL_SCANCODE_Z            : return CF_KEY_Z             ;
    case SDL_SCANCODE_0            : return CF_KEY_0             ;
    case SDL_SCANCODE_1            : return CF_KEY_1             ;
    case SDL_SCANCODE_2            : return CF_KEY_2             ;
    case SDL_SCANCODE_3            : return CF_KEY_3             ;
    case SDL_SCANCODE_4            : return CF_KEY_4             ;
    case SDL_SCANCODE_5            : return CF_KEY_5             ;
    case SDL_SCANCODE_6            : return CF_KEY_6             ;
    case SDL_SCANCODE_7            : return CF_KEY_7             ;
    case SDL_SCANCODE_8            : return CF_KEY_8             ;
    case SDL_SCANCODE_9            : return CF_KEY_9             ;
    case SDL_SCANCODE_RETURN       : return CF_KEY_ENTER         ;
    case SDL_SCANCODE_BACKSPACE    : return CF_KEY_BACKSPACE     ;
    case SDL_SCANCODE_MINUS        : return CF_KEY_MINUS         ;
    case SDL_SCANCODE_EQUALS       : return CF_KEY_EQUAL         ;
    case SDL_SCANCODE_PERIOD       : return CF_KEY_DOT           ;
    case SDL_SCANCODE_COMMA        : return CF_KEY_COMMA         ;
    case SDL_SCANCODE_SLASH        : return CF_KEY_SLASH         ;
    case SDL_SCANCODE_BACKSLASH    : return CF_KEY_BACKSLASH     ;
    case SDL_SCANCODE_APOSTROPHE   : return CF_KEY_QUOTE         ;
    case SDL_SCANCODE_GRAVE        : return CF_KEY_BACKQUOTE     ;
    case SDL_SCANCODE_TAB          : return CF_KEY_TAB           ;
    case SDL_SCANCODE_LEFTBRACKET  : return CF_KEY_LEFT_BRACKET  ;
    case SDL_SCANCODE_RIGHTBRACKET : return CF_KEY_RIGHT_BRACKET ;
    case SDL_SCANCODE_SPACE        : return CF_KEY_SPACE         ;
    case SDL_SCANCODE_SEMICOLON    : return CF_KEY_SEMICOLON     ;
    case SDL_SCANCODE_UP           : return CF_KEY_UP            ;
    case SDL_SCANCODE_DOWN         : return CF_KEY_DOWN          ;
    case SDL_SCANCODE_LEFT         : return CF_KEY_LEFT          ;
    case SDL_SCANCODE_RIGHT        : return CF_KEY_RIGHT         ;
    case SDL_SCANCODE_LSHIFT       : return CF_KEY_SHIFT         ;
    case SDL_SCANCODE_LALT         : return CF_KEY_ALT           ;
    case SDL_SCANCODE_LCTRL        : return CF_KEY_CTRL          ;
    case SDL_SCANCODE_RSHIFT       : return CF_KEY_SHIFT         ;
    case SDL_SCANCODE_RALT         : return CF_KEY_ALT           ;
    case SDL_SCANCODE_RCTRL        : return CF_KEY_CTRL          ;
    case SDL_SCANCODE_ESCAPE       : return CF_KEY_ESCAPE        ;
    default                        : return CF_KEY_NULL          ;
    }
} // sandboxKeyFromSdlScancode

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

            case SDL_KEYUP: {
                SDL_Scancode scancode = event.key.keysym.scancode;
                const CfKey key = sandboxKeyFromSdlScancode(scancode);

                if (key != CF_KEY_NULL)
                    SDL_AtomicAdd(&context->keyStates[(size_t)key], -1);
                break;
            }

            case SDL_KEYDOWN: {
                SDL_Scancode scancode = event.key.keysym.scancode;
                const CfKey key = sandboxKeyFromSdlScancode(scancode);

                if (key != CF_KEY_NULL) {
                    SDL_AtomicAdd(&context->keyStates[(size_t)key], 1);

                    if (SDL_AtomicGet(&context->waitKeyRequired))
                        SDL_AtomicSet(&context->waitKeyValue, key);
                }
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

            // just copy
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
 * @brief key state getting function
 * 
 * @param[in,out] userContext user-provided context
 * @param[in]     key         key to get state of
 * @param[out]    dst         state destination (non-null) (true if pressed, false if not)
 * 
 * @return true if succeeded, false if something went wrong.
 */
bool sandboxGetKeyState( void *userContext, CfKey key, bool *dst ) {
    assert(dst != NULL);

    SandboxContext *context = (SandboxContext *)userContext;

    if ((uint32_t)key >= (uint32_t)_CF_KEY_MAX) // check array bounds
        return false;

    *dst = SDL_AtomicGet(&context->keyStates[(size_t)key]) > 0;

    return true; // unimplemented yet
} // sandboxGetKeyState

/**
 * @brief any key press waiting function
 * 
 * @param[in,out] userContext user-provided context
 * @param[out]    dst         key id destination (non-null)
 * 
 * @return true if succeeded, false if something went wrong.
 */
bool sandboxWaitKeyDown( void *userContext, CfKey *dst ) {
    // TODO(q): should I do main?
    assert(dst != NULL);

    SandboxContext *context = (SandboxContext *)userContext;

    SDL_AtomicSet(&context->waitKeyValue, CF_KEY_NULL);
    SDL_AtomicSet(&context->waitKeyRequired, true);

    CfKey key = CF_KEY_NULL;
    volatile int nopper = 0;

    // wait for value is set
    while (true
        && !SDL_AtomicGet(&context->isTerminated)
        && (key = (CfKey)SDL_AtomicGet(&context->waitKeyValue)) == CF_KEY_NULL
    )
        nopper = 42; // kind of nop

    *dst = key;

    // unsed waitKey requirement
    SDL_AtomicSet(&context->waitKeyRequired, false);

    return key != CF_KEY_NULL;
} // sandboxWaitKeyDown

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
    // reverse font bytes to improve usage comfortability
    for (size_t i = 0 ; i < 256; i++) {
        context->font[i] = ((context->font[i] & 0x5555555555555555) << 1) | ((context->font[i] & 0xAAAAAAAAAAAAAAAA) >> 1);
        context->font[i] = ((context->font[i] & 0x3333333333333333) << 2) | ((context->font[i] & 0xCCCCCCCCCCCCCCCC) >> 2);
        context->font[i] = ((context->font[i] & 0x0F0F0F0F0F0F0F0F) << 4) | ((context->font[i] & 0xF0F0F0F0F0F0F0F0) >> 4);
    }

    context->initialPerformanceCounter = SDL_GetPerformanceCounter();
    context->performanceFrequency = SDL_GetPerformanceFrequency();

    // initialize VM -> SDL variables

    SDL_AtomicSet(&context->pixelStorageFormat, CF_VIDEO_STORAGE_FORMAT_TEXT);
    SDL_AtomicSet(&context->alwaysUpdate, true);
    SDL_AtomicSet(&context->manualUpdateRequested, false);
    SDL_AtomicSet(&context->shouldTerminate, false);

    // initialize SDL -> VM variables
    SDL_AtomicSet(&context->isTerminated, false);

    // initialize keyStates
    for (uint32_t i = 0; i < (uint32_t)_CF_KEY_MAX; i++)
        SDL_AtomicSet(&context->keyStates[i], false);

    // initialize waitKey variables
    SDL_AtomicSet(&context->waitKeyRequired, false);
    SDL_AtomicSet(&context->waitKeyValue, CF_KEY_NULL);

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

    vmSandbox->getKeyState = sandboxGetKeyState;
    vmSandbox->waitKeyDown = sandboxWaitKeyDown;

    vmSandbox->readFloat64 = readFloat64;
    vmSandbox->writeFloat64 = writeFloat64;
} // sandboxConfigure

// sandbox.c
