#ifndef WINPROC_H
#define WINPROC_H

/* Win32 process helpers shared by Kiosk mode and launching (app only).
 * XP-compatible: Toolhelp snapshots and CreateProcessA. */

#include <windows.h>

/* Called for each process found; return 0 to stop the walk. */
typedef int (*WinProcVisit)(void *user, DWORD pid);

/* Visits every running process, other than this one, whose executable file
 * name is exe_name (ignoring case). */
void winproc_each_named(const char *exe_name, WinProcVisit visit, void *user);

/* Starts exe with the full command line (may be NULL) in work_dir (NULL =
 * inherit). Returns the process handle, or NULL on failure; *pid (may be
 * NULL) gets its id. */
HANDLE winproc_spawn(const char *exe, const char *cmdline, const char *work_dir, DWORD *pid);

#endif
