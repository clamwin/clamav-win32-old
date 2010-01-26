/*
 * Clamav Native Windows Port: dllmain
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

#include <osdeps.h>
#include <pthread.h>

BOOL WINAPI IsDebuggerPresent(void);

static uint32_t platform = 0;
static helpers_t helpers;

extern uint32_t cw_getplatform(void) { return platform; };
extern helpers_t *cw_gethelpers(void) { return &helpers; }
extern void jit_init(void);
extern void jit_uninit(void);

#define Q(string) # string

#define IMPORT_FUNC(m, x) \
    helpers.m.x = ( imp_##x ) GetProcAddress(helpers.m.hLib, Q(x));

#define IMPORT_FUNC_OR_FAIL(m, x) \
    IMPORT_FUNC(m, x); \
    if (!helpers.m.x) helpers.m.ok = FALSE;

static void dynLoad(void)
{
    memset(&helpers, 0, sizeof(helpers_t));
    memset(&helpers.av32, 0, sizeof(advapi32_t));
    memset(&helpers.k32, 0, sizeof(kernel32_t));
    memset(&helpers.psapi, 0, sizeof(psapi_t));
    memset(&helpers.ws2, 0, sizeof(ws2_32_t));

    helpers.k32.hLib = LoadLibraryA("kernel32.dll");
    helpers.av32.hLib = LoadLibraryA("advapi32.dll");
    helpers.psapi.hLib = LoadLibraryA("psapi.dll");
    helpers.ws2.hLib = LoadLibraryA("wship6.dll");
    if (!helpers.ws2.hLib)
        helpers.ws2.hLib = LoadLibraryA("ws2_32.dll");

    /* kernel 32*/
    if (helpers.k32.hLib) /* Unlikely */
    {
        /* Win2k + */
        IMPORT_FUNC(k32, HeapSetInformation);

        /* Win64 WoW from 32 applications */
        IMPORT_FUNC(k32, IsWow64Process);
        IMPORT_FUNC(k32, Wow64DisableWow64FsRedirection);
        IMPORT_FUNC(k32, Wow64RevertWow64FsRedirection);

        /* kernel32 */
        helpers.k32.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(k32, CreateToolhelp32Snapshot);
        IMPORT_FUNC_OR_FAIL(k32, Process32First);
        IMPORT_FUNC_OR_FAIL(k32, Process32Next);
        IMPORT_FUNC_OR_FAIL(k32, Module32First);
        IMPORT_FUNC_OR_FAIL(k32, Module32Next);
        IMPORT_FUNC_OR_FAIL(k32, CreateRemoteThread);
    }

    /* advapi32 */
    if (helpers.av32.hLib) /* Unlikely */
    {
        /* Win2k + */
        IMPORT_FUNC(av32, ChangeServiceConfig2A);

        helpers.av32.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(av32, OpenProcessToken);
        IMPORT_FUNC_OR_FAIL(av32, LookupPrivilegeValueA);
        IMPORT_FUNC_OR_FAIL(av32, AdjustTokenPrivileges);
    }

    /* psapi */
    if (helpers.psapi.hLib)
    {
        helpers.psapi.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(psapi, EnumProcessModules);
        IMPORT_FUNC_OR_FAIL(psapi, EnumProcesses);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleBaseNameA);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleFileNameExA);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleFileNameExW);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleInformation);
    }

    /* ws2_32 ipv6 */
    if (helpers.ws2.hLib)
    {
        helpers.ws2.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(ws2, getaddrinfo);
        IMPORT_FUNC_OR_FAIL(ws2, freeaddrinfo);
    }

    /* DynLoad jit */
    jit_init();
}
static void dynUnLoad(void)
{
    if (helpers.k32.hLib) FreeLibrary(helpers.k32.hLib);
    if (helpers.av32.hLib) FreeLibrary(helpers.av32.hLib);
    if (helpers.psapi.hLib) FreeLibrary(helpers.psapi.hLib);
    jit_uninit();
}

