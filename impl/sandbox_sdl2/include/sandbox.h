/**
 * @brief SDL2-based sandbox declaration file
 */

#ifndef SANDBOX_SDL2_H_
#define SANDBOX_SDL2_H_

#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief sandbox context representation structure
typedef struct __SandboxContext {
    uint64_t font[256];                        ///< font

    SDL_Thread * sandboxThread;                ///< sandbox thread pointer

    // SDL -> VM
    SDL_atomic_t isTerminated;                 ///< true if SDL thread terminated, FALSE otherwise

    // VM -> SDL
    SDL_atomic_t shouldTerminate;              ///< flag should SDL thread finish execution
    SDL_atomic_t alwaysUpdate;                 ///< true if screen should be updating immediately after video memory change
    SDL_atomic_t manualUpdateRequested;        ///< true if screen should update after manual executor request only
    SDL_atomic_t pixelStorageFormat;           ///< 'CfVideoStorageFormat', way of interpreting framebuffer bytes

    uint64_t performanceFrequency;             ///< SDL timer frequency
    uint64_t initialPerformanceCounter;        ///< initial time

    void *memory;                              ///< executor memory pointer
    size_t memorySize;                         ///< size of executor memory

    SDL_atomic_t keyStates[SDL_NUM_SCANCODES]; ///< keys by scancode array
} SandboxContext;

/**
 * @brief VM sandbox configuration function
 * 
 * @param[out] vmSandbox VM sandbox pointer (non-null, zero-initialized)
 * @param[in]  context   context to configure vm sandbox to be used with (non-null)
 */
void sandboxConfigure( CfSandbox *vmSandbox, SandboxContext *context );

#ifdef __cplusplus
}
#endif

#endif // !defined(SANDBOX_SDL2_H_)

// sandbox.h
