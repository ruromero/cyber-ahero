#ifndef FS_H
#define FS_H

/* File-system seam used by the library scan. The app uses fs_win(); the unit
 * tests pass an in-memory fake. All paths are Windows-style strings. */

typedef void (*FsVisit)(void *user, const char *name, int is_dir, int is_hidden);

typedef struct Fs {
    void *ctx;
    /* Calls visit for every entry of dir (never "." or ".."). */
    void (*list_dir)(void *ctx, const char *dir, FsVisit visit, void *user);
    int (*exists)(void *ctx, const char *path);
    /* Whole file as UTF-8 text (malloc'd), or NULL. */
    char *(*read_text)(void *ctx, const char *path);
    /* Target path of a shortcut (.lnk), malloc'd, or NULL if it can't be read. */
    char *(*shortcut_target)(void *ctx, const char *lnk_path);
} Fs;

/* Real Windows file system (fs_win.c, app only). */
const Fs *fs_win(void);

#endif
