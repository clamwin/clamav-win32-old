/*
 * NT Service Wrapper for ClamD/FreshClam
 *
 * Copyright (c) 2008 Gianluigi Tiesi <sherpya@netfarm.it>
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
#include <osdeps.h>
#include <winsvc.h>
#include <shared/output.h>

extern int gnulib_snprintf(char *str, size_t size, const char *format, ...);

void WINAPI ServiceMain(DWORD dwArgc, LPSTR *lpszArgv);

static SERVICE_STATUS svc;
static SERVICE_STATUS_HANDLE svc_handle;
static SERVICE_TABLE_ENTRYA DT[] = {{ "Service", ServiceMain }, { NULL, NULL }};

static HANDLE ServiceProc;

int cw_uninstallservice(const char *name, int verbose)
{
    SC_HANDLE sm, svc;
    int ret = 1;

    if (!(sm = OpenSCManagerA(NULL, NULL, DELETE)))
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            fprintf(stderr, "Windows Services are not supported on this Platform\n");
        else
            fprintf(stderr, "Unable to Open SCManager (%d)\n", GetLastError());
        return 0;
    }

    if ((svc = OpenServiceA(sm, name, DELETE)))
    {
        if (DeleteService(svc))
        {
            if (verbose) printf("Service %s successfully removed\n", name);
        }
        else
        {
            fprintf(stderr, "Unable to Open Service %s (%d)\n", name, GetLastError());
            ret = 0;
        }
    }
    else
    {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            if (verbose) printf("Service %s does not exist\n", name);
        }
        else
        {
            fprintf(stderr, "Unable to Open Service %s (%d)\n", name, GetLastError());
            ret = 0;
        }
    }

    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(sm);
    return ret;
}

int cw_installservice(const char *name, const char *dname, const char *desc)
{
    SC_HANDLE sm, svc;
    char modulepath[MAX_PATH];
    char binpath[MAX_PATH];
    SERVICE_DESCRIPTIONA sdesc = { (char *) desc };

    if (!GetModuleFileName(NULL, modulepath, MAX_PATH - 1))
    {
        fprintf(stderr, "Unable to get the executable name (%d)\n", GetLastError());
        return 0;
    }

    if (!cw_uninstallservice(name, 0)) return 0;

    if (!(sm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE | DELETE)))
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            fprintf(stderr, "Windows Services are not supported on this Platform\n");
        else
            fprintf(stderr, "Unable to Open SCManager (%d)\n", GetLastError());
        return 0;
    }

    if (strchr(modulepath, ' '))
        gnulib_snprintf(binpath, MAX_PATH - 1, "\"%s\" --daemon", modulepath);
    else
        gnulib_snprintf(binpath, MAX_PATH - 1, "%s --daemon", modulepath);

    svc = CreateServiceA(sm, name, dname, SERVICE_CHANGE_CONFIG,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        binpath,
        NULL, /* Load group order */
        NULL, /* Tag Id */
        NULL, /* Dependencies */
        NULL, /* User -> Local System */
        "");

    if (!svc)
    {
        fprintf(stderr, "Unable to Create Service %s (%d)\n", name, GetLastError());
        CloseServiceHandle(sm);
        return 0;
    }

    /* ChangeServiceConfig2A() */
    if (cw_helpers.av32.ChangeServiceConfig2A &&
        (!cw_helpers.av32.ChangeServiceConfig2A(svc, SERVICE_CONFIG_DESCRIPTION, &sdesc)))
        fprintf(stderr, "Unable to set description for Service %s (%d)\n", name, GetLastError());

    CloseServiceHandle(svc);
    CloseServiceHandle(sm);

    printf("Service %s successfully created\n", name);
    return 1;
}

void cw_registerservice(const char *name)
{
    DWORD tid;
    DT->lpServiceName = (char *) name;

    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &ServiceProc, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        logg("[service] DuplicateHandle() failed with %d\n", GetLastError());
        exit(1);
    }
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) StartServiceCtrlDispatcherA, (LPVOID) DT, 0, &tid);
}

void WINAPI ServiceCtrlHandler(DWORD code)
{
    switch (code)
    {
        case SERVICE_CONTROL_PAUSE:
            svc.dwCurrentState = SERVICE_PAUSED;
            break;
        case SERVICE_CONTROL_CONTINUE:
            svc.dwCurrentState = SERVICE_RUNNING;
            break;
        case SERVICE_CONTROL_STOP:
            svc.dwWin32ExitCode = 0;
            svc.dwCurrentState = SERVICE_STOPPED;
            svc.dwCheckPoint = 0;
            svc.dwWaitHint = 0;
            cw_stop_ctrl_handler(CTRL_C_EVENT);
            SetServiceStatus(svc_handle, &svc);
            break;
        case SERVICE_CONTROL_INTERROGATE:
            break;
    }
    return;
}

void WINAPI ServiceMain(DWORD dwArgc, LPSTR *lpszArgv)
{
    svc.dwServiceType = SERVICE_WIN32;
    svc.dwCurrentState = SERVICE_START_PENDING;
    svc.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    svc.dwWin32ExitCode = 0;
    svc.dwServiceSpecificExitCode = 0;
    svc.dwCheckPoint = 0;
    svc.dwWaitHint = 0;

    if (!(svc_handle = RegisterServiceCtrlHandlerA(DT->lpServiceName, ServiceCtrlHandler)))
    {
        logg("[service] RegisterServiceCtrlHandler() failed with %d\n", GetLastError());
        exit(1);
    }

    svc.dwCurrentState = SERVICE_RUNNING;
    svc.dwCheckPoint = 0;
    svc.dwWaitHint = 0;

    if (!SetServiceStatus(svc_handle, &svc))
    {
        logg("[service] SetServiceStatus() failed with %d\n", GetLastError());
        exit(1);
    }

    WaitForSingleObject(ServiceProc, INFINITE);
}
