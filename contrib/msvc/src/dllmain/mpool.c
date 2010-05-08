/*
 * Clamav Native Windows Port: allocation replacement
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

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#ifdef USE_MPOOL

/* Using dlmalloc */
#define USE_DL_PREFIX 1
#include <src/helpers/dlmalloc.c>
#define RPL_MALLOC      dlmalloc
#define RPL_CALLOC      dlcalloc
#define RPL_REALLOC     dlrealloc
#define RPL_FREE        dlfree
#define RPL_MALLINFO    dlmallinfo

#include "mpool.h"
#include "others.h"
#include "str.h"
#include <string.h>

mpool_t *mpool_create(void)
{
    return (mpool_t *) 1234;
}

void mpool_destroy(mpool_t *mpool)
{
}

void *mpool_malloc(mpool_t *mpool, size_t size)
{
    return RPL_MALLOC(size);
}

void mpool_free(mpool_t *mpool, void *ptr)
{
    RPL_FREE(ptr);
}

void *mpool_calloc(mpool_t *mpool, size_t nmemb, size_t size)
{
    return RPL_CALLOC(nmemb, size);
}

void *mpool_realloc(mpool_t *mpool, void *ptr, size_t size)
{
    return RPL_REALLOC(ptr, size);
}

void *mpool_realloc2(mpool_t *mpool, void *ptr, size_t size)
{
    void *new_ptr = mpool_realloc(mpool, ptr, size);
    if (new_ptr)
        return new_ptr;
    mpool_free(mpool, ptr);
    return NULL;
}

unsigned char *cli_mpool_hex2str(mpool_t* mpool, const char *src)
{
    unsigned char *str;
    size_t len = strlen((const char *) src);

    if (len & 1)
    {
        cli_errmsg("cli_hex2str(): Malformed hexstring: %s (length: %u)\n", src, (unsigned) len);
        return NULL;
    }

    str = mpool_malloc(mpool, (len/2) + 1);
    if (cli_hex2str_to(src, (char *) str, len) == -1)
    {
        mpool_free(mpool, str);
        return NULL;
    }
    str[len / 2] = '\0';
    return str;
}

char *cli_mpool_strdup(mpool_t *mpool, const char *src)
{
    char *dst;
    size_t size = strlen(src) + 1;
    dst = mpool_malloc(mpool, size);
    memcpy(dst, src, size - 1);
    dst[size - 1] = 0;
    return dst;
}

char *cli_mpool_virname(mpool_t *mpool, const char *virname, unsigned int official)
{
    char *newname, *pt;

    if (!virname)
        return NULL;

    if ((pt = strchr(virname, ' ')))
        if ((pt = strstr(pt, " (Clam)")))
            *pt='\0';

    if (!virname[0])
    {
        cli_errmsg("cli_virname: Empty virus name\n");
        return NULL;
    }

    if (official)
        return cli_mpool_strdup(mpool, virname);

    newname = (char *) mpool_malloc(mpool, strlen(virname) + 11 + 1);

    if (!newname)
    {
        cli_errmsg("cli_virname: Can't allocate memory for newname\n");
        return NULL;
    }

    sprintf(newname, "%s.UNOFFICIAL", virname);
    return newname;
}

uint16_t *cli_mpool_hex2ui(mpool_t *mpool, const char *hex)
{
    uint16_t *str;
    unsigned int len;
  
    len = strlen(hex);

    if (len % 2 != 0)
    {
        cli_errmsg("cli_mpool_hex2ui(): Malformed hexstring: %s (length: %u)\n", hex, len);
        return NULL;
    }

    str = mpool_calloc(mpool, (len / 2) + 1, sizeof(uint16_t));
    if (!str)
        return NULL;

    if (cli_realhex2ui(hex, str, len))
        return str;
    
    mpool_free(mpool, str);
    return NULL;
}

void mpool_flush(mpool_t *mpool)
{
}

int mpool_getstats(const struct cl_engine *engine, size_t *used, size_t *total)
{
    struct mallinfo info = RPL_MALLINFO();
    *used = *total = info.uordblks;
    *total += info.fordblks;
    return 0;
}

#else
#include "mpool.h"
#include <sys/types.h>
#undef mpool_getstats
int mpool_getstats(const struct cl_engine *engine, size_t *used, size_t *total)
{
    return -1;
}
#endif /* USE_MPOOL */
