/*
 * Clamav Native Windows Port: tls helper
 *
 * Copyright (c) 2010-2011 Gianluigi Tiesi <sherpya@netfarm.it>
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
#include <assert.h>

#define REDIR_COOKIE (PVOID) 0xdeedee13
//#define DEBUG_TLS

#ifdef DEBUG_TLS
#define TRACE(format, ...) fprintf(stderr, "[tls] " format, ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#endif

static DWORD __currentfile_idx = TLS_OUT_OF_INDEXES;
static DWORD __fsredir_idx = TLS_OUT_OF_INDEXES;

void tls_index_alloc(void)
{
    assert(__currentfile_idx == TLS_OUT_OF_INDEXES);
    assert(__fsredir_idx == TLS_OUT_OF_INDEXES);

    if ((__currentfile_idx = TlsAlloc()) == TLS_OUT_OF_INDEXES)
    {
        cli_errmsg("[tls] Unable to allocate Tls slot for currentfile storage: %d\n", GetLastError());
        exit(1);
    }

    if ((__fsredir_idx = TlsAlloc()) == TLS_OUT_OF_INDEXES)
    {
        cli_errmsg("[tls] Unable to allocate Tls slot for fsredir state storage: %d\n", GetLastError());
        exit(1);
    }

    TRACE("tls_index_alloc() T:%d F:IDX:%d R:IDX\n", GetCurrentThreadId(), __currentfile_idx, __fsredir_idx);
}

void tls_storage_alloc(void)
{
    PVOID *ptr;

    if (!(ptr = VirtualAlloc(NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
    {
        cli_errmsg("[tls] Unable to allocate memory for currentfile storage: %d\n", GetLastError());
        exit(1);
    }

    TlsSetValue(__currentfile_idx, ptr);

    if (!(ptr = VirtualAlloc(NULL, sizeof(PVOID), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
    {
        cli_errmsg("[tls] Unable to allocate memory for fsredir state storage: %d\n", GetLastError());
        exit(1);
    }

    *ptr = REDIR_COOKIE;
    TlsSetValue(__fsredir_idx, ptr);

    TRACE("tls_storage_alloc() T:%d F:IDX:%d R:IDX\n", GetCurrentThreadId(), __currentfile_idx, __fsredir_idx);
}

void tls_index_free(void)
{
    assert(__currentfile_idx != TLS_OUT_OF_INDEXES);
    assert(__fsredir_idx != TLS_OUT_OF_INDEXES);

    TRACE("tls_index_free() T:%d F:IDX:%d R:IDX\n", GetCurrentThreadId(), __currentfile_idx, __fsredir_idx);

    TlsFree(__currentfile_idx);
    TlsFree(__fsredir_idx);
}

void tls_storage_free(void)
{
    VirtualFree(TlsGetValue(__currentfile_idx), MAX_PATH, MEM_RELEASE);
    VirtualFree(TlsGetValue(__fsredir_idx), sizeof(PVOID), MEM_RELEASE);
    TRACE("tls_storage_free() T:%d F:IDX:%d R:IDX\n", GetCurrentThreadId(), __currentfile_idx, __fsredir_idx);
}

const char *cw_get_currentfile(void)
{
    const char *fptr = TlsGetValue(__currentfile_idx);
    TRACE("get_currentfile() T:%d F:IDX:%d fptr:0x%p\n", GetCurrentThreadId(), __currentfile_idx, fptr);
    return fptr;
}

void cw_set_currentfile(const char *filename)
{
    char *fptr = TlsGetValue(__currentfile_idx);
    TRACE("get_currentfile() T:%d F:IDX:%d fptr:0x%p\n", GetCurrentThreadId(), __currentfile_idx, fptr);

    if (!fptr)
    {
        cli_errmsg("[tls] cw_set_currentfile() TlsGetValue() failed %d\n", GetLastError());
        return;
    }

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
    BOOL result;
    PVOID *state = TlsGetValue(__fsredir_idx);

    TRACE("disablefsredir() T:%d R:IDX:%d S:0x%p\n", GetCurrentThreadId(), __fsredir_idx, state);

    if (*state != REDIR_COOKIE)
    {
        cli_errmsg("[tls] cw_disablefsredir() called multiple times\n");
        return FALSE;
    }

    if (GetLastError() != ERROR_SUCCESS)
    {
        cli_errmsg("[tls] cw_disablefsredir() TlsGetValue failed %d\n", GetLastError());
        return FALSE;
    }

    result = cw_helpers.k32.Wow64DisableWow64FsRedirection(state);
    return result;
}

BOOL cw_revertfsredir(void)
{
    BOOL result;
    PVOID *state = TlsGetValue(__fsredir_idx);
    TRACE("revertfsredir() T:%d R:IDX:%d S:0x%p\n", GetCurrentThreadId(), __fsredir_idx, state);

    if (!state)
    {
        if (GetLastError() == ERROR_SUCCESS)
            cli_errmsg("[tls] %d cw_revertfsredir() called without first calling cw_disablefsredir()\n", GetCurrentThreadId());
        else
            cli_errmsg("[tls] cw_revertfsredir() TlsGetValue failed %d\n", GetLastError());
        return FALSE;
    }

    if (*state == REDIR_COOKIE)
    {
        cli_errmsg("[tls] %d cw_revertfsredir() called multiple times\n", GetCurrentThreadId());
        return FALSE;
    }

    result = cw_helpers.k32.Wow64RevertWow64FsRedirection(state);
    *state = REDIR_COOKIE;
    return result;
}
