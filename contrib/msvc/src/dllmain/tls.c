/*
 * Clamav Native Windows Port: tls helper
 *
 * Copyright (c) 2010 Gianluigi Tiesi <sherpya@netfarm.it>
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
#include <others.h>

// __declspec(thread) are not happy with python, maybe different crt?
//#ifdef _MSC_VER
//#define HAVE_DECLSPEC_THREAD
//#endif

#ifdef HAVE_DECLSPEC_THREAD
static __declspec(thread) const char *__currentfile = NULL;
static __declspec(thread) PVOID state = NULL;

void tls_alloc(void) {}
void tls_free(void) {}

const char *cw_get_currentfile(void)
{
    return __currentfile;
}

void cw_set_currentfile(const char *filename)
{
    __currentfile = filename;
}

BOOL cw_disablefsredir(void)
{
    cli_dbgmsg("[tls] DISABLED REDIR for %d\n", GetCurrentThreadId());
    return cw_helpers.k32.Wow64DisableWow64FsRedirection(&state);
}

BOOL cw_revertfsredir(void)
{
    cli_dbgmsg("[tls] REVERTED REDIR for %d\n", GetCurrentThreadId());
    return cw_helpers.k32.Wow64RevertWow64FsRedirection(&state);
}

#else
static DWORD __currentfile_idx = TLS_OUT_OF_INDEXES;
static DWORD __fsredir_idx = TLS_OUT_OF_INDEXES;

void tls_alloc(void)
{
    __currentfile_idx = TlsAlloc();
    if (__currentfile_idx == TLS_OUT_OF_INDEXES)
    {
        cli_errmsg("[tls] Unable to allocate Tls slot for currentfile storage: %d\n", GetLastError());
        exit(1);
    }

    __fsredir_idx = TlsAlloc();
    if (__fsredir_idx == TLS_OUT_OF_INDEXES)
    {
        cli_errmsg("[tls] Unable to allocate Tls slot for fsredir state storage: %d\n", GetLastError());
        exit(1);
    }
}

void tls_free(void)
{
    TlsFree(__currentfile_idx);
    TlsFree(__fsredir_idx);
}

const char *cw_get_currentfile(void)
{
    return TlsGetValue(__currentfile_idx);
}

void cw_set_currentfile(const char *filename)
{
    TlsSetValue(__currentfile_idx, (LPVOID) filename);
}

BOOL cw_disablefsredir(void)
{
    PVOID *state = TlsGetValue(__fsredir_idx);
    cli_dbgmsg("[tls] DISABLED REDIR for %d\n", GetCurrentThreadId());

    if (state)
    {
        cli_errmsg("[tls] cw_disablefsredir() called multiple times\n");
        return FALSE;
    }

    if (GetLastError() != ERROR_SUCCESS)
    {
        cli_errmsg("[tls] cw_disablefsredir() TlsGetValue failed %d\n", GetLastError());
        return FALSE;
    }

    state = calloc(1, sizeof(PVOID));
    TlsSetValue(__fsredir_idx, state);
    return cw_helpers.k32.Wow64DisableWow64FsRedirection(state);
}

BOOL cw_revertfsredir(void)
{
    BOOL result;
    PVOID *state = TlsGetValue(__fsredir_idx);
    cli_dbgmsg("[tls] REVERTED REDIR for %d\n", GetCurrentThreadId());

    if (!state)
    {
        if (GetLastError() == ERROR_SUCCESS)
            cli_errmsg("[tls] cw_revertfsredir() called without first calling cw_disablefsredir()\n");
        else
            cli_errmsg("[tls] cw_revertfsredir() TlsGetValue failed %d\n", GetLastError());
        return FALSE;
    }

    result = cw_helpers.k32.Wow64RevertWow64FsRedirection(state);
    free(*state);
    TlsSetValue(__fsredir_idx, NULL);
    return result;
}
#endif
