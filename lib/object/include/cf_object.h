/**
 * @brief object declaration file
 */

#ifndef CF_OBJECT_H_
#define CF_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <cf_hash.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief maximal length of jump label supported
#define CF_LABEL_MAX 64

/// @brief label (point to code or just constant)
typedef struct CfLabel_ {
    uint32_t sourceLine;               ///< source file line label declared at
    uint32_t value;                    ///< label underlying value
    uint32_t isRelative;               ///< should value be corrected during linking process (e.g. is it code offset or not)
    char     label     [CF_LABEL_MAX]; ///< label string
} CfLabel;

/// @brief link (references to certain code point with different semantics)
typedef struct CfLink_ {
    uint32_t sourceLine;               ///< line label declared at
    uint32_t codeOffset;               ///< offset link encodes
    char     label     [CF_LABEL_MAX]; ///< label
} CfLink;

/// @brief object (single .cfasm compilation result) represetnation structure
typedef struct CfObject_ {
    const char * sourceName; ///< name of object source name
    size_t       codeLength; ///< length of bytecode
    uint8_t    * code;       ///< bytecode itself
    size_t       linkCount;  ///< count of links in bytecode
    CfLink     * links;      ///< links itself
    size_t       labelCount; ///< count of labels in bytecode
    CfLabel    * labels;     ///< labels itself
} CfObject;

/// @brief object from file reading status representation enumeration
typedef enum CfObjectReadStatus_ {
    CF_OBJECT_READ_STATUS_OK,                   ///< succeeded
    CF_OBJECT_READ_STATUS_INTERNAL_ERROR,       ///< internal error occured
    CF_OBJECT_READ_STATUS_UNEXPECTED_FILE_END,  ///< reading from file failed
    CF_OBJECT_READ_STATUS_INVALID_OBJECT_MAGIC, ///< invalid object magic
    CF_OBJECT_READ_STATUS_INVALID_HASH,         ///< object file hash
} CfObjectReadStatus;

/**
 * @brief object from file reading function
 * 
 * @param[in]  file file to read (opened for binary reading)
 * @param[out] dst  reading destination
 * 
 * @return operation status
 */
CfObjectReadStatus cfObjectRead( FILE *file, CfObject *dst );

/// @brief object to file writing status representation enumeration
typedef enum CfObjectWriteStatus_ {
    CF_OBJECT_WRITE_STATUS_OK,          ///< succeeded
    CF_OBJECT_WRITE_STATUS_WRITE_ERROR, ///< writing to file error
} CfObjectWriteStatus;

/**
 * @brief object to file writing function
 * 
 * @param[in] file file to read object from (opened for binary writing)
 * @param[in] src  object to write
 * 
 * @return operation status
 */
bool cfObjectWrite( FILE *file, const CfObject *src );

/**
 * @brief object data destructor
 * 
 * @param[in] object object to destroy pointer
 */
void cfObjectDtor( CfObject *object );

/**
 * @brief object read status string getting function
 * 
 * @param status status to get value for
 * 
 * @return string corresponding to status
 */
const char * cfObjectReadStatusStr( CfObjectReadStatus status );



/// @brief object
typedef struct CfObject2_ CfObject2;

/// @brief object builder
typedef struct CfObjectBuilder_ CfObjectBuilder;




/**
 * @brief object destructor
 * 
 * @param[in] object to destroy (nullable)
 */
void cfObjectDtor2( CfObject2 *object );

/*
 * @brief get object code length
 * 
 * @param[in] object object pointer (non-null)
 * 
 * @return object code length
 */
size_t cfObjectGetCodeLength( const CfObject2 *object );

/**
 * @brief get object code
 * 
 * @param[in] object object pointer (non-null)
 * 
 * @return object code
 */
const void * cfObjectGetCode( const CfObject2 *object );

/**
 * @brief get object link array length
 * 
 * @param[in] object object pointer (non-null)
 * 
 * @return object link array length
 */
size_t cfObjectGetLinkArrayLength( const CfObject2 *object );

/**
 * @brief get object link array
 * 
 * @param[in] object object pointer (non-null)
 * 
 * @return object link array contents
 */
const CfLink * cfObjectGetLinkArray( const CfObject2 *object );

/**
 * @brief get object label array length
 * 
 * @param[in] object object pointer (non-null)
 * 
 * @return object label array length
 */
size_t cfObjectGetLabelArrayLength( const CfObject2 *object );

/**
 * @brief get object label array
 * 
 * @param[in] object object pointer (non-null)
 * 
 * @return object label array contents
 */
const CfLabel * cfObjectGetLabelArray( const CfObject2 *object );

