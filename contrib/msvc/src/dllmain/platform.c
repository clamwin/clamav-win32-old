/*
 * Clamav Native Windows Port: platform specific helpers
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
#include <dirent.h>
#include <others.h>

#include <shared/output.h>

struct cfgstruct;

int localserver(const struct cfgstruct *copt) { return -1; }

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

static void cw_getregvalue(const char *key, char *path, char *default_value)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD flags = KEY_QUERY_VALUE;
    unsigned char data[MAX_PATH];
    DWORD datalen = sizeof(data);

    strncpy(path, default_value, MAX_PATH - 1);
    path[MAX_PATH - 1] = 0;

    if (cw_iswow64()) flags |= KEY_WOW64_64KEY;

    /* First look in HKCU then in HKLM */
    if (RegOpenKeyExA(HKEY_CURRENT_USER, DATADIRBASEKEY, 0, flags, &hKey) != ERROR_SUCCESS)
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, DATADIRBASEKEY, 0, flags, &hKey) != ERROR_SUCCESS)
            return;

    if ((RegQueryValueExA(hKey, key, NULL, &dwType, data, &datalen) == ERROR_SUCCESS) &&
        datalen && ((dwType == REG_SZ) || dwType == REG_EXPAND_SZ))
    {
        path[0] = 0;
        ExpandEnvironmentStrings((LPCSTR) data, path, MAX_PATH - 1);
        path[MAX_PATH - 1] = 0;
    }
    RegCloseKey(hKey);
}

/* Picks data dir from the Windows Registry, then resolve the request.
   This function leaks memory, but it's called once or two times per run,
   the advantage is thread safety. */
char *cw_getpath(const char *base, const char *file)
{
    char path[MAX_PATH] = "", *result = NULL;
    cw_getregvalue(base, path, ".");
    if (file && *file)
    {
        if ((*file == '\\') || (*file == '/'))
            file++;
        strncat(path, "\\", MAX_PATH - 1 - strlen(path));
        path[MAX_PATH - 1] = 0;
        strncat(path, file, MAX_PATH - 1 - strlen(path));
        path[MAX_PATH - 1] = 0;
    }

    CW_CHECKALLOC(result, malloc(strlen(path) + 1), return NULL);
    strcpy(result, path);
    return result;
}

/* Get the alternative name for a file, so esotic names/paths can be easily accessed */
static char *cw_getaltname(const char *filename)
{
    size_t len = strlen(filename) + 6;
    size_t wlen = 0, pos = 0;
    char *lslash = strrchr(filename, '\\');
    char *name_a = NULL, *fqname_a = NULL;
    wchar_t *name_w = NULL;
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW wfdw;

    if (!lslash)
    {
        cli_errmsg("[platform] Unexpected path syntax\n");
        return NULL;
    }

    CW_CHECKALLOC(name_a, malloc(len), return NULL);

    snprintf(name_a, len - 1, "%s%s", cw_uncprefix(filename), filename);

    if (!(name_w = cw_mb2wc(name_a)))
    {
        cli_errmsg("[platform] Error in conversion from ansi to widechar (%d)\n", GetLastError());
        free(name_a);
        return NULL;
    }

    hf = FindFirstFileW(name_w, &wfdw);
    free(name_w);

    if (hf == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            cli_errmsg("[platform] No such file or directory\n");
        else
            cli_errmsg("[platform] FindFirstFileW() failed (%d)\n"
            "[%s] is not accessible by the OS\n", GetLastError(), name_a);
        free(name_a);
        return NULL;
    }

    FindClose(hf);
    free(name_a);
    if (wcslen(wfdw.cAlternateFileName) && (!(name_a = cw_wc2mb(wfdw.cAlternateFileName, 0))))
    {
        cli_errmsg("[platform] Error while getting alternate name (%d)\n", GetLastError());
        free(name_a);
        return NULL;
    }

    pos = lslash - filename + 1;
    len = pos + strlen(name_a) + 2;
    CW_CHECKALLOC(fqname_a, malloc(len), { free(name_a); return NULL; });
    strncpy(fqname_a, filename, pos);
    fqname_a[pos] = 0;
    strncat(fqname_a, name_a, len - 1 - strlen(fqname_a));
    free(name_a);
    return fqname_a;
}

