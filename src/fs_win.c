/* The Fs seam over the real Windows file system (XP-compatible APIs only). */
#define COBJMACROS
#include "fs.h"
#include "util.h"

#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <stdlib.h>
#include <string.h>

static void win_list_dir(void *ctx, const char *dir, FsVisit visit, void *user)
{
    (void)ctx;
    char *pattern = path_join(dir, "*");
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    free(pattern);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        visit(user, fd.cFileName, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0,
              (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static int win_exists(void *ctx, const char *path)
{
    (void)ctx;
    return file_exists(path);
}

static char *win_read_text(void *ctx, const char *path)
{
    (void)ctx;
    return file_read_text(path);
}

/* Reads the target path stored in a .lnk through IShellLink (no Resolve, so
 * it never searches the disk or shows UI). NULL when it can't be read, e.g.
 * shortcuts to shell items with no file path. */
static char *win_shortcut_target(void *ctx, const char *lnk_path)
{
    (void)ctx;
    WCHAR wpath[MAX_PATH];
    if (!MultiByteToWideChar(CP_ACP, 0, lnk_path, -1, wpath, MAX_PATH))
        return NULL;

    HRESULT init = CoInitialize(NULL);
    char *result = NULL;
    IShellLinkA *link = NULL;
    if (SUCCEEDED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                   &IID_IShellLinkA, (void **)&link))) {
        IPersistFile *file = NULL;
        if (SUCCEEDED(IShellLinkA_QueryInterface(link, &IID_IPersistFile, (void **)&file))) {
            if (SUCCEEDED(IPersistFile_Load(file, wpath, STGM_READ))) {
                char target[MAX_PATH];
                target[0] = 0;
                if (IShellLinkA_GetPath(link, target, MAX_PATH, NULL, 0) == S_OK && target[0])
                    result = xstrdup(target);
            }
            IPersistFile_Release(file);
        }
        IShellLinkA_Release(link);
    }
    if (SUCCEEDED(init))
        CoUninitialize();
    return result;
}

static const Fs win_fs = {NULL, win_list_dir, win_exists, win_read_text, win_shortcut_target};

const Fs *fs_win(void)
{
    return &win_fs;
}
