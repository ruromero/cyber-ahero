/* Portable helpers (memory, strings, paths). No Windows or SDL headers here:
 * this file is also compiled into the native unit tests (tests/). */
#include "util.h"
#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t size)
{
    void *p = malloc(size ? size : 1);
    if (!p) {
        fputs("Out of memory\n", stderr);
        exit(1);
    }
    return p;
}

void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size ? size : 1);
    if (!p) {
        fputs("Out of memory\n", stderr);
        exit(1);
    }
    return p;
}

char *xstrdup(const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s) + 1;
    char *d = xmalloc(n);
    memcpy(d, s, n);
    return d;
}

char *xstrndup(const char *s, size_t n)
{
    char *d = xmalloc(n + 1);
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}

char *path_join(const char *a, const char *b)
{
    size_t la = strlen(a);
    size_t lb = strlen(b);
    int need_sep = (la > 0 && a[la - 1] != '\\' && a[la - 1] != '/');
    char *d = xmalloc(la + lb + 2);
    memcpy(d, a, la);
    size_t i = la;
    if (need_sep)
        d[i++] = '\\';
    memcpy(d + i, b, lb);
    d[i + lb] = 0;
    return d;
}

int path_has_ext(const char *path, const char *ext)
{
    size_t lp = strlen(path);
    size_t le = strlen(ext);
    if (lp < le)
        return 0;
    return _stricmp(path + lp - le, ext) == 0;
}

int path_is_absolute(const char *path)
{
    if (!path || !path[0])
        return 0;
    if (path[0] == '\\' || path[0] == '/')
        return 1;
    return path[1] == ':';
}

const char *path_basename(const char *path)
{
    const char *s1 = strrchr(path, '\\');
    const char *s2 = strrchr(path, '/');
    const char *s = s1 > s2 ? s1 : s2;
    return s ? s + 1 : path;
}
