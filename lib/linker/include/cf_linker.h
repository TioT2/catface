/**
 * @brief linker ([CFOBJ] -> CFEXE translator) declaration file
 */

#ifndef CF_LINKER_H_
#define CF_LINKER_H_

#include <cf_executable.h>
#include <cf_object.h>
#include <cf_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief link details
typedef enum __CfLinkStatus {
    CF_LINK_STATUS_OK,              ///< succeeded
    CF_LINK_STATUS_INTERNAL_ERROR,  ///< internal error
    CF_LINK_STATUS_UNKNOWN_LABEL,   ///< unknown label
    CF_LINK_STATUS_DUPLICATE_LABEL, ///< duplicated label declaration
} CfLinkStatus;

/// @brief linking process details
typedef struct __CfLinkDetails {
    struct {
        CfStr  file;  ///< file where label is referenced
        size_t line;  ///< line in file where label is referenced
        CfStr  label; ///< referenced label
    } unknownLabel;

    struct {
        CfStr firstFile;   ///< file where first label is declared
        size_t firstLine;  ///< line where first label is declared
        CfStr secondFile;  ///< file where second label is declared
        size_t secondLine; ///< line where second label is declared

        CfStr label;       ///< label itself
    } duplicateLabel;
} CfLinkDetails;

/**
 * @brief objects linking function
 * 
 * @param[in]  objects     objects to link (non-null)
 * @param[in]  objectCount count of objects to link (non-zero)
 * @param[out] dst         executable building destination (non-null)
 * @param[out] details     more detailed info about linking process (nullable)
 * 
 * @return operation status
 */
CfLinkStatus cfLink(
    const CfObject * objects,
    size_t           objectCount,
    CfExecutable   * dst,
    CfLinkDetails  * details
);

#ifdef __cplusplus
}
#endif

#endif // !defined(CF_LINKER_H_)

// cf_linker.h
