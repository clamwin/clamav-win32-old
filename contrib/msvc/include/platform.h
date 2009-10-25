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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <direct.h>  /* _mkdir()  */
#include <process.h> /* _getpid() */
#include <malloc.h>  /* _alloca() */

#include <posix-errno.h>
#include <safe_ctype.h>
#include <cw_inline.h>
#include <socket_inline.h>

/* re-route main to cw_main to handle some startup code */
#define main cw_main

#undef strtok_r /* thanks to pthread.h */

/* <strings.h> */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

/* cw */
extern char *cw_normalizepath(const char *path);
extern size_t cw_heapcompact(void);
extern BOOL cw_fsredirection(BOOL value);
extern void cw_registerservice(const char *name);
extern int cw_installservice(const char *name, const char *dname, const char *desc);
extern int cw_uninstallservice(const char *name, int verbose);

/* gnulib entries */
extern char *strtok_r(char *s, const char *delim, char **save_ptr);
extern struct tm *localtime_r(time_t const *t, struct tm *tp);
extern char *strptime (const char *buf, const char *format, struct tm *tm);

/* Re_routing */
extern int cw_stat(const char *path, struct stat *buf);
extern int cw_unlink(const char *pathname);

#define lstat           cw_stat
#define stat(p, b)      cw_stat(p, b)
#define unlink          cw_unlink
#define cli_unlink      cw_unlink
#define cli_rmdirs      cw_rmdirs
#define safe_open       cw_open

/* errno remap */
#define strerror cw_strerror
#define perror cw_perror

#define mkdir(a, b) mkdir(a)
#define match_regex cli_matchregex

#ifdef _MSC_VER
extern void cw_scanning(const char *filename);
#else
#define cw_scanning(x)
#endif

#define S_IROTH S_IREAD
#define S_ISLNK(x) (0)
#define S_IRWXO S_IEXEC
#define S_IRWXG S_IRWXU

/* <stdio.h> / <stdarg.h> */
/* Use snprintf and vsnprintf from gnulib, win32 crt has broken a snprintf */
#undef snprintf
#undef vsnprintf
#define snprintf gnulib_snprintf
#define vsnprintf gnulib_vsnprintf
extern int gnulib_snprintf(char *str, size_t size, const char *format, ...);
extern int gnulib_vsnprintf(char *str, size_t size, const char *format, va_list args);

#if defined(_MSC_VER)
#define fseeko _fseeki64
#elif defined(__GNUC__)
#define fseeko fseeko64
extern int __cdecl fseeko64 (FILE* stream, off64_t offset, int whence);
#else
#undef HAVE_FSEEKO
#endif

/* tmpfile() on win32 uses root dir, not suitable if non-admin */
#define tmpfile do_not_use_tmpfile_on_win32

/* <stdlib.h> */
extern int mkstemp(char *tmpl);

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

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

typedef	unsigned short in_port_t;
typedef	unsigned int in_addr_t;

#undef IMAGE_DOS_SIGNATURE

#define PATHSEP "\\"

#ifndef _OSDEPS_H_
#undef DATADIR
#undef CONFDIR
__declspec(dllimport) extern const char *DATADIR;
__declspec(dllimport) extern const char *CONFDIR;
__declspec(dllimport) extern const char *CONFDIR_CLAMD;
__declspec(dllimport) extern const char *CONFDIR_FRESHCLAM;
__declspec(dllimport) extern const char *CONFDIR_MILTER;
#undef HAVE_CONFIG_H
#endif

#endif /* _PLATFORM_H */
