/* cyber.log next to the launcher (see cyberlog.h). XP-compatible APIs only:
 * GetLocalTime and MoveFileExA, plus the C runtime's stdio. */
#include "cyberlog.h"
#include "util.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

/* Opens cyber.log for appending, first rotating it to cyber.old.log once it
 * has grown too big. NULL when the folder can't be written. */
static FILE *log_open(void)
{
    char *path = path_join(exe_dir(), CYBERLOG_FILE);
    FILE *f = fopen(path, "ab");
    if (f && fseek(f, 0, SEEK_END) == 0 && cyberlog_rotation_due(ftell(f))) {
        fclose(f);
        char *old = path_join(exe_dir(), CYBERLOG_OLD_FILE);
        MoveFileExA(path, old, MOVEFILE_REPLACE_EXISTING);
        free(old);
        f = fopen(path, "ab");
    }
    free(path);
    return f;
}

static void write_line(void *user, const char *msg)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    LogTime t = {st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond};
    char line[4096];
    cyberlog_line(line, sizeof(line), &t, msg);
    fputs(line, (FILE *)user);
}

void cyber_log(const char *msg)
{
    FILE *f = log_open();
    if (!f)
        return;
    write_line(f, msg);
    fclose(f);
}

void cyber_log_scan(const Library *lib, ScanCause why)
{
    FILE *f = log_open();
    if (!f)
        return;
    cyberlog_scan(lib, why, write_line, f);
    fclose(f);
}
