/* Win32 side of Kiosk mode (ticket 05). XP-compatible APIs only: the
 * registry (RegCreateKeyExA, RegSetValueExA, RegDeleteValueA), Toolhelp
 * process lists, TerminateProcess/CreateProcessA for explorer.exe, and a
 * WH_KEYBOARD_LL hook on its own thread. */
#include "kiosk.h"
#include "util.h"

#include <windows.h>
#include <tlhelp32.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

/* ---- registry ------------------------------------------------------------ */

static HKEY root_of(KioskScope scope)
{
    return scope == KIOSK_MACHINE ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
}

/* The 32-bit launcher on 64-bit Windows must write the machine's real keys,
 * not the WOW64 copies; 32-bit XP ignores the flag. */
static REGSAM sam_of(KioskScope scope, REGSAM sam)
{
    return scope == KIOSK_MACHINE ? (sam | KEY_WOW64_64KEY) : sam;
}

static int set_value(KioskScope scope, const char *key, const char *name, DWORD type,
                     const BYTE *data, DWORD size)
{
    HKEY h;
    if (RegCreateKeyExA(root_of(scope), key, 0, NULL, REG_OPTION_NON_VOLATILE,
                        sam_of(scope, KEY_SET_VALUE), NULL, &h, NULL) != ERROR_SUCCESS)
        return 0;
    LONG r = RegSetValueExA(h, name, 0, type, data, size);
    RegCloseKey(h);
    return r == ERROR_SUCCESS;
}

static int w_set_dword(void *ctx, KioskScope scope, const char *key, const char *name,
                       unsigned long v)
{
    (void)ctx;
    DWORD d = (DWORD)v;
    return set_value(scope, key, name, REG_DWORD, (const BYTE *)&d, sizeof(d));
}

static int w_set_string(void *ctx, KioskScope scope, const char *key, const char *name,
                        const char *v)
{
    (void)ctx;
    return set_value(scope, key, name, REG_SZ, (const BYTE *)v, (DWORD)strlen(v) + 1);
}

static int w_remove_value(void *ctx, KioskScope scope, const char *key, const char *name)
{
    (void)ctx;
    HKEY h;
    LONG r = RegOpenKeyExA(root_of(scope), key, 0, sam_of(scope, KEY_SET_VALUE), &h);
    if (r == ERROR_FILE_NOT_FOUND)
        return 1;
    if (r != ERROR_SUCCESS) {
        /* without write access: fine if the value isn't there at all */
        if (RegOpenKeyExA(root_of(scope), key, 0, sam_of(scope, KEY_QUERY_VALUE), &h) != ERROR_SUCCESS)
            return 0;
        r = RegQueryValueExA(h, name, NULL, NULL, NULL, NULL);
        RegCloseKey(h);
        return r == ERROR_FILE_NOT_FOUND;
    }
    r = RegDeleteValueA(h, name);
    RegCloseKey(h);
    return r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND;
}

/* ---- processes ------------------------------------------------------------ */

/* Ends every other process named exe_name with exit code `code`. */
static void kill_named(const char *exe_name, UINT code)
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
        HANDLE p = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
        if (p) {
            TerminateProcess(p, code);
            CloseHandle(p);
        }
    }
    CloseHandle(snap);
}

static int shell_running(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    int found = 0;
    for (BOOL more = Process32First(snap, &pe); more && !found; more = Process32Next(snap, &pe))
        found = _stricmp(path_basename(pe.szExeFile), "explorer.exe") == 0;
    CloseHandle(snap);
    return found;
}

/* Kept closed: Windows (without AutoRestartShell=0) or a Player may start
 * explorer.exe again, so a thread closes it every couple of seconds. */
static HANDLE keeper_thread;
static HANDLE keeper_stop;

static DWORD WINAPI keeper_main(LPVOID arg)
{
    (void)arg;
    do {
        kill_named("explorer.exe", 0);
    } while (WaitForSingleObject(keeper_stop, 2000) == WAIT_TIMEOUT);
    return 0;
}