/* A path is a path... not on Windows... */
char *cw_normalizepath(const char *path)
{
    size_t len = 0;
    char *plain = NULL, *seek = NULL;
    char *name_u = NULL;
    char *filename = NULL, *norm = NULL;

    assert(path);
    len = strlen(path);
    /* NULL + trailing \ */
    CW_CHECKALLOC(filename, malloc(len + 2), return NULL);

    strcpy(filename, path);
    cw_pathtowin32(filename);
    cw_rmtrailslashes(filename);
    len = strlen(filename);
    plain = PATH_PLAIN(filename);

    if ((strlen(plain) == 2) && (plain[1] == ':')) /* Allow c: d: notation */
    {
        strcat(filename, "\\");
        return filename;
    }
    else if (!PATH_ISNET(filename))
    {
        /* relative path, then add current directory */
        if (plain[1] != ':')
        {
            char *fq = NULL;
            size_t clen = 0;

            if (!(fq = cw_getcurrentdir()))
            {
                cli_errmsg("[platform] cw_getcurrentdir() failed %d\n", GetLastError());
                return NULL;
            }

            clen = len + strlen(fq) + 2;

            /* \path notation */
            if ((plain[0] == '\\') && ((seek = strchr(fq, ':')))) (*(seek + 1) = 0);

            /* reallocate the string to make room for the filename */
            CW_CHECKALLOC(fq, realloc(fq, clen + strlen(filename)), return NULL);
            strncat(fq, "\\", clen - 1 - strlen(fq));
            strncat(fq, filename, clen - 1 - strlen(fq));
            free(filename);
            filename = fq;
        }

        /* Win9x does not like paths like c:\\something */
        if (isWin9x())
        {
            char *p = NULL, *s = NULL;
            p = s = filename;
            while (*s && *p)
            {
                *p = *s;
                if (*s == '\\')
                {
                    p++;
                    while (*s && (*s == '\\')) s++;
                    continue;
                }
                p++; s++;
            }
            *p = 0;
        }
        else if (!PATH_ISUNC(filename))
        {
            len = strlen(filename) + sizeof(UNC_PREFIX) + 1;
            CW_CHECKALLOC(name_u, malloc(len), return NULL);
            snprintf(name_u, len - 1, "%s%s", cw_uncprefix(filename), filename);
            free(filename);
            filename = name_u;
        }
    }

    /* total path len > MAX_PATH, it's valid but some of windows api do not think so */
    if (len > MAX_PATH)
        norm = cw_getaltname(filename);
    else
        norm = cw_getfullpathname(filename);

    free(filename);

    cli_dbgmsg("\nPath converted from [%s] to [%s]\n", path, norm);
    return norm;
}

int cw_movefileex(const char *source, const char *dest, DWORD flags)
{
    assert(source);
    if (!source)
    {
        SetLastError(ERROR_BAD_ARGUMENTS);
        return 0;
    }
    if (!isWin9x()) return (MoveFileExA(source, dest, flags));

    /* Yuppi, MoveFileEx on Win9x */

    if (flags & MOVEFILE_REPLACE_EXISTING)
        DeleteFileA(dest);

    if (flags & MOVEFILE_DELAY_UNTIL_REBOOT)
    {
        char WinInitIni[MAX_PATH] = "";
        char ssource[MAX_PATH] = "";
        GetWindowsDirectoryA(WinInitIni, MAX_PATH - 1);
        WinInitIni[MAX_PATH - 1] = 0;
        strncat(WinInitIni, "\\wininit.ini", MAX_PATH - 1 - strlen(WinInitIni));
        WinInitIni[MAX_PATH - 1] = 0;

        /* Currently first copy the file to the destination, then schedule the remove.
           I cannot known the 8.3 path before having the file, so wininit stuff
           will fail, and windows 98 needs to be rebooted two times, since the first
           time freezes for me */

        if (dest) CopyFileA(source, dest, 0);

        if (!GetShortPathNameA(source, ssource, MAX_PATH - 1))
        {
            cli_warnmsg("GetShortPathNameA() for %s failed %d\n", source, GetLastError());
            return 0;
        }
        ssource[MAX_PATH - 1] = 0;
        return (WritePrivateProfileStringA("rename", "NUL", ssource, WinInitIni));
    }

    return (MoveFileA(source, dest));
}

