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
#include <others.h>

static DWORD __currentfile_idx = TLS_OUT_OF_INDEXES;
static DWORD __fsredir_idx = TLS_OUT_OF_INDEXES;

void tls_alloc(void)
{
    char *fptr;

    __currentfile_idx = TlsAlloc();
    if (__currentfile_idx == TLS_OUT_OF_INDEXES)
    {
        cli_errmsg("[tls] Unable to allocate Tls slot for currentfile storage: %d\n", GetLastError());
        exit(1);
    }

    if (!(fptr = VirtualAlloc(NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
    {
        cli_errmsg("[tls] Unable to allocate memory for currentfile storage: %d\n", GetLastError());
        exit(1);
    }

    TlsSetValue(__currentfile_idx, (LPVOID) fptr);

    __fsredir_idx = TlsAlloc();
    if (__fsredir_idx == TLS_OUT_OF_INDEXES)
    {
        cli_errmsg("[tls] Unable to allocate Tls slot for fsredir state storage: %d\n", GetLastError());
        exit(1);
    }
}

void tls_free(void)
{
    VirtualFree(TlsGetValue(__currentfile_idx), MAX_PATH, MEM_RELEASE);
    TlsFree(__currentfile_idx);
    TlsFree(__fsredir_idx);
}

const char *cw_get_currentfile(void)
{
    return TlsGetValue(__currentfile_idx);
}

void cw_set_currentfile(const char *filename)
{
    char *fptr = TlsGetValue(__currentfile_idx);
    if (filename)
    {
        strncpy(fptr, filename, MAX_PATH - 1);
        fptr[MAX_PATH] = 0;
    }
    else
        fptr[0] = 0;
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

    state = malloc(sizeof(PVOID));
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
    free(state);
    TlsSetValue(__fsredir_idx, NULL);
    return result;
}
