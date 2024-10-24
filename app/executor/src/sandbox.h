/**
 * @brief SDL2-based sandbox declaration file
 */

#include <stdbool.h>
#include <SDL2/SDL.h>

/// @brief font representation structure
typedef struct __SandboxFont {
    uint64_t letters[256]; ///< font 8x8 letters
} SandboxFont;

/// @brief key representation enumeration
typedef enum __CfKey {
    CF_KEY_A,             ///< A key
    CF_KEY_B,             ///< B key
    CF_KEY_C,             ///< C key
    CF_KEY_D,             ///< D key
    CF_KEY_E,             ///< E key
    CF_KEY_F,             ///< F key
    CF_KEY_G,             ///< G key
    CF_KEY_H,             ///< H key
    CF_KEY_I,             ///< I key
    CF_KEY_J,             ///< J key
    CF_KEY_K,             ///< K key
    CF_KEY_L,             ///< L key
    CF_KEY_M,             ///< M key
    CF_KEY_N,             ///< N key
    CF_KEY_O,             ///< O key
    CF_KEY_P,             ///< P key
    CF_KEY_Q,             ///< Q key
    CF_KEY_R,             ///< R key
    CF_KEY_S,             ///< S key
    CF_KEY_T,             ///< T key
    CF_KEY_U,             ///< U key
    CF_KEY_V,             ///< V key
    CF_KEY_W,             ///< W key
    CF_KEY_X,             ///< X key
    CF_KEY_Y,             ///< Y key
    CF_KEY_Z,             ///< Z key

    CF_KEY_RETURN,        ///< return key
    CF_KEY_SPACE,         ///< space
    CF_KEY_SEMICOLON,     ///< semicolon
    CF_KEY_COMMA,         ///< comma
    CF_KEY_DOT,           ///< dot
    CF_KEY_SLASH,         ///< slash
    CF_KEY_QUOTE,         ///< quote
    CF_KEY_BACKSLASH,     ///< bacslash
    CF_KEY_TILDE,         ///< tilde (or backquote)
    CF_KEY_TAB,           ///< tab

    CF_KEY_0,             ///< 0 key
    CF_KEY_1,             ///< 
    CF_KEY_2,             ///< 
    CF_KEY_3,             ///< 
    CF_KEY_4,             ///< 
    CF_KEY_5,             ///< 
    CF_KEY_6,             ///< 
    CF_KEY_7,             ///< 
    CF_KEY_8,             ///< 
    CF_KEY_9,             ///< 
    CF_KEY_MINUS,         ///< 
    CF_KEY_EQUAL,         ///< 

    CF_KEY_LEFT_BRACKET,  ///< 
    CF_KEY_RIGHT_BRACKET, ///< 

    CF_KEY_SHIFT,         ///< 
    CF_KEY_ALT,           ///< 
    CF_KEY_CTRL,          ///< 

} CfKey;

/// @brief sandbox context representation structure
typedef struct __SandboxContext {
    SandboxFont * font;                        ///< font

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
 * @brief program execution time (in seconds) getting function
 * 
 * @param[in]  userContext user-provided context
 * @param[out] dst         time destination
 * 
 * @return true if succeeded, false if something went wrong.
 * 
 * @note matches prototype of 'CfSandbox::getExecutionTime' function pointer
 */
bool sandboxGetExecutionTime( void *userContext, float *dst );

/**
 * @brief sandbox initialization function
 * 
 * @param[in] context     user context
 * @param[in] execContext execution context
 * 
 * @note matches prototype of 'CfSandbox::initialize' function pointer
 */
bool sandboxInitialize( void *userContext, const CfExecContext *execContext );

/**
 * @brief vm panic handling function
 * 
 * @param context   sandbox user context
 * @param termInfo describes occured panic (non-null)
 * 
 * @note matches prototype of 'CfSandbox::terminate' function pointer
 */
void sandboxTerminate( void *userContext, const CfTermInfo *termInfo );

/**
 * @brief screen update function
 * 
 * @param[in] userContext user context reference
 * 
 * @return true if succeeded, false otherwise.
 * 
 * @note matches prototype of 'CfSandbox::refreshScreen' function pointer
 */
bool sandboxRefreshScreen( void *userContext );

/**
 * @brief video mode setting function
 * 
 * @param userContext   sandbox context pointer
 * @param storageFormat pixel storage format
 * @param updateMode    screen update mode
 * 
 * @note matches prototype of 'CfSandbox::setVideoMode' function pointer
 */
bool sandboxSetVideoMode(
    void                       *userContext,
    const CfVideoStorageFormat  storageFormat,
    const CfVideoUpdateMode     updateMode
);

// sandbox.h
