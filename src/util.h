#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);

char *path_join(const char *a, const char *b);
int path_has_ext(const char *path, const char *ext);
int path_is_absolute(const char *path);
const char *path_basename(const char *path);

int file_exists(const char *path);
char *file_read_text(const char *path);
char *font_path_from_system(void);
char *font_path_title(void);
const char *exe_dir(void);

#endif
