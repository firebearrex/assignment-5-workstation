/*
 * string_util.h
 *
 * Functions that implement string operations.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "string_util.h"

/**
 * Write the lower-case version of the source
 * string to the destination string. Source and
 * destination strings can be the same.
 *
 * @param dest destination string
 * @param src source string
 */
char *strlower(char *dest, const char *src)
{
	int i;
    for (i = 0; src[i] != '\0'; i++) {
        dest[i] = tolower(src[i]);
    }
    dest[i] = '\0';
    return dest;
}

/**
 * Returns true if src string ends with endswith string.
 *
 * @param src source string
 * @param endswith ends with string
 * @return true if src ends with endswith
 */
bool strendswith(const char *src, const char *endswith) {
	size_t srclen = strlen(src);
	size_t endslen = strlen(endswith);
	return (srclen >= endslen) && (strcmp(src+srclen-endslen, endswith) == 0);
}
