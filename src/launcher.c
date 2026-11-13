#include "launcher.h"

#include <windows.h>
#include <shellapi.h>

int launcher_open(const char *shortcut_path, const char *work_dir)
{
    if (!shortcut_path)
        return -1;
    HINSTANCE r = ShellExecuteA(NULL, "open", shortcut_path, NULL, work_dir, SW_SHOWNORMAL);
    return ((INT_PTR)r > 32) ? 0 : -1;
}
