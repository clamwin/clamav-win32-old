/*
 * Clamav Native Windows Port
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

#ifndef _OSDEPS_H_
#define _OSDEPS_H_

#ifdef _DEBUG
#define CL_DEBUG 1
#endif

#ifdef __MINGW32__
#define HAVE_GETTIMEOFDAY 1
#endif

#include <platform.h>
#include <direct.h>

/* Process execution */
#define WIFSIGNALED(x) (x)
#define WIFEXITED(x) (1)
#define WTERMSIG(x) (x)
#include <process.h>

/* ctype.h functions are not safe on win32 */
#include <safe_ctype.h>

/* re-route main to cw_main to handle some startup code */
#define main cw_main

#ifdef LIBCLAMAV_EXPORTS
#define LIBCLAMAV_API
#else
#define LIBCLAMAV_API __declspec(dllimport)
#endif

/* errno remap */
#define strerror cw_strerror
#define perror cw_perror

#define mkdir(a, b) mkdir(a)

#define match_regex cli_matchregex

extern int getpagesize(void);

#undef strtok_r /* thanks to pthread.h */

/* gnulib entries */
extern char *strtok_r(char *s, const char *delim, char **save_ptr);
extern struct tm *localtime_r(time_t const *t, struct tm *tp);
extern char *strptime (const char *buf, const char *format, struct tm *tm);

/* Re_routing */
extern int cw_stat(const char *path, struct stat *buf);
#define lstat stat
#define stat(p, b) cw_stat(p, b)

#define S_IROTH S_IREAD
#define S_ISLNK(x) (0)
#define S_IRWXO S_IEXEC
#define S_IRWXG S_IRWXU

/* adds _O_SHORT_LIVED flag */
#define open cw_open
/* #define fopen(f, m) fopen(f, m"T") */

/* <stdio.h> / <stdarg.h> */
/* Use snprintf and vsnprintf from gnulib, win32 crt has broken a snprintf */
#undef snprintf
#undef vsnprintf
#define snprintf gnulib_snprintf
#define vsnprintf gnulib_vsnprintf
extern int snprintf(char *str, size_t size, const char *format, ...);
extern int vsnprintf(char *str, size_t size, const char *format, va_list args);

/* tmpfile() on win32 uses root dir, not suitable if non-admin */
#define tmpfile do_not_use_tmpfile_on_win32

/* <stdlib.h> */
extern int mkstemp(char *tmpl);

void clamscan_ctrl_handler(DWORD ctrl_type);

/* yes msvc 6 sucks... */
#ifndef _INTPTR_T_DEFINED
typedef long int intptr_t;
#define _INTPTR_T_DEFINED
#endif

static volatile const char portrev_rodata[] =
{
    0x43, 0x6c, 0x61, 0x6d, 0x41, 0x56, 0x20, 0x41,
    0x6e, 0x74, 0x69, 0x76, 0x69, 0x72, 0x75, 0x73,
    0x7c, 0x47, 0x50, 0x4c, 0x7c, 0x53, 0x6f, 0x75,
    0x72, 0x63, 0x65, 0x66, 0x69, 0x72, 0x65, 0x20,
    0x49, 0x6e, 0x63, 0x7c, 0x47, 0x69, 0x61, 0x6e,
    0x6c, 0x75, 0x69, 0x67, 0x69, 0x20, 0x54, 0x69,
    0x65, 0x73, 0x69, 0x7c, 0x3c, 0x73, 0x68, 0x65,
    0x72, 0x70, 0x79, 0x61, 0x40, 0x6e, 0x65, 0x74,
    0x66, 0x61, 0x72, 0x6d, 0x2e, 0x69, 0x74, 0x3e
};

#endif /* _OSDEPS_H_ */