/// @brief object reading status
typedef enum CfObjectReadStatus2_ {
    CF_OBJECT_READ_STATUS_2_OK,                  ///< success
    CF_OBJECT_READ_STATUS_2_INTERNAL_ERROR,      ///< internal error occured
    CF_OBJECT_READ_STATUS_2_UNEXPECTED_FILE_END, ///< file ended while more bytes required
    CF_OBJECT_READ_STATUS_2_INVALID_MAGIC,       ///< invalid object magic
    CF_OBJECT_READ_STATUS_2_INVALID_HASH,        ///< invalid file hash
} CfObjectReadStatus2;

/// @brief object reading result
typedef struct CfObjectReadResult2_ {
    CfObjectReadStatus2 status; ///< operation status

    union {
        CfObject2 *ok; ///< read succeeded

        struct {
            size_t offset;            ///< current file offset
            size_t requiredByteCount; ///< required byte count
            size_t actualByteCount;   ///< actually read
        } unexpectedFileEnd;

        struct {
            uint32_t expected; ///< object file magic
            uint32_t actual;   ///< actual file magic
        } invalidMagic;

        struct {
            CfHash expected; ///< expected hash
            CfHash actual;   ///< actual hash
        } invalidHash;
    };
} CfObjectReadResult2;

/**
 * @brief print object read result to file
 * 
 * @param[in] out    file to print to
 * @param[in] result result pointer (non-null)
 */
void cfObjectReadResultWrite( FILE *out, const CfObjectReadResult2 *result );

/**
 * @brief read object from file
 * 
 * @param[in] file    file to read object from (opened for RB reading)
 * @param[in] builder object builder (nullable)
 * 
 * @return object reading result
 */
CfObjectReadResult2 cfObjectRead2( FILE *file, CfObjectBuilder *builder );

/**
 * @brief write object to file
 * 
 * @param[in] file   file to write object to
 * @param[in] object object to write (non-null)
 * 
 * @return operation status
 */
CfObjectWriteStatus cfObjectWrite2( FILE *file, const CfObject2 *object );


/**
 * @brief object builder constructor
 * 
 * @return created object builder (NULL if failed)
 */
CfObjectBuilder * cfObjectBuilderCtor( void );

/**
 * @brief object builder destructor
 * 
 * @param[in] builder object builder to destroy (nullable)
 */
void cfObjectBuilderDtor( CfObjectBuilder *builder );

/**
 * @brief emit new object
 * 
 * @param[in] self builder to emit object in (non-null)
 * 
 * @return built object (NULL if smth went wrong)
 * 
 * @note this function **does not** performs builder reset,
 * you should do it manually if you want, of course.
 */
CfObject2 * cfObjectBuilderEmit( CfObjectBuilder *const self );

/**
 * @brief reset builder
 * 
 * @param[in] self builder to reset (non-null)
 * 
 * @note resets builder as if it was newly created
 */
void cfObjectBuilderReset( CfObjectBuilder *const self );

/**
 * @brief current code size getter
 * 
 * @param[in] self builder pointer (non-null)
 * 
 * @return current builder code size
 */
size_t cfObjectBuilderGetCodeLength( CfObjectBuilder *const self );

/**
 * @brief add code to object
 * 
 * @param[in] self builder pointer (non-null)
 * @param[in] code code to add (non-null)
 * @param[in] size code size
 * 
 * @return true if added, false if smth went wrong
 */
bool cfObjectBuilderAddCode( CfObjectBuilder *const self, const void *code, size_t size );

/**
 * @brief add link to object
 * 
 * @param[in] self builder pointer (non-null)
 * @param[in] code code to add (non-null)
 * @param[in] size code size
 * 
 * @return true if added, false if smth went wrong
 */
bool cfObjectBuilderAddLink( CfObjectBuilder *const self, const CfLink *link );

/**
 * @brief add link array to builder
 * 
 * @param[in] self   object pointer (non-null)
 * @param[in] array  array pointer (non-null)
 * @param[in] length length
 * 
 * @return true if added, false if not
 */
bool cfObjectBuilderAddLinkArray( CfObjectBuilder *const self, const CfLink *array, size_t length );

/**
 * @brief add label to object
 * 
 * @param[in] self  builder pointer
 * @param[in] label label
 * 
 * @return true if added, false if smth went wrong
 */
bool cfObjectBuilderAddLabel( CfObjectBuilder *const self, const CfLabel *label );

/**
 * @brief add link array to builder
 * 
 * @param[in] self   object pointer (non-null)
 * @param[in] array  array pointer (non-null)
 * @param[in] length length
 * 
 * @return true if added, false if not
 */
bool cfObjectBuilderAddLabelArray( CfObjectBuilder *const self, const CfLabel *array, size_t length );

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_OBJECT_H_)

// cf_object.h
