#include "util.h"

#include <SDL.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void *xmalloc(size_t size)
{
    void *p = malloc(size ? size : 1);
    if (!p) {
        SDL_Log("Out of memory");
        exit(1);
    }
    return p;
}

void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size ? size : 1);
    if (!p) {
        SDL_Log("Out of memory");
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

int file_exists(const char *path)
{
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES;
}

static int is_valid_utf8(const unsigned char *s, size_t n)
{
    size_t i = 0;
    while (i < n) {
        unsigned char c = s[i];
        if (c < 0x80) {
            i++;
        } else if ((c >> 5) == 0x6) {
            if (i + 1 >= n || (s[i + 1] & 0xC0) != 0x80)
                return 0;
            i += 2;
        } else if ((c >> 4) == 0xE) {
            if (i + 2 >= n || (s[i + 1] & 0xC0) != 0x80 || (s[i + 2] & 0xC0) != 0x80)
                return 0;
            i += 3;
        } else if ((c >> 3) == 0x1E) {
            if (i + 3 >= n || (s[i + 1] & 0xC0) != 0x80 || (s[i + 2] & 0xC0) != 0x80 ||
                (s[i + 3] & 0xC0) != 0x80)
                return 0;
            i += 4;
        } else {
            return 0;
        }
    }
    return 1;
}

char *file_read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    unsigned char *buf = xmalloc((size_t)sz + 1);
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = 0;

    if (got >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
        memmove(buf, buf + 3, got - 3);
        got -= 3;
        buf[got] = 0;
    }

    if (is_valid_utf8(buf, got))
        return (char *)buf;

    int wn = MultiByteToWideChar(CP_ACP, 0, (char *)buf, (int)got, NULL, 0);
    if (wn <= 0)
        return (char *)buf;

    wchar_t *wbuf = xmalloc(((size_t)wn + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, (char *)buf, (int)got, wbuf, wn);
    wbuf[wn] = 0;

    int un = WideCharToMultiByte(CP_UTF8, 0, wbuf, wn, NULL, 0, NULL, NULL);
    char *out = xmalloc((size_t)un + 1);
    WideCharToMultiByte(CP_UTF8, 0, wbuf, wn, out, un, NULL, NULL);
    out[un] = 0;

    free(wbuf);
    free(buf);
    return out;
}

const char *exe_dir(void)
{
    static char buf[MAX_PATH];
    static int done = 0;
    if (!done) {
        DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
        if (n == 0 || n == MAX_PATH)
            buf[0] = 0;
        char *p = strrchr(buf, '\\');
        if (p)
            *p = 0;
        done = 1;
    }
    return buf;
}

static char *font_from_dir(const char *const *names, int n)
{
    char windir[MAX_PATH];
    if (GetWindowsDirectoryA(windir, MAX_PATH) == 0)
        return NULL;
    char *fonts = path_join(windir, "Fonts");
    char *found = NULL;
    for (int i = 0; i < n; i++) {
        char *p = path_join(fonts, names[i]);
        if (file_exists(p)) {
            found = p;
            break;
        }
        free(p);
    }
    free(fonts);
    return found;
}

static char *font_first_local(const char *const *paths, int n)
{
    for (int i = 0; i < n; i++)
        if (file_exists(paths[i]))
            return xstrdup(paths[i]);
    return NULL;
}

char *font_path_from_system(void)
{
    const char *local[] = { "assets\\fonts\\font.ttf", "assets\\fonts\\DejaVuSans.ttf",
                            "assets\\fonts\\segoeui.ttf" };
    char *p = font_first_local(local, 3);
    if (p)
        return p;

    const char *names[] = { "arial.ttf", "tahoma.ttf", "verdana.ttf", "segoeui.ttf", "calibri.ttf" };
    return font_from_dir(names, 5);
}

char *font_path_title(void)
{
    const char *local[] = { "assets\\fonts\\title.ttf", "assets\\fonts\\title.otf" };
    char *p = font_first_local(local, 2);
    if (p)
        return p;

    const char *names[] = { "lucon.ttf", "consolab.ttf", "consola.ttf", "impact.ttf",
                            "trebucbd.ttf", "segoeuib.ttf", "arialbd.ttf" };
    p = font_from_dir(names, 7);
    if (p)
        return p;
    return font_path_from_system();
}
