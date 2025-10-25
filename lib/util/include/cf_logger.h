/**
 * @brief logger main declaration file
 */

#ifndef CF_LOGGER_H_
#define CF_LOGGER_H_

#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)

/**
 * @brief logger initialization function
 */
void cfLoggerInitialize( void );

/**
 * @brief logger deinitialization function
 */
void cfLoggerTerminate( void );

/**
 * @brief logger printing function
 * 
 * @param[in] fstr formatting string
 * @param  ...
 */
void cfLoggerPrint( const char *fstr, ... );

#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#endif // !defined(CF_LOGGER_H_)

// cf_logger.h
