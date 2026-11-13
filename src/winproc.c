#include "winproc.h"
#include "util.h"

#include <tlhelp32.h>

#include <stdlib.h>
#include <string.h>

void winproc_each_named(const char *exe_name, WinProcVisit visit, void *user)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return;
    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    DWORD self = GetCurrentProcessId();
    for (BOOL more = Process32First(snap, &pe); more; more = Process32Next(snap, &pe)) {
        if (pe.th32ProcessID == self || _stricmp(path_basename(pe.szExeFile), exe_name) != 0)
            continue;
        if (!visit(user, pe.th32ProcessID))
            break;
    }
    CloseHandle(snap);
}

HANDLE winproc_spawn(const char *exe, const char *cmdline, const char *work_dir, DWORD *pid)
{
    char *cmd = cmdline ? xstrdup(cmdline) : NULL; /* CreateProcessA may write to it */
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    BOOL ok = CreateProcessA(exe, cmd, NULL, NULL, FALSE, 0, NULL, work_dir, &si, &pi);
    free(cmd);
    if (!ok)
        return NULL;
    CloseHandle(pi.hThread);
    if (pid)
        *pid = pi.dwProcessId;
    return pi.hProcess;
}
