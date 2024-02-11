/*

        c_escape.c by EliteAsian123

        MIT License

        https://github.com/EliteAsian123/c_escape

*/

#include "c_escape.h"

#define ONLY_C          \
        if (type != CE_C) { \
                goto _default;  \
        }

static const char* HEX = "0123456789ABCDEF";

static inline bool isNorm(char c) {
        return c >= ' ' && c <= '~';
}

static inline bool isHex(char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int fromHex(const char* str) {
        // If the characters aren't hex, fail
        if (!isHex(str[1]) || !isHex(str[2]) || !isHex(str[3]) || !isHex(str[4])) {
                return -1;
        }

        // Convert the 4 characters into an int
        char hex[5];
        hex[0] = str[1];
        hex[1] = str[2];
        hex[2] = str[3];
        hex[3] = str[4];
        hex[4] = '\0';
        return strtol(hex, NULL, 16);
}

char* escape(const char* str, int type) {
        // Get the length of the output string
        size_t len = strlen(str);
        size_t escapedLen = len;
        for (size_t i = 0; i < len; i++) {
                switch (str[i]) {
                        case '\a':
                        case '\e':
                        case '\v':
                                if (type != CE_C) {
                                        goto size_default;
                                }

                        case '\b':
                        case '\f':
                        case '\n':
                        case '\r':
                        case '\t':
                        /* case '\'': */ /* dà fastidio a deepl */
                        case '\"':
                        case '\\':
                                escapedLen++;
                                break;

                        size_default:
                        default:
                                if (!isNorm(str[i])) {
                                        // Use unsigned char for bit manipulation
                                        unsigned char val = (unsigned char) str[i];

                                        // Get the length of the unicode escape
                                        if (val <= 0x7F) {
                                                // BYTES : 1
                                                // ESCAPE: \xXX (4)
                                                // 4 - 1 = 3

                                                // OR

                                                // BYTES : 1
                                                // ESCAPE: \uXXXX (6)
                                                // 6 - 1 = 5

                                                if (type == CE_C) {
                                                        escapedLen += 3;
                                                } else {
                                                        escapedLen += 5;
                                                }
                                        } else if ((val & 0b11100000) == 0b11000000) {
                                                // BYTES : 2
                                                // ESCAPE: \uXXXX (6)
                                                // 6 - 2 = 4

                                                escapedLen += 4;
                                        } else if ((val & 0b11110000) == 0b11100000) {
                                                // BYTES : 3
                                                // ESCAPE: \uXXXX (6)
                                                // 6 - 3 = 3

                                                escapedLen += 3;
                                        } else if ((val & 0b11111000) == 0b11110000) {
                                                // BYTES : 4
                                                // ESCAPE: \uXXXX\uXXXX (12)
                                                // 12 - 4 = 8

                                                escapedLen += 8;
                                        }
                                        // Else, we are a continuation byte, so we don't need to escape it.
                                }

                                break;
                }
        }

        // Create output string
        char* escaped = malloc(escapedLen + 1);

#define ESC(c)           \
        escaped[j++] = '\\'; \
        escaped[j++] = c;

        // Fill output string
        size_t j = 0;
        for (size_t i = 0; i < len; i++) {
                // clang-format off
                switch (str[i]) {
                        case '\a': ONLY_C; ESC('a');    break;
                        case '\e': ONLY_C; ESC('e');    break;
                        case '\v': ONLY_C; ESC('v');    break;
                        case '\b':         ESC('b');    break;
                        case '\f':         ESC('f');    break;
                        case '\n':         ESC('n');    break;
                        case '\r':         ESC('r');    break;
                        case '\t':         ESC('t');    break;

                        /* case '\'': */ /* dà fastidio a deepl */
                        case '\"':
                        case '\\':         ESC(str[i]); break;

                        default:
                        _default:
                                if (!isNorm(str[i])) {
                                        // Use unsigned int for bit manipulation
                                        unsigned int val = (unsigned int) str[i];

                                        if (val <= 0x7F) {
                                                escaped[j++] = '\\';
                                                if (type == CE_C) {
                                                        escaped[j++] = 'x';
                                                        escaped[j++] = HEX[(val >> 4) & 0xF];
                                                        escaped[j++] = HEX[val        & 0xF];
                                                } else {
                                                        escaped[j++] = 'u';
                                                        escaped[j++] = '0';
                                                        escaped[j++] = '0';
                                                        escaped[j++] = HEX[(val >> 4) & 0xF];
                                                        escaped[j++] = HEX[val        & 0xF];
                                                }
                                        } else if ((val & 0b11100000) == 0b11000000) {
                                                // Add value of next UTF-8 continuation byte to val
                                                val = (val << 6)
                                                        | (str[i + 1] & 0b00111111);

                                                escaped[j++] = '\\';
                                                escaped[j++] = 'u';
                                                escaped[j++] = '0'; // Will always be 0
                                                escaped[j++] = HEX[(val >> 8 ) & 0xF];
                                                escaped[j++] = HEX[(val >> 4 ) & 0xF];
                                                escaped[j++] = HEX[val         & 0xF];
                                        } else if ((val & 0b11110000) == 0b11100000) {
                                                // Add values of next two UTF-8 continuation bytes to val
                                                val = (val << 12)
                                                        | ((str[i + 1] & 0b00111111) << 6)
                                                        | (str[i + 2] & 0b00111111);

                                                escaped[j++] = '\\';
                                                escaped[j++] = 'u';
                                                escaped[j++] = HEX[(val >> 12) & 0xF];
                                                escaped[j++] = HEX[(val >> 8 ) & 0xF];
                                                escaped[j++] = HEX[(val >> 4 ) & 0xF];
                                                escaped[j++] = HEX[(val      ) & 0xF];
                                        } else if ((val & 0b11111000) == 0b11110000) {
                                                // Add values of next three UTF-8 continuation bytes to val
                                                val = (val << 18)
                                                        | ((str[i + 1] & 0b00111111) << 12)
                                                        | ((str[i + 2] & 0b00111111) << 6)
                                                        | (str[i + 3] & 0b00111111);

                                                // Split into high and low surrogates
                                                val -= 0x10000;
                                                unsigned int high = (val >> 10 & 0x3FF) + 0xD800;
                                                unsigned int low  = (val       & 0x3FF) + 0xDC00;

                                                // Add high surrogate
                                                escaped[j++] = '\\';
                                                escaped[j++] = 'u';
                                                escaped[j++] = HEX[(high >> 12) & 0xF];
                                                escaped[j++] = HEX[(high >> 8 ) & 0xF];
                                                escaped[j++] = HEX[(high >> 4 ) & 0xF];
                                                escaped[j++] = HEX[(high      ) & 0xF];

                                                // Add low surrogate
                                                escaped[j++] = '\\';
                                                escaped[j++] = 'u';
                                                escaped[j++] = HEX[(low >> 12) & 0xF];
                                                escaped[j++] = HEX[(low >> 8 ) & 0xF];
                                                escaped[j++] = HEX[(low >> 4 ) & 0xF];
                                                escaped[j++] = HEX[(low      ) & 0xF];
                                        }
                                } else {
                                        escaped[j++] = str[i];
                                }

                                break;
                }
                // clang-format on
        }

#undef ESC

        // Add null terminator and return
        escaped[j] = '\0';
        return escaped;
}

char* unescape(const char* str, int type, int* error_pos) {
        // Create output string
        // We know that it will ALWAYS be shorter than the original
        int len = strlen(str);
        char* unescaped = malloc(len + 1);

        // Fill output string
        bool inEscape = false;
        size_t j = 0;
        for (size_t i = 0; i < len; i++) {
                if (!inEscape) {
                        if (str[i] == '\\') {
                                inEscape = true;
                        } else {
                                unescaped[j++] = str[i];
                        }
                } else {
                        // clang-format off
                        switch (str[i]) {
                                case 'a': ONLY_C; unescaped[j++] = '\a';   break;
                                case 'e': ONLY_C; unescaped[j++] = '\e';   break;
                                case 'v': ONLY_C; unescaped[j++] = '\v';   break;
                                case 'b':         unescaped[j++] = '\b';   break;
                                case 'f':         unescaped[j++] = '\f';   break;
                                case 'n':         unescaped[j++] = '\n';   break;
                                case 'r':         unescaped[j++] = '\r';   break;
                                case 't':         unescaped[j++] = '\t';   break;

                                case '\'':
                                case '\"':
                                case '\\':        unescaped[j++] = str[i]; break;

                                case 'x': {
                                        ONLY_C;

                                        // If there isn't 2 characters after the 'x', fail
                                        if (i + 2 >= len) {
                                                goto _default;
                                        }

                                        // If the characters after the 'x' aren't hex, fail
                                        if (!isHex(str[i + 1]) || !isHex(str[i + 2])) {
                                                goto _default;
                                        }

                                        // Convert the 2 characters into an int
                                        char hex[3];
                                        hex[0] = str[i + 1];
                                        hex[1] = str[i + 2];
                                        hex[2] = '\0';
                                        int value = strtol(hex, NULL, 16);

                                        // Add the value to the output string
                                        unescaped[j++] = value;

                                        // Skip the two characters
                                        i += 2;
                                        break;
                                }

                                case 'u': {
                                        // If there isn't 4 characters after the 'u', fail
                                        if (i + 4 >= len) {
                                                goto _default;
                                        }

                                        // Convert the 4 characters into an int
                                        int value = fromHex(str + i);
                                        if (value == -1) {
                                                goto _default;
                                        }

                                        // Check for high surrogate (for > U+FFFF)
                                        if (value >= 0xD800 && value <= 0xDBFF) {
                                                // Skip over the first escape code
                                                i += 5;

                                                // If there isn't 6 characters more characters (\uXXXX), fail
                                                if (i + 5 >= len) {
                                                        goto _default;
                                                }

                                                // If there isn't an escape sequence, fail
                                                if (str[i] != '\\' || str[i + 1] != 'u') {
                                                        goto _default;
                                                }

                                                // Skip over to the 'u'
                                                i++;

                                                // Convert the 4 characters into an int
                                                int value2 = fromHex(str + i);
                                                if (value2 == -1) {
                                                        goto _default;
                                                }

                                                // Convert high (value) and low (value2) surrogate to code point
                                                value -= 0xD800;
                                                value2 -= 0xDC00;
                                                value = (value << 10) + value2 + 0x10000;
                                        }

                                        // How many bytes?
                                        if (value <= 0x007F) {
                                                // 1 byte UTF-8
                                                unescaped[j++] = value;
                                        } else if (value <= 0x07FF) {
                                                // 2 byte UTF-8
                                                unescaped[j++] = 0xC0 | ((value >> 6 ) & 0x1F);
                                                unescaped[j++] = 0x80 | ((value      ) & 0x3F);
                                        } else if (value <= 0xFFFF) {
                                                // 3 byte UTF-8
                                                unescaped[j++] = 0xE0 | ((value >> 12) & 0x0F);
                                                unescaped[j++] = 0x80 | ((value >> 6 ) & 0x3F);
                                                unescaped[j++] = 0x80 | ((value      ) & 0x3F);
                                        } else {
                                                // 4 byte UTF-8
                                                unescaped[j++] = 0xF0 | ((value >> 18) & 0x07);
                                                unescaped[j++] = 0x80 | ((value >> 12) & 0x3F);
                                                unescaped[j++] = 0x80 | ((value >> 6 ) & 0x3F);
                                                unescaped[j++] = 0x80 | ((value      ) & 0x3F);
                                        }

                                        // Skip the 4 characters
                                        i += 4;
                                        break;
                                }

                                default:
                                _default:
                                        if (error_pos != NULL) {
                                                *error_pos = i + 1;
                                        }
                                        return NULL;
                        }
                        // clang-format on

                        inEscape = false;
                }
        }

        // Resize to fit, add null terminator, and return
        unescaped = realloc(unescaped, j + 1);
        unescaped[j] = '\0';
        return unescaped;
}

#undef ONLY_C
