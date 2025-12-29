/**
 * @file utf8To.h
 * @brief UTF-8 to single-byte Serbian LCD encoding converter.
 *
 * Provides conversion between UTF-8 Serbian characters (č, ć, š, ž, đ)
 * and their single-byte LCD codes or ASCII equivalents.
 */

#ifndef UTF8TO_H
#define UTF8TO_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculates the UTF-8 string length (number of characters).
 * @param s Pointer to UTF-8 string.
 * @return Number of characters (not bytes).
 */
size_t strlen_utf8(const char* s);

/**
 * @brief Converts UTF-8 Serbian characters to single-byte LCD codes or ASCII equivalents.
 * @param str Input UTF-8 string.
 * @param uppercase If true, convert to uppercase.
 * @return Pointer to static converted string (valid until next call).
 */
char* utf8To(const char* str, bool uppercase);

#ifdef __cplusplus
}
#endif

#endif  // UTF8TO_H
