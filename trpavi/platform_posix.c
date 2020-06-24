/*
 * platform_posix.c -- plain POSIX platform wrappers.
 * (C) 2007-2009 - Francesco Romani <fromani -at- gmail -dot- com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "platform.h"

/*************************************************************************/
/* Memory management is straightforward too.                             */
/*************************************************************************/

void *_plat_malloc(const char *file, int line, size_t size)
{
    return malloc(size);
}

void *_plat_zalloc(const char *file, int line, size_t size)
{
    return calloc(1, size);
}

void *_plat_realloc(const char *file, int line, void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void plat_free(void *ptr)
{
    free(ptr);
}

/*************************************************************************/
/* Trivial logging support.                                              */
/*************************************************************************/

int plat_log_open(void)
{
    return 0;
}

int plat_log_send(PlatLogLevel level,
                  const char *tag, const char *fmt, ...)
{
    /*
    char buffer[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buffer, 1024, fmt, ap);
    va_end(ap);

    fprintf(stderr, "[%s] %s\n", tag, buffer);
    */
    return 0;
}

int plat_log_close(void)
{
    return 0;
}

