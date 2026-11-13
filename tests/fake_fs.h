#ifndef FAKE_FS_H
#define FAKE_FS_H

#include "fs.h"

/* In-memory folder tree for tests. Folders exist implicitly as parents of the
 * files added; paths are compared case-insensitively, separator '\'. */
typedef struct FakeFs FakeFs;

FakeFs *fake_fs_new(void);
void fake_fs_free(FakeFs *f);
/* The Fs interface backed by f (valid while f lives). */
const Fs *fake_fs_iface(FakeFs *f);

void fake_fs_file(FakeFs *f, const char *path, const char *text);
/* A .lnk file; target NULL means the shortcut can't be read. */
void fake_fs_shortcut(FakeFs *f, const char *path, const char *target);
/* An empty folder; hidden marks it with the hidden attribute. */
void fake_fs_dir(FakeFs *f, const char *path, int hidden);

#endif
