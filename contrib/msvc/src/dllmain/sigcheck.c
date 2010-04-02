/*
 * Clamav Native Windows Port: executables signature check
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
#include <fmap.h>
#include <wincrypt.h>

#include <pshpack8.h>

typedef struct WINTRUST_CATALOG_INFO_
{
    DWORD cbStruct;
    DWORD dwCatalogVersion;
    LPCWSTR pcwszCatalogFilePath;
    LPCWSTR pcwszMemberTag;
    LPCWSTR pcwszMemberFilePath;
    HANDLE hMemberFile;
    BYTE *pbCalculatedFileHash;
    DWORD cbCalculatedFileHash;
    PCCTL_CONTEXT pcCatalogContext;
} WINTRUST_CATALOG_INFO, *PWINTRUST_CATALOG_INFO;

typedef struct _WINTRUST_DATA
{
    DWORD cbStruct;
    LPVOID pPolicyCallbackData;
    LPVOID pSIPClientData;
    DWORD dwUIChoice;
    DWORD fdwRevocationChecks;
    DWORD dwUnionChoice;
    union
    {
        struct WINTRUST_FILE_INFO_ *pFile;
        struct WINTRUST_CATALOG_INFO_ *pCatalog;
        struct WINTRUST_BLOB_INFO_ *pBlob;
        struct WINTRUST_SGNR_INFO_ *pSgnr;
        struct WINTRUST_CERT_INFO_ *pCert;
    };
    DWORD dwStateAction;
    HANDLE hWVTStateData;
    WCHAR *pwszURLReference;
    DWORD dwProvFlags;
    DWORD dwUIContext;
} WINTRUST_DATA, *PWINTRUST_DATA;


#include <poppack.h>

#define WINTRUST_ACTION_GENERIC_VERIFY_V2 { 0xaac56b, 0xcd44, 0x11d0, { 0x8c, 0xc2, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee } }
#define WTD_UI_ALL 1
#define WTD_UI_NONE 2
#define WTD_STATEACTION_VERIFY 1
#define WTD_STATEACTION_CLOSE 2
#define WTD_CHOICE_CATALOG 2

#ifndef TRUST_E_NOSIGNATURE
#define TRUST_E_NOSIGNATURE 0x800B0100L
#endif

static int cw_getfnfromhandle(void *mem, wchar_t *filename)
{
    wchar_t szTemp[512] = L"";
    wchar_t szDrive[3] = L" :";
    wchar_t szName[MAX_PATH + 1];
    wchar_t *p = szTemp;
    wchar_t flname[MAX_PATH + 1];

    if (!cw_helpers.psapi.GetMappedFileNameW(GetCurrentProcess(), mem, flname, MAX_PATH))
        return 0;

    if (!GetLogicalDriveStringsW(sizeof(szTemp) - 1, szTemp))
        return 0;

    do
    {
        size_t len;
        *szDrive = *p;
        if (QueryDosDeviceW(szDrive, szName, MAX_PATH) && ((len = wcslen(szName)) < MAX_PATH)
            && !wcsnicmp(flname, szName, len))
        {
            wcsncpy(filename, szDrive, MAX_PATH);
            wcsncat(filename, flname + len, MAX_PATH - 1 - wcslen(filename));
            return 1;
        }
        while (*p++) {}
    } while (*p);
    return 0;
}

static int sigcheck(cli_ctx *ctx, int checkfp)
{
    fmap_t *map = *ctx->fmap;
    LONG lstatus;
    LONG lsigned = TRUST_E_NOSIGNATURE;
    HCATINFO *phCatInfo = NULL;
    CATALOG_INFO sCatInfo;
    WINTRUST_CATALOG_INFO wtci;
    WINTRUST_DATA wtd;
    BYTE bHash[20];
    /*
    int i;
    wchar_t mTag[41];
    */
    wchar_t filename[MAX_PATH + 1];
    GUID pgActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    DWORD cbHash = sizeof(bHash);

    while (ctx->container_type == CL_TYPE_ANY)
    {

        if (!cw_helpers.wt.CryptCATAdminCalcHashFromFileHandle(map->fh, &cbHash, bHash, 0))
        {
            /* cli_errmsg("sigcheck: CryptCATAdminCalcHashFromFileHandle() failed: %d\n", GetLastError()); */
            break;
        }

        if (!cw_getfnfromhandle(map->data, filename))
            break;

        /*
        for (i = 0; i < sizeof(bHash); i++)
            _snwprintf(&mTag[i * 2], 2, L"%02X", bHash[i]);
        mTag[i * 2] = 0;
        */

        memset(&sCatInfo, 0, sizeof(sCatInfo));
        sCatInfo.cbStruct = sizeof(sCatInfo);

        if (!(phCatInfo = cw_helpers.wt.CryptCATAdminEnumCatalogFromHash(cw_helpers.wt.hCatAdmin, bHash, cbHash, 0, NULL)))
            break;

        lstatus = cw_helpers.wt.CryptCATCatalogInfoFromContext(phCatInfo, &sCatInfo, 0);
        cw_helpers.wt.CryptCATAdminReleaseCatalogContext(cw_helpers.wt.hCatAdmin, phCatInfo, 0);
        if (!lstatus)
        {
            cli_errmsg("sigcheck: CryptCATCatalogInfoFromContext() failed\n");
            break;
        }

        memset(&wtci, 0, sizeof(wtci));
        wtci.cbStruct = sizeof(wtci);
        wtci.cbCalculatedFileHash = sizeof(bHash);
        wtci.pbCalculatedFileHash = bHash;
        /* wtci.pcwszMemberTag = mTag; */
        wtci.pcwszCatalogFilePath = sCatInfo.wszCatalogFile;
        wtci.pcwszMemberFilePath = filename;

        memset(&wtd, 0, sizeof(wtd));
        wtd.cbStruct = sizeof(wtd);
        wtd.dwStateAction = WTD_STATEACTION_VERIFY;
        wtd.dwUIChoice = WTD_UI_NONE;
        wtd.dwUnionChoice = WTD_CHOICE_CATALOG;
        wtd.pCatalog = &wtci;

        lsigned = cw_helpers.wt.WinVerifyTrust(0, &pgActionID, (LPVOID) &wtd);

        wtd.dwStateAction = WTD_STATEACTION_CLOSE;
        lstatus = cw_helpers.wt.WinVerifyTrust(0, &pgActionID, (LPVOID) &wtd);

        break;
    }

      if (checkfp && (lsigned == ERROR_SUCCESS))
		fwprintf(stderr, L"%s: [%S] FALSE POSITIVE FOUND\n", filename, *ctx->virname);

    return lsigned;
}

static int sigcheck_dummy(cli_ctx *ctx, int checkfp)
{
    return TRUST_E_NOSIGNATURE;
}

typedef int (__cdecl *pfn_sigcheck)(cli_ctx *ctx, int checkfp);
static pfn_sigcheck pf_sigcheck = sigcheck_dummy;

int cw_sigcheck(cli_ctx *ctx, int checkfp)
{
    return pf_sigcheck(ctx, checkfp);
}

int cw_sig_init(void)
{
    if (!(cw_helpers.wt.ok && cw_helpers.psapi.ok))
        return 1;

    if (!cw_helpers.wt.CryptCATAdminAcquireContext(&cw_helpers.wt.hCatAdmin, NULL, 0))
    {
        cli_errmsg("sigcheck: CryptCATAdminAcquireContext() failed\n");
        return 1;
    }

    pf_sigcheck = sigcheck;
    return 0;
}
