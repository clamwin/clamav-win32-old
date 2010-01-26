/* This is needed to compile with MinGW using -std=c89 */

#ifndef __NONANSI_H_
#define __NONANSI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fileno(x) _fileno(x)
#define putenv(x) _putenv(x)
#define strdup(x) _strdup(x)
#define fdopen(x, y) _fdopen(x, y)

_CRTIMP int      __cdecl __MINGW_NOTHROW _stricmp (const char*, const char*);
_CRTIMP int      __cdecl __MINGW_NOTHROW _strnicmp (const char*, const char*, size_t);
_CRTIMP FILE*    __cdecl __MINGW_NOTHROW _fdopen (int, const char*);
_CRTIMP int      __cdecl __MINGW_NOTHROW _fileno (FILE*);
_CRTIMP int      __cdecl __MINGW_NOTHROW _putenv (const char*);
_CRTIMP char*    __cdecl __MINGW_NOTHROW _strdup (const char*) __MINGW_ATTRIB_MALLOC;
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wcsdup (const wchar_t*);

#endif /* __NONANSI_H_ */