static uint32_t GetWindowsVersion(void)
{
    OSVERSIONINFOA osv;
    memset(&osv, 0, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    if (!GetVersionEx((LPOSVERSIONINFOA) &osv))
        osv.dwPlatformId = 0x0010400; /* Worst case report as Win95 */

    return ((osv.dwPlatformId << 16) | (osv.dwMajorVersion << 8) | (osv.dwMinorVersion));
}

/* avoid bombing in stupid msvcrt checks - msvcrt8 only */
#ifdef _MSC_VER
void clamavInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function,
   const wchar_t* file,
   unsigned int line,
   uintptr_t pReserved)
{
    fprintf(stderr, "\nW00ps!! you have something strange with this file\n(maybe crt versions mismatch)\n");

#ifdef _DEBUG
    if (expression && function && file && line)
        fwprintf(stderr, L"Expression: %s (%s at %s:%d)\n\n", expression, function, file, line);
#endif
}
#else
#define _set_invalid_parameter_handler(x)
#endif

static void cwi_processattach(void)
{
    ULONG HeapFragValue = 2;
    WSADATA wsaData;

    platform = GetWindowsVersion();
    dynLoad();

    if (helpers.k32.HeapSetInformation && !IsDebuggerPresent())
    {
        if (!helpers.k32.HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &HeapFragValue, sizeof(HeapFragValue))
            && (GetLastError() != ERROR_NOT_SUPPORTED))
            fprintf(stderr, "[DllMain] Error setting up low-fragmentation heap: %d\n", GetLastError());
    }

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
        fprintf(stderr, "[DllMain] Error at WSAStartup(): %d\n", WSAGetLastError());

    /* winsock will try to load dll from system32 when fs redirection is disabled,
       so we'll preload them */
    if (cw_iswow64())
    {
        LoadLibrary("mswsock.dll");
        LoadLibrary("winrnr.dll");
        LoadLibrary("wshtcpip.dll");
    }
}

/* Winsock internals */
#define SO_SYNCHRONOUS_ALERT    0x10
#define SO_SYNCHRONOUS_NONALERT 0x20
#define SO_OPENTYPE             0x7008

/* currently unused */
void cw_async_noalert(void)
{
    int sockopt = SO_SYNCHRONOUS_NONALERT;

    if (setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (char *) &sockopt, sizeof(sockopt)) < 0)
        fprintf(stderr, "[DllMain] Error setting sockets in synchronous non-alert mode (%d)\n", WSAGetLastError());
}

BOOL cw_iswow64(void)
{
    BOOL bIsWow64 = FALSE;

    if (!(helpers.k32.IsWow64Process &&
        helpers.k32.Wow64DisableWow64FsRedirection &&
        helpers.k32.Wow64RevertWow64FsRedirection))
        return FALSE;

    if (helpers.k32.IsWow64Process(GetCurrentProcess(), &bIsWow64))
        return bIsWow64;

    fprintf(stderr, "[dllmain] IsWow64Process() failed %d\n", GetLastError());
    return FALSE;
}

BOOL cw_fsredirection(BOOL value)
{
    static PVOID oldValue = NULL;
    BOOL result = FALSE;

    if (!cw_iswow64()) return TRUE;

    if (value)
        result = helpers.k32.Wow64RevertWow64FsRedirection(&oldValue);
    else
        result = helpers.k32.Wow64DisableWow64FsRedirection(&oldValue);

    if (!result)
        fprintf(stderr, "[dllmain] Unable to enabe/disable fs redirection %d\n", GetLastError());

    return result;
}

size_t cw_heapcompact(void)
{
    size_t lcommit = 0;
    if (!isWin9x())
    {
        if (!(lcommit = HeapCompact(GetProcessHeap(), 0)))
            fprintf(stderr, "[DllMain] Error calling HeapCompact() (%d)\n", GetLastError());
    }
    return lcommit;
}

