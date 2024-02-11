/*

        c_escape.h by EliteAsian123

        MIT License

        https://github.com/EliteAsian123/c_escape

*/

#ifndef C_ESCAPE_H
#define C_ESCAPE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define CE_C 0
#define CE_JSON 1

/**
 * Escapes a string. Converts newline characters into "\\n", tabs into "\\t", etc.
 *
 * @param str The original null-terminated char*.
 * @param type What style/flavor of escaping to use. Use "CE_C", or "CE_JSON".
 *
 * @return An automatically allocated null-terminated char* of the correct size. This should be freed after use.
 */
char* escape(const char* str, int type);

/**
 * Unescapes a string. Converts "\\n" into newline characters, "\\t" into tabs, etc.
 *
 * @param str The original null-terminated char*.
 * @param type What style/flavor of unescaping to use. Use "CE_C", or "CE_JSON".
 * @param error_pos The index of the error relative to the original string. This can be "NULL".
 *
 * @return If an error occurs, "NULL" will be returned. Otherwise, an automatically allocated null-terminated char* of the
 * correct size will be returned. This should be freed after use.
 */
char* unescape(const char* str, int type, int* error_pos);

#endif
