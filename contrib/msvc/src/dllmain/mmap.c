/*
 * Clamav Native Windows Port: mmap win32 compatibility layer
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
#include <sys/mman.h>
#include <assert.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    HANDLE hMap = INVALID_HANDLE_VALUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD cf = 0, mf = 0;
    void *mm = NULL;

    assert(addr == NULL);
    if (addr)
    {
        fprintf(stderr, "[mmap] unsupported non-void mmap\n");
        return MAP_FAILED;
    }

    if (flags & MAP_PRIVATE)
    {
        cf = PAGE_WRITECOPY;
        mf = FILE_MAP_COPY;
    }
    else if (prot & PROT_WRITE)
    {
        cf = PAGE_READWRITE;
        mf = FILE_MAP_ALL_ACCESS;
    }
    else
    {
        cf = PAGE_READONLY;
        mf = FILE_MAP_READ;
    }

    if (fd >= 0)
        hFile = (HANDLE) _get_osfhandle(fd);

    if (!(hMap = CreateFileMappingA(hFile, NULL, cf, 0, len, NULL)))
    {
        errno = EBADF;
        return MAP_FAILED;
    }

    mm = MapViewOfFile(hMap, mf, 0, offset, len);
    CloseHandle(hMap);

    assert(mm);
    if (!mm)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    return mm;
}

int munmap(void *addr, size_t len)
{
    assert(addr);
    if (!UnmapViewOfFile(addr)) return -1;
    return 0;
}