#define ISLOCKED(error) \
    ((error == ERROR_ACCESS_DENIED) || (error == ERROR_SHARING_VIOLATION) || (error == ERROR_LOCK_VIOLATION))

int cw_movefile(const char *source, const char *dest, int reboot)
{
    if (!reboot)
    {
        FIXATTRS(source);
        FIXATTRS(dest);
        if (cw_movefileex(source, dest, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return 1;
        if (!ISLOCKED(GetLastError()))
        {
            cli_warnmsg("%s cannot be moved (%lu)\n", source, GetLastError());
            return 0;
        }
        cli_warnmsg("%s cannot be moved (%lu), scheduling the move operation for next reboot\n", source, GetLastError());
    }
    if (cw_movefileex(source, dest, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING))
        return 1;
    cli_warnmsg("error scheduling the move operation for reboot (%lu)\n", GetLastError());
    return 0;
}

int cw_unlink(const char *pathname)
{
    FIXATTRS(pathname);
    if (!DeleteFileA(pathname))
    {
        if (!ISLOCKED(GetLastError())) return 1;
        cli_warnmsg("%s cannot be deleted, scheduling for deletetion at next reboot\n", pathname);
        cw_movefile(pathname, NULL, 1);
    }
    return 0;
}

/* Recursively remove a directory, portable version */
int cw_rmdirs(const char *dirname)
{
    DIR *dd = NULL;
    struct dirent *dent = NULL;
    struct stat maind, statbuf;
    char *path = NULL;

    _chmod(dirname, 0700);
    if (stat(dirname, &maind) == -1) return 0; /* Doesn't exists */

    if (!_rmdir(dirname)) return 0; /* ok - deleted */

    if ((errno != ENOTEMPTY) && !isWin9x())
    {
        cli_errmsg("Can't remove temporary directory %s: %s\n", dirname, strerror(errno));
        return -1;
    }

    if ((dd = opendir(dirname)) == NULL) return -1;

    while ((dent = readdir(dd)))
    {
        size_t len = 0;
        /* Skip . and .. */
        if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) continue;

        len = strlen(dirname) + strlen(dent->d_name) + 3;
        CW_CHECKALLOC(path, malloc(len), break);
        snprintf(path, len - 1, "%s/%s", dirname, dent->d_name);
        path[len - 1] = 0;

        /* stat the entry */
        if (lstat(path, &statbuf) == -1)
        {
            /* Possible causes:
               - Directory has only read permission but not execute,
               - The file is share-locked (win32_stat uses CreateFile())
               - Sherpya b0rked the dirent or other code
             */
            free(path);
            continue;
        }

        if (S_ISDIR(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode))
        {
            /* I've found a directory */
            _chmod(dirname, 0700); /* Fix permissions */
            if (_rmdir(path))
            {
                if ((errno == ENOTEMPTY) || isWin9x())
                    cw_rmdirs(path); /* If it's not empty then recurse */
                else
                {
                    /* Cannot be deleted */
                    cli_errmsg("[inner] Can't remove temporary directory %s: %s\n", path, strerror(errno));
                    closedir(dd);
                    free(path);
                    return 0;
                }
            }
        }
        else /* It's a regular file */
            if (cw_unlink(path)) cli_errmsg("Can't unlink temporary file %s: %s\n", path, strerror(errno));
        free(path);
    }

    closedir(dd);

    /* Finally try to remove the directory itself */
    if (_rmdir(dirname))
        cli_errmsg("[outer] Can't remove temporary directory %s: %s\n", dirname, strerror(errno));

    return 0;
}


/* only setting O_NONBLOCK is supported - F_GETFL returns always 0 */
int fcntl(int fd, int cmd, long arg)
{
    u_long mode = (arg & O_NONBLOCK);

    switch (cmd)
    {
        case F_GETFL: return 0;
        case F_SETFL:
            if (!ioctlsocket(fd, FIONBIO, &mode))
                return 0;
    }

    errno = EBADF;
    return -1;
}

int getpagesize(void)
{
    int pagesize = 0;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}
