/* Win32 processes for launch.c. XP-compatible APIs only: CreateProcessA and
 * Toolhelp process lists (winproc.c), ShellExecuteExA,
 * MsgWaitForMultipleObjects; for the mapper helper EnumWindows,
 * GetWindowThreadProcessId, PostMessageA and TerminateProcess. */
#include "launch.h"
#include "util.h"
#include "winproc.h"

#include <windows.h>
#include <shellapi.h>

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
    return winproc_spawn(exe, cmdline, work_dir, NULL);
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

static int open_first(void *user, DWORD pid)
{
    HANDLE *found = user;
    *found = OpenProcess(SYNCHRONIZE, FALSE, pid);
    return *found == NULL; /* keep looking only if it couldn't be opened */
}

static ProcHandle w_find_process(void *ctx, const char *exe_name)
{
    (void)ctx;
    HANDLE found = NULL;
    winproc_each_named(exe_name, open_first, &found);
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

/* --- Mapper helper --- */

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
    DWORD pid = 0;
    HANDLE proc = winproc_spawn(exe, cmdline, work_dir, &pid);
    if (!proc)
        return NULL;
    Helper *h = xmalloc(sizeof(Helper));
    h->proc = proc;
    h->pid = pid;
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