static void w_stop_shell(void *ctx)
{
    (void)ctx;
    kill_named("explorer.exe", 0);
    if (keeper_thread)
        return;
    keeper_stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (keeper_stop)
        keeper_thread = CreateThread(NULL, 0, keeper_main, NULL, 0, NULL);
}

static void w_start_shell(void *ctx)
{
    (void)ctx;
    if (keeper_thread) {
        SetEvent(keeper_stop);
        WaitForSingleObject(keeper_thread, 5000);
        CloseHandle(keeper_thread);
        CloseHandle(keeper_stop);
        keeper_thread = keeper_stop = NULL;
    }
    if (shell_running())
        return;
    char dir[MAX_PATH];
    UINT n = GetWindowsDirectoryA(dir, sizeof(dir));
    if (n == 0 || n >= sizeof(dir))
        strcpy(dir, "C:\\WINDOWS");
    char *exe = path_join(dir, "explorer.exe");
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessA(exe, NULL, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    free(exe);
}

static void w_stop_launchers(void *ctx)
{
    (void)ctx;
    char self[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, self, sizeof(self));
    if (n == 0 || n >= sizeof(self))
        return;
    kill_named(path_basename(self), KIOSK_EXIT_QUIT);
    Sleep(500); /* let the dead launcher's handles go */
}

/* ---- log ------------------------------------------------------------------ */

static void w_log(void *ctx, const char *msg)
{
    (void)ctx;
    char *path = path_join(exe_dir(), "kiosko.log");
    FILE *f = fopen(path, "ab");
    free(path);
    if (!f)
        return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    KioskTime t = {st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond};
    char line[512];
    kiosk_log_line(line, sizeof(line), &t, msg);
    fputs(line, f);
    fclose(f);
}

const KioskApi *kiosk_win(void)
{
    static const KioskApi api = {NULL, w_set_dword, w_set_string, w_remove_value,
                                 w_stop_shell, w_start_shell, w_stop_launchers, w_log};
    return &api;
}

void kiosk_win_log(const char *msg)
{
    w_log(NULL, msg);
}

/* ---- keyboard hook ---------------------------------------------------------- */

/* The hook lives on its own thread with its own message loop (as the old
 * kbdblock.exe did), so it answers at once whatever the menu is doing. */
static HANDLE kbd_thread;
static DWORD kbd_tid;
static HANDLE kbd_ready;

static LRESULT CALLBACK kbd_proc(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION)
        return 1; /* swallow the key */
    return CallNextHookEx(NULL, code, wp, lp);
}

static DWORD WINAPI kbd_main(LPVOID arg)
{
    (void)arg;
    MSG msg;
    PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE); /* create the queue */
    HHOOK hook = SetWindowsHookExA(WH_KEYBOARD_LL, kbd_proc, GetModuleHandleA(NULL), 0);
    SetEvent(kbd_ready);
    if (!hook) {
        kiosk_win_log("ERROR: no se pudo instalar el bloqueo de teclado");
        return 1;
    }
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    UnhookWindowsHookEx(hook);
    return 0;
}

void kiosk_win_keyboard(int block)
{
    if (block && !kbd_thread) {
        kbd_ready = CreateEventA(NULL, TRUE, FALSE, NULL);
        kbd_thread = CreateThread(NULL, 0, kbd_main, NULL, 0, &kbd_tid);
        if (kbd_thread && kbd_ready)
            WaitForSingleObject(kbd_ready, 2000);
        if (kbd_ready) {
            CloseHandle(kbd_ready);
            kbd_ready = NULL;
        }
    } else if (!block && kbd_thread) {
        PostThreadMessageA(kbd_tid, WM_QUIT, 0, 0);
        WaitForSingleObject(kbd_thread, 2000);
        CloseHandle(kbd_thread);
        kbd_thread = NULL;
    }
}
