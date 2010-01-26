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

#include <platform.h>
#include <pthread.h>
#include <sys/socket.h>

static uint32_t platform = 0;
static helpers_t helpers;

extern uint32_t cw_getplatform(void) { return platform; };
extern helpers_t *cw_gethelpers(void) { return &helpers; }

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

    helpers.k32.hLib = LoadLibraryA("kernel32.dll");
    helpers.av32.hLib = LoadLibraryA("advapi32.dll");
    helpers.psapi.hLib = LoadLibraryA("psapi.dll");

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
}
static void dynUnLoad(void)
{
    if (helpers.k32.hLib) FreeLibrary(helpers.k32.hLib);
    if (helpers.av32.hLib) FreeLibrary(helpers.av32.hLib);
    if (helpers.psapi.hLib) FreeLibrary(helpers.psapi.hLib);
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

#ifndef _DEBUG
    if (helpers.k32.HeapSetInformation &&
        !helpers.k32.HeapSetInformation(GetProcessHeap(),
        HeapCompatibilityInformation, &HeapFragValue, sizeof(HeapFragValue)))
        fprintf(stderr, "[DllMain] Error setting up low-fragmentation heap: %d\n", GetLastError());
#endif

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
