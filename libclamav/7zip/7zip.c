/*
 * Clamav Native Windows Port: 7zip interface
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
 * Based and includes code from Lzma SDK:
 * Copyright (c) Igor Pavlov, license LGPL
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

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lzma/7zCrc.h"
#include "lzma/Archive/7z/7zIn.h"
#include "lzma/Archive/7z/7zExtract.h"

#include <clamav.h>
#include <scanners.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

typedef struct _CFileInStream
{
    ISzInStream InStream;
    int fd;
} CFileInStream;

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];

SZ_RESULT SzFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *) object;
  size_t processedSizeLoc;

  if (maxRequiredSize > kBufferSize) maxRequiredSize = kBufferSize;

  processedSizeLoc = read(s->fd, g_Buffer, maxRequiredSize);
  *buffer = g_Buffer;
  if (processedSize != 0) *processedSize = processedSizeLoc;
  return SZ_OK;
}

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
    CFileInStream *s = (CFileInStream *) object;
    int res = lseek(s->fd, (long) pos, SEEK_SET);
    if (!res) return SZE_FAIL;
    return SZ_OK;
}

const char *SzStrError(SZ_RESULT res)
{
    switch(res)
    {
        case SZE_DATA_ERROR:
            return "Data error";
        case SZE_OUTOFMEMORY:
            return "Out of memory";
        case SZE_CRC_ERROR:
            return "CRC error";
        case SZE_NOTIMPL:
            return "Method not implemented";
        case SZE_FAIL:
            return "Generic Failure";
        case SZE_ARCHIVE_ERROR:
            return "Archive error";
        default:
            return "Unknown 7zip error code";
    }
}

#define close_and_unlink() \
{ \
    if (ofd != -1) { close(ofd); ofd = -1; } \
    if (fname) { if (!ctx->engine->keeptmp) unlink(fname); free(fname); fname = NULL; } \
}

int cli_scan7zip(int fd, cli_ctx *ctx)
{
    CFileInStream ar;
    CArchiveDatabaseEx db;
    SZ_RESULT ret = SZ_OK;
    ISzAlloc aImp, aTempImp;
    CFileItem *f = NULL;
    UInt32 bIndex = -1, i = 0;
    Byte *obuff = NULL;
    size_t obufsz = 0, szdone = 0;
    size_t offset = 0;
    int ofd = -1;
    char *fname = NULL, *dir = NULL;

    ar.fd = fd;
    ar.InStream.Read = SzFileReadImp;
    ar.InStream.Seek = SzFileSeekImp;

    aImp.Alloc = SzAlloc;
    aImp.Free = SzFree;

    aTempImp.Alloc = SzAllocTemp;
    aTempImp.Free = SzFreeTemp;

    CrcGenerateTable();
    SzArDbExInit(&db);
    ret = SzArchiveOpen(&ar.InStream, &db, &aImp, &aTempImp);

    if (ret != SZ_OK)
    {
        cli_dbgmsg("7zip: SzArchiveOpen failed: %s\n", SzStrError(ret));
        SzArDbExFree(&db, aImp.Free);
        return CL_CLEAN;
    }

    if (ctx->engine->keeptmp)
    {
        if (!(dir = cli_gentemp(ctx->engine->tmpdir)))
            return CL_EMEM;

        if(mkdir(dir, 0700))
        {
            cli_warnmsg("7zip: Can't create temporary directory %s\n", dir);
            free(dir);
            return CL_ETMPDIR;
        }
    }

    for (i = 0; i < db.Database.NumFiles; i++)
    {
        f = db.Database.Files + i;
        if (f->IsDirectory) continue;

        if ((ret = cli_checklimits("7Zip", ctx, 0, (unsigned long) f->Size, 0)) != CL_CLEAN)
        {
            if ((ret != CL_EMAXFILES) && ((bIndex == -1) || (bIndex = i)))
                continue;
            else
                break;
        }

        if ((ret = SzExtract(&ar.InStream, &db, i,
                        &bIndex, &obuff, &obufsz,
                        &offset, &szdone, &aImp, &aTempImp)) != SZ_OK)
        {
            cli_dbgmsg("7zip: %s, while unpacking %s\n", SzStrError(ret), f->Name);
            ret = CL_CLEAN;
            break;
        }

        cli_dbgmsg("7zip: extracting %s\n", f->Name);

        if (!(fname = cli_gentemp(dir)))
            return CL_EMEM;

        if ((ofd = open(fname, O_RDWR|O_BINARY|O_CREAT|O_EXCL, 0600)) < 0)
        {
            cli_warnmsg("7zip: cannot create temp file: %s\n", strerror(errno));
            ret = CL_ETMPFILE;
            break;
        }

        if (write(ofd, obuff + offset, szdone) != szdone)
        {
            cli_warnmsg("7zip: cannot write to temp file: %s\n", strerror(errno));
            ret = CL_EWRITE;
            break;
        }

        lseek(ofd, 0, SEEK_SET);
        if ((ret = cli_magic_scandesc(ofd, ctx)) != CL_CLEAN) break;
        close_and_unlink();
    }

    close_and_unlink();

    aImp.Free(obuff);
    SzArDbExFree(&db, aImp.Free);
    if (dir) free(dir);
    cli_dbgmsg("7zip: Done\n");
    return ret;
}
