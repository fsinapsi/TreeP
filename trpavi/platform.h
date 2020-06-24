/*
 * platform.h -- platform utilities wrapper for stream handling libraries
 *               (avilib, wavilib) in transcode.
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

#ifndef PLATFORM_H
#define PLATFORM_H

#include "../trp/trp.h"

#ifdef OS_DARWIN
#include <sys/uio.h>
#endif

#include <fcntl.h>
#include <errno.h>

/*************************************************************************/
/* libc-like memory handling                                             */
/*************************************************************************/

void *_plat_malloc(const char *file, int line, size_t size);
void *_plat_zalloc(const char *file, int line, size_t size);
void *_plat_realloc(const char *file, int line, void *ptr, size_t size);
void plat_free(void *ptr);

#define plat_malloc(size) \
            _plat_malloc(__FILE__, __LINE__, size)
#define plat_zalloc(size) \
            _plat_zalloc(__FILE__, __LINE__, size)
#define plat_realloc(p,size) \
            _plat_realloc(__FILE__, __LINE__, p, size)

/*************************************************************************/
/* simple logging facility                                               */ 
/*************************************************************************/

typedef enum platloglevel_ PlatLogLevel;
enum platloglevel_ {
    PLAT_LOG_DEBUG = 0,
    PLAT_LOG_INFO,
    PLAT_LOG_WARNING,
    PLAT_LOG_ERROR,
};

int plat_log_open(void);
int plat_log_send(PlatLogLevel level,
                  const char *tag, const char *fmt, ...);
int plat_log_close(void);

#endif /* PLATFORM_H */
