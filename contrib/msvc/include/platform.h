/*
 * Clamav Native Windows Port: platform specific helpers
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <cwdefs.h>
#include <winsock2.h>
#include <inttypes.h>
#include <malloc.h>
#include <assert.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <fcntl.h>

#include <cwhelpers.h>
#include <posix-errno.h>
#include <socket_inline.h>

#include <sys/types.h>

#define PROT_READ   0x1     /* Page can be read */
#define PROT_WRITE  0x2     /* Page can be written */
/* #define PROT_EXEC   0x4 */    /* Page can be executed */
/* #define PROT_NONE   0x0 */    /* Page can not be accessed */
#define PROT_EXEC   mmap_prot_exec_not_implemented
#define PROT_NONE   mmap_prot_none_not_implemented

#define MAP_SHARED    0x01    /* Share changes */
#define MAP_PRIVATE   0x02    /* Changes are private */
#define MAP_FIXED     0x10    /* Interpret addr exactly */ /* IGNORED on win32 */
#define MAP_ANONYMOUS 0x20    /* don't use a file */

/* Return value of 'mmap' in case of an error */
#define MAP_FAILED  ((void *) -1)

/* IGNORED on win32 */
#define madvise(a, b, c)

extern void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
extern int munmap(void *addr, size_t len);

#define DATADIRBASEKEY  "Software\\ClamAV"
#undef IMAGE_DOS_SIGNATURE

#ifdef _MSC_VER
extern void cw_scanning(const char *filename);
#else
#define cw_scanning(x)
#endif

extern uint32_t cw_getplatform(void);
extern helpers_t *cw_gethelpers(void);
extern char *cw_normalizepath(const char *path);
extern char *cw_getpath(const char *base, const char *file);
extern int cw_movefile(const char *source, const char *dest, int reboot);
extern int cw_movefileex(const char *source, const char *dest, DWORD flags);
extern void cw_registerservice(const char *name);
extern int cw_installservice(const char *name, const char *dname, const char *desc);
extern int cw_uninstallservice(const char *name, int verbose);

#define PlatformId          ((cw_getplatform() >> 16) & 0x000000ff)
#define PlatformMajor       ((cw_getplatform() >> 8 ) & 0x000000ff)
#define PlatformMinor       (cw_getplatform() & 0x000000ff)
#define PlatformVersion     (cw_getplatform() & 0x0000ffff)
#define isWin9x()           (PlatformId == VER_PLATFORM_WIN32_WINDOWS)
#define isOldOS()           (PlatformVersion <= 0x400)

#define LODWORD(l)  ((DWORD)((uint64_t)(l) & 0xffffffff))
#define HIDWORD(l)  ((DWORD)((uint64_t)(l) >> 32))

#define cli_rmdirs(directory) cw_rmdirs(directory)

/* UNC Path Handling on win32 */
#define UNC_PREFIX "\\\\?\\"
#define UN2_PREFIX "\\??\\"
#define DEV_PREFIX "\\\\.\\"
#define NET_PREFIX "\\\\"
#define UNC_OFFSET(x) (&x[4])
#define PATH_ISUNC(path)  (!strncmp(path, UNC_PREFIX, 4))
#define PATH_ISUN2(path)  (!strncmp(path, UN2_PREFIX, 4))
#define PATH_ISDEV(path)  (!strncmp(path, DEV_PREFIX, 4))
#define PATH_ISNET(path)  (!strncmp(path, NET_PREFIX, 2))
#define PATH_PLAIN(path)  (PATH_ISUNC(path) ? UNC_OFFSET(path) : path)

static inline const char *cw_uncprefix(const char *filename)
{
    if (PATH_ISUNC(filename) || PATH_ISNET(filename) || isWin9x())
        return "";
    else
        return UNC_PREFIX;
}

#define CW_CHECKALLOC(var, alloc, ret) \
do \
{ \
    if (!(var = alloc)) \
    { \
        fprintf(stderr, #alloc" failed\n"); \
        ret; \
    } \
} while (0)

#define NORMALIZE_PATH(path, free_src, fail)                        \
{                                                                   \
    char *swap = cw_normalizepath((char *) path);                   \
    if (free_src) free((void *) path);                              \
    if (!swap) { fail; }                                            \
    if (free_src)                                                   \
        CW_CHECKALLOC(path, malloc(strlen(swap) + 1), fail);        \
    else                                                            \
        CW_CHECKALLOC(path, alloca(strlen(swap) + 1), fail);        \
    strcpy((char *) path, swap);                                    \
    free(swap); swap = NULL;                                        \
}

#define FIXATTRS(filename) \
{ \
    DWORD dwAttrs = GetFileAttributes(filename); \
    SetFileAttributes(filename, dwAttrs & ~ (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN)); \
}

extern int cw_unlink(const char *pathname);
#define cli_unlink(x)   cw_unlink(x)
#define unlink(x)       cw_unlink(x)

extern BOOL cw_fsredirection(BOOL value);
extern BOOL cw_iswow64(void);
extern size_t cw_heapcompact(void);

struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};

static inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    struct timeb timebuffer;
    ftime(&timebuffer);
    tv->tv_sec = (long) timebuffer.time;
    tv->tv_usec = 1000 * timebuffer.millitm;
    return 0;
}

static inline wchar_t *cw_mb2wc(const char *mb)
{
    wchar_t *wc = NULL;
    DWORD len = 0;

    if (!(len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mb, -1, NULL, 0)))
        return NULL;

    CW_CHECKALLOC(wc, malloc(len * sizeof(wchar_t)), return NULL);

    if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mb, -1, wc, len))
        return wc;

    free(wc);
    return NULL;
}

#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS 1024
#endif

static inline char *cw_wc2mb(const wchar_t *wc, DWORD flags)
{
    BOOL invalid = FALSE;
    DWORD len = 0, res = 0;
    char *mb = NULL;

    /* NT4 does not like WC_NO_BEST_FIT_CHARS */
    if (isOldOS()) flags &= ~WC_NO_BEST_FIT_CHARS;

    len = WideCharToMultiByte(CP_ACP, flags, wc, -1, NULL, 0, NULL, &invalid);
    if (!len && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        fprintf(stderr, "WideCharToMultiByte() failed with %d\n", GetLastError());
        return NULL;
    }

    CW_CHECKALLOC(mb, malloc(len), return NULL);

    res = WideCharToMultiByte(CP_ACP, flags, wc, -1, mb, len, NULL, &invalid);
    if (res && ((!invalid || (flags != WC_NO_BEST_FIT_CHARS)))) return mb;
    free(mb);
    return NULL;
}

static inline char *cw_getfullpathname(const char *path)
{
    char *fp = NULL;
    DWORD len = GetFullPathNameA(path, 0, NULL, NULL);
    if (!len) return NULL;

    CW_CHECKALLOC(fp, malloc(len + 1), return NULL);

    if (GetFullPathNameA(path, len, fp, NULL))
        return fp;
    free(fp);
    return NULL;
}

static inline char *cw_getcurrentdir(void)
{
    DWORD len = GetCurrentDirectoryA(0, NULL);
    char *cwd = NULL;
    if (!len) return NULL;
    len++;

    CW_CHECKALLOC(cwd, malloc(len), return NULL);

    len = GetCurrentDirectoryA(len - 1, cwd);
    if (len) return cwd;
    free(cwd);
    return NULL;
}

static inline void cw_pathtowin32(char *name)
{
    /* UNC Paths need to have only backslashes */
    char *p = name;
    while (*p)
    {
        if (*p == '/') *p = '\\';
        p++;
    }
}

static inline void cw_rmtrailslashes(char *path)
{
    size_t i = strlen(path) - 1;
    while ((i > 0) && ((path[i] == '/') || (path[i] == '\\')))
        path[i--] = 0;
}

/* If timer represents a date before midnight, January 1, 1970,
   gmtime returns NULL. There is no error return */
static inline struct tm *safe_gmtime(const time_t *timer)
{
    struct tm *gmt = NULL;
    if ((*timer < 0) || !(gmt = gmtime(timer)))
    {
        time_t t = 0;
        gmt = gmtime(&t);
    }
    return gmt;
}
#define gmtime safe_gmtime

static inline int cw_open(const char *filename, int oflag, ...)
{
    va_list ap;
    int pmode = 0;

    va_start(ap, oflag);
    pmode = va_arg(ap, int);
    va_end(ap);

    if (oflag & _O_CREAT)
    {
        oflag |= _O_SHORT_LIVED;
        pmode |= (_S_IREAD | _S_IWRITE);
    }

    return _open(filename, oflag, pmode);
}

/* FIXME: check if this function works as expected */
static inline ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    off_t lastpos = lseek(fd, offset, SEEK_SET);
    ssize_t res;
    if (lastpos == -1) return -1;
    res = (ssize_t) read(fd, buf, (unsigned int) count);
    return ((lseek(fd, lastpos, SEEK_SET) == -1) ? -1 : res);
}

#include "../../../platform.h.in"

#endif /* _PLATFORM_H */
