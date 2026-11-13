/* Win32 processes for launch.c (ticket 04). XP-compatible APIs only:
 * CreateProcessA, ShellExecuteExA, CreateToolhelp32Snapshot,
 * MsgWaitForMultipleObjects; for the mapper helper (ticket 09) EnumWindows,
 * GetWindowThreadProcessId, PostMessageA and TerminateProcess. */
#include "launch.h"
#include "util.h"

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include <stdlib.h>
#include <string.h>

/* Handles every queued window message of this thread. A WM_QUIT is kept
 * and posted again once waiting is over. */
static void pump(int *quit, int *quit_code)
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            *quit = 1;
            *quit_code = (int)msg.wParam;
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

/* Blocks until h is signalled (h NULL: until timeout_ms passes), sleeping
 * between window messages. */
static void wait_pumping(HANDLE h, DWORD timeout_ms)
{
    int quit = 0, quit_code = 0;
    DWORD start = GetTickCount();
    for (;;) {
        DWORD left = INFINITE;
        if (timeout_ms != INFINITE) {
            DWORD spent = GetTickCount() - start;
            if (spent >= timeout_ms)
                break;
            left = timeout_ms - spent;
        }
        DWORD r = MsgWaitForMultipleObjects(h ? 1 : 0, h ? &h : NULL, FALSE, left, QS_ALLINPUT);
        if (h && r == WAIT_OBJECT_0)
            break;
        if (r == WAIT_TIMEOUT || r == WAIT_FAILED)
            break;
        pump(&quit, &quit_code);
    }
    if (quit)
        PostQuitMessage(quit_code);
}

static ProcHandle w_start_exe(void *ctx, const char *exe, const char *cmdline,
                              const char *work_dir)
{
    (void)ctx;
    char *cmd = xstrdup(cmdline); /* CreateProcessA may write to it */
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
    return pi.hProcess;
}

static int w_open_shell(void *ctx, const char *path, const char *args, const char *work_dir,
                        ProcHandle *proc)
{
    (void)ctx;
    SHELLEXECUTEINFOA sei;
    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "open";
    sei.lpFile = path;
    sei.lpParameters = (args && *args) ? args : NULL;
    sei.lpDirectory = work_dir;
    sei.nShow = SW_SHOWNORMAL;
    *proc = NULL;
    if (!ShellExecuteExA(&sei))
        return 0;
    *proc = sei.hProcess;
    return 1;
}

static ProcHandle w_find_process(void *ctx, const char *exe_name)
{
    (void)ctx;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return NULL;
    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    HANDLE found = NULL;
    DWORD self = GetCurrentProcessId();
    for (BOOL more = Process32First(snap, &pe); more && !found; more = Process32Next(snap, &pe)) {
        if (pe.th32ProcessID == self)
            continue;
        if (_stricmp(path_basename(pe.szExeFile), exe_name) == 0)
            found = OpenProcess(SYNCHRONIZE, FALSE, pe.th32ProcessID);
    }
    CloseHandle(snap);
    return found;
}

static void w_wait_exit(void *ctx, ProcHandle proc)
{
    (void)ctx;
    wait_pumping((HANDLE)proc, INFINITE);
}

static void w_sleep_ms(void *ctx, int ms)
{
    (void)ctx;
    wait_pumping(NULL, (DWORD)ms);
}

static uint32_t w_ticks(void *ctx)
{
    (void)ctx;
    return (uint32_t)GetTickCount();
}

static void w_close(void *ctx, ProcHandle proc)
{
    (void)ctx;
    if (proc)
        CloseHandle((HANDLE)proc);
}

/* --- Mapper helper (ticket 09) --- */

/* How long the helper gets to close on its own before it is ended. */
#define HELPER_CLOSE_MS 2000

typedef struct Helper {
    HANDLE proc;
    DWORD pid;
} Helper;

static ProcHandle w_start_helper(void *ctx, const char *exe, const char *cmdline,
                                 const char *work_dir)
{
    (void)ctx;
    char *cmd = xstrdup(cmdline);
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
    Helper *h = xmalloc(sizeof(Helper));
    h->proc = pi.hProcess;
    h->pid = pi.dwProcessId;
    return h;
}

/* Asks every top-level window of the process (hidden ones too: mappers sit
 * in the tray) to close. */
static BOOL CALLBACK close_window_of(HWND w, LPARAM pid)
{
    DWORD owner = 0;
    GetWindowThreadProcessId(w, &owner);
    if (owner == (DWORD)pid)
        PostMessageA(w, WM_CLOSE, 0, 0);
    return TRUE;
}

static void w_stop_helper(void *ctx, ProcHandle helper)
{
    (void)ctx;
    Helper *h = helper;
    if (!h)
        return;
    if (WaitForSingleObject(h->proc, 0) != WAIT_OBJECT_0) {
        EnumWindows(close_window_of, (LPARAM)h->pid);
        wait_pumping(h->proc, HELPER_CLOSE_MS);
        if (WaitForSingleObject(h->proc, 0) != WAIT_OBJECT_0) {
            TerminateProcess(h->proc, 0);
            WaitForSingleObject(h->proc, 1000);
        }
    }
    CloseHandle(h->proc);
    free(h);
}

const ProcApi *proc_win(void)
{
    static const ProcApi api = {NULL, w_start_exe, w_open_shell, w_find_process,
                                w_wait_exit, w_sleep_ms, w_ticks, w_close,
                                w_start_helper, w_stop_helper};
    return &api;
}