void fix_paths();

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            pthread_win32_process_attach_np();
            DisableThreadLibraryCalls((HMODULE) hModule);
            cwi_processattach();
            _set_invalid_parameter_handler(clamavInvalidParameterHandler);
            fix_paths();
            break;
        }
        case DLL_THREAD_ATTACH:
            return pthread_win32_thread_attach_np();
        case DLL_THREAD_DETACH:
            return pthread_win32_thread_detach_np();
        case DLL_PROCESS_DETACH:
            pthread_win32_thread_detach_np();
            pthread_win32_process_detach_np();
            WSACleanup();
            dynUnLoad();
    }
    return TRUE;
}

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

static int cw_getregvalue(const char *key, char *path, char *default_value)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD flags = KEY_QUERY_VALUE;
    unsigned char data[MAX_PATH];
    DWORD datalen = sizeof(data);

    if (default_value)
    {
        strncpy(path, default_value, MAX_PATH - 1);
        path[MAX_PATH - 1] = 0;
    }

    if (cw_iswow64()) flags |= KEY_WOW64_64KEY;

    /* First look in HKCU then in HKLM */
    if (RegOpenKeyExA(HKEY_CURRENT_USER, DATADIRBASEKEY, 0, flags, &hKey) != ERROR_SUCCESS)
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, DATADIRBASEKEY, 0, flags, &hKey) != ERROR_SUCCESS)
            return 0;

    if ((RegQueryValueExA(hKey, key, NULL, &dwType, data, &datalen) == ERROR_SUCCESS) &&
        datalen && ((dwType == REG_SZ) || dwType == REG_EXPAND_SZ))
    {
        path[0] = 0;
        ExpandEnvironmentStrings((LPCSTR) data, path, MAX_PATH - 1);
        path[MAX_PATH - 1] = 0;
        RegCloseKey(hKey);
        return 1;
    }
    RegCloseKey(hKey);
    return 0;
}

/* look at win32/compat/libclamav_main.c for more info */
char _DATADIR[MAX_PATH] = "db";
char _CONFDIR[MAX_PATH] = ".";
char _CONFDIR_CLAMD[MAX_PATH] = "clamd.conf";
char _CONFDIR_FRESHCLAM[MAX_PATH] = "freshclam.conf";
char _CONFDIR_MILTER[MAX_PATH] = "clamav-milter.conf";

#undef DATADIR
#undef CONFDIR
const char *DATADIR = _DATADIR;
const char *CONFDIR = _CONFDIR;
const char *CONFDIR_CLAMD = _CONFDIR_CLAMD;
const char *CONFDIR_FRESHCLAM = _CONFDIR_FRESHCLAM;
const char *CONFDIR_MILTER = _CONFDIR_MILTER;

#define DATADIR _DATADIR
#define CONFDIR _CONFDIR
#define CONFDIR_CLAMD _CONFDIR_CLAMD
#define CONFDIR_FRESHCLAM _CONFDIR_FRESHCLAM
#define CONFDIR_MILTER _CONFDIR_MILTER

#include <shared/getopt.c>
#include <shared/optparser.c>

void fix_paths()
{
    cw_getregvalue("DataDir", _DATADIR, NULL);
    if (cw_getregvalue("ConfigDir", _CONFDIR, NULL))
    {
        snprintf(_CONFDIR_CLAMD, sizeof(_CONFDIR_CLAMD), "%s\\%s", _CONFDIR, "clamd.conf");
        snprintf(_CONFDIR_FRESHCLAM, sizeof(_CONFDIR_FRESHCLAM), "%s\\%s", _CONFDIR, "freshclam.conf");
        snprintf(_CONFDIR_MILTER, sizeof(_CONFDIR_MILTER), "%s\\%s", _CONFDIR, "clamav-milter.conf");
    }
}
