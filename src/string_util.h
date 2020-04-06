/*
 * string_util.h
 *
 * Functions that implement string operations.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#ifndef STRING_UTIL_H_
#define STRING_UTIL_H_

#include <stdbool.h>

/**
 * Write the lower-case version of the source
 * string to the destination string. Source and
 * destination strings can be the same.
 *
 * @param dest destination string
 * @param src source string
 */
char *strlower(char *dest, const char *src);

/**
 * Returns true if src string ends with endswith string.
 *
 * @param src source string
 * @param endswith ends with string
 * @return true if src ends with endswith
 */
bool strendswith(const char *src, const char *endswith);

#endif /* STRING_UTIL_H_ */
