#include "library.h"
#include "util.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int ci_contains(const char *hay, const char *needle)
{
    if (!hay || !needle || !*needle)
        return 0;
    size_t nl = strlen(needle);
    for (const char *p = hay; *p; p++) {
        size_t i = 0;
        while (i < nl && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i]))
            i++;
        if (i == nl)
            return 1;
    }
    return 0;
}

static int ci_equal(const char *a, const char *b)
{
    return _stricmp(a, b) == 0;
}

static int stem_equals(const char *filename, const char *name)
{
    char base[128];
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = 0;
    char *dot = strrchr(base, '.');
    if (dot)
        *dot = 0;
    return ci_equal(base, name);
}

static int is_image_ext(const char *path)
{
    return path_has_ext(path, ".png") || path_has_ext(path, ".bmp") ||
           path_has_ext(path, ".jpg") || path_has_ext(path, ".jpeg") ||
           path_has_ext(path, ".ico");
}

static int score_icon(const char *filename, const char *game_name)
{
    int s = 0;
    char base[256];
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = 0;
    char *dot = strrchr(base, '.');
    if (dot)
        *dot = 0;

    if (ci_equal(base, game_name))
        s += 40;
    if (ci_contains(base, "icon"))
        s += 20;
    if (ci_contains(base, "cover"))
        s += 15;
    if (ci_contains(base, "logo"))
        s += 12;
    if (ci_contains(base, "poster") || ci_contains(base, "box"))
        s += 10;
    if (path_has_ext(filename, ".ico"))
        s += 18;
    if (path_has_ext(filename, ".png"))
        s += 4;
    if (path_has_ext(filename, ".jpg") || path_has_ext(filename, ".jpeg"))
        s += 2;
    return s;
}

static int is_meta_stem(const char *filename)
{
    char base[128];
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = 0;
    char *dot = strrchr(base, '.');
    if (dot)
        *dot = 0;
    return ci_equal(base, "ficha") || ci_equal(base, "info") || ci_equal(base, "datos") ||
           ci_equal(base, "details");
}

static int is_cfg_stem(const char *filename)
{
    char base[128];
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = 0;
    char *dot = strrchr(base, '.');
    if (dot)
        *dot = 0;
    return ci_equal(base, "juego");
}

static int score_exe(const char *filename, const char *game_name)
{
    char base[256];
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = 0;
    char *dot = strrchr(base, '.');
    if (dot)
        *dot = 0;
    for (char *p = base; *p; p++)
        *p = (char)tolower((unsigned char)*p);

    int s = 0;
    char name[256];
    strncpy(name, game_name, sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    for (char *p = name; *p; p++)
        *p = (char)tolower((unsigned char)*p);

    char *tok = name;
    while (*tok) {
        while (*tok && !isalnum((unsigned char)*tok))
            tok++;
        char *end = tok;
        while (*end && isalnum((unsigned char)*end))
            end++;
        size_t len = (size_t)(end - tok);
        if (len >= 3) {
            char word[256];
            size_t n = len < sizeof(word) - 1 ? len : sizeof(word) - 1;
            memcpy(word, tok, n);
            word[n] = 0;
            if (ci_contains(base, word))
                s += 20;
        }
        tok = end;
    }
    if (ci_contains(base, "unins") || ci_contains(base, "ded") || ci_contains(base, "setup") ||
        ci_contains(base, "crash") || ci_contains(base, "update") || ci_contains(base, "config") ||
        ci_contains(base, "server"))
        s -= 60;
    if (ci_contains(base, "vulkan") || ci_contains(base, "glx") || ci_contains(base, "sdl"))
        s -= 30;
    s -= (int)(strlen(base) / 8);
    return s;
}

static void classify(Game *g, const char *full)
{
    const char *base = path_basename(full);
    if (path_has_ext(full, ".lnk")) {
        if (!g->shortcut)
            g->shortcut = xstrdup(full);
    } else if (path_has_ext(full, ".exe")) {
        int sc = score_exe(base, g->name);
        if (!g->exe || sc > g->exe_score) {
            free(g->exe);
            g->exe = xstrdup(full);
            g->exe_score = sc;
        }
    } else if (is_image_ext(full)) {
        int sc = score_icon(base, g->name);
        if (!g->icon || sc >= g->icon_score) {
            free(g->icon);
            g->icon = xstrdup(full);
            g->icon_score = sc;
        }
        if (path_has_ext(full, ".png") && stem_equals(base, "cover")) {
            free(g->cover);
            g->cover = xstrdup(full);
        }
    } else if ((path_has_ext(full, ".mpg") || path_has_ext(full, ".mpeg")) &&
               stem_equals(base, "preview")) {
        free(g->video);
        g->video = xstrdup(full);
    } else if (path_has_ext(full, ".cfg")) {
        free(g->cfg_path);
        g->cfg_path = xstrdup(full);
    } else if (path_has_ext(full, ".txt")) {
        if (is_meta_stem(base)) {
            free(g->meta_path);
            g->meta_path = xstrdup(full);
        } else {
            free(g->desc_path);
            g->desc_path = xstrdup(full);
        }
    }
}

static void add_meta(Game *g, const char *key, const char *val)
{
    g->meta_keys = xrealloc(g->meta_keys, (size_t)(g->meta_count + 1) * sizeof(char *));
    g->meta_vals = xrealloc(g->meta_vals, (size_t)(g->meta_count + 1) * sizeof(char *));
    g->meta_keys[g->meta_count] = xstrdup(key);
    g->meta_vals[g->meta_count] = xstrdup(val);
    g->meta_count++;
}

static void parse_meta(Game *g)
{
    char *text = file_read_text(g->meta_path);
    if (!text)
        return;
    char *p = text;
    while (*p) {
        char *line = p;
        char *nl = strchr(p, '\n');
        if (nl) {
            *nl = 0;
            p = nl + 1;
        } else {
            p = line + strlen(line);
        }
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\r' || line[l - 1] == ' ' || line[l - 1] == '\t'))
            line[--l] = 0;
        char *s = line;
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s || *s == '#' || *s == ';')
            continue;
        char *c = strchr(s, ':');
        char *e = strchr(s, '=');
        char *sep = c;
        if (!sep || (e && e < sep))
            sep = e;
        if (!sep)
            continue;
        *sep = 0;
        char *key = s;
        char *val = sep + 1;
        size_t kl = strlen(key);
        while (kl && (key[kl - 1] == ' ' || key[kl - 1] == '\t'))
            key[--kl] = 0;
        while (*val == ' ' || *val == '\t')
            val++;
        size_t vl = strlen(val);
        while (vl && (val[vl - 1] == ' ' || val[vl - 1] == '\t'))
            val[--vl] = 0;
        if (*key)
            add_meta(g, key, val);
    }
    free(text);
}

static char *cfg_dir_of(const char *path)
{
    char *d = xstrdup(path);
    const char *s1 = strrchr(d, '\\');
    const char *s2 = strrchr(d, '/');
    const char *s = s1 > s2 ? s1 : s2;
    if (s)
        d[s - d] = 0;
    return d;
}

static char *resolve_cfg_value(Game *g, const char *cfg_dir, const char *val)
{
    if (path_is_absolute(val))
        return xstrdup(val);
    char *p1 = path_join(g->dir, val);
    if (file_exists(p1))
        return p1;
    if (cfg_dir && _stricmp(cfg_dir, g->dir) != 0) {
        char *p2 = path_join(cfg_dir, val);
        if (file_exists(p2)) {
            free(p1);
            return p2;
        }
        free(p2);
    }
    return p1;
}

static void apply_cfg_value(Game *g, const char *cfg_dir, const char *key, const char *val)
{
    if (!*val)
        return;
    if (_stricmp(key, "nombre") == 0) {
        free(g->name);
        g->name = xstrdup(val);
    } else if (_stricmp(key, "exe") == 0) {
        free(g->exe);
        g->exe = resolve_cfg_value(g, cfg_dir, val);
        g->exe_score = 10000;
    } else if (_stricmp(key, "shortcut") == 0) {
        free(g->shortcut);
        g->shortcut = resolve_cfg_value(g, cfg_dir, val);
    } else if (_stricmp(key, "icon") == 0 || _stricmp(key, "portada") == 0) {
        free(g->icon);
        g->icon = resolve_cfg_value(g, cfg_dir, val);
        g->icon_score = 10000;
    } else if (_stricmp(key, "cover") == 0) {
        free(g->cover);
        g->cover = resolve_cfg_value(g, cfg_dir, val);
    } else if (_stricmp(key, "video") == 0 || _stricmp(key, "preview") == 0) {
        free(g->video);
        g->video = resolve_cfg_value(g, cfg_dir, val);
    } else if (_stricmp(key, "descripcion") == 0 || _stricmp(key, "desc") == 0) {
        free(g->desc_path);
        g->desc_path = resolve_cfg_value(g, cfg_dir, val);
    }
}

static void parse_game_cfg(Game *g)
{
    if (!g->cfg_path)
        return;
    char *text = file_read_text(g->cfg_path);
    if (!text)
        return;
    char *cfg_dir = cfg_dir_of(g->cfg_path);
    char *p = text;
    while (*p) {
        char *line = p;
        char *nl = strchr(p, '\n');
        if (nl) {
            *nl = 0;
            p = nl + 1;
        } else {
            p = line + strlen(line);
        }
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\r' || line[l - 1] == ' ' || line[l - 1] == '\t'))
            line[--l] = 0;
        char *s = line;
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s || *s == '#' || *s == ';')
            continue;
        char *e = strchr(s, '=');
        if (!e)
            continue;
        *e = 0;
        char *key = s;
        char *val = e + 1;
        size_t kl = strlen(key);
        while (kl && (key[kl - 1] == ' ' || key[kl - 1] == '\t'))
            key[--kl] = 0;
        while (*val == ' ' || *val == '\t')
            val++;
        size_t vl = strlen(val);
        while (vl && (val[vl - 1] == ' ' || val[vl - 1] == '\t'))
            val[--vl] = 0;
        if (*key)
            apply_cfg_value(g, cfg_dir, key, val);
    }
    free(cfg_dir);
    free(text);
}

static void scan_dir_files(Game *g, const char *dir)
{
    char *pattern = path_join(dir, "*");
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    free(pattern);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        char *full = path_join(dir, fd.cFileName);
        classify(g, full);
        free(full);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static void scan_nested_launchers(Game *g, const char *dir)
{
    char *pattern = path_join(dir, "*");
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    free(pattern);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        char *sub = path_join(dir, fd.cFileName);
        char *p2 = path_join(sub, "*");
        WIN32_FIND_DATAA fd2;
        HANDLE h2 = FindFirstFileA(p2, &fd2);
        free(p2);
        if (h2 != INVALID_HANDLE_VALUE) {
            do {
                if (fd2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                char *full = path_join(sub, fd2.cFileName);
                if (path_has_ext(full, ".lnk")) {
                    if (!g->shortcut)
                        g->shortcut = xstrdup(full);
                } else if (path_has_ext(full, ".exe")) {
                    int sc = score_exe(path_basename(full), g->name);
                    if (!g->exe || sc > g->exe_score) {
                        free(g->exe);
                        g->exe = xstrdup(full);
                        g->exe_score = sc;
                    }
                }
                free(full);
            } while (FindNextFileA(h2, &fd2));
            FindClose(h2);
        }
        free(sub);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static int game_cmp(const void *a, const void *b)
{
    const Game *ga = (const Game *)a;
    const Game *gb = (const Game *)b;
    return _stricmp(ga->name, gb->name);
}

Library *library_scan(const char *root)
{
    Library *lib = xmalloc(sizeof(Library));
    lib->root = xstrdup(root);
    lib->games = NULL;
    lib->count = 0;

    char *pattern = path_join(root, "*");
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    free(pattern);
    if (h == INVALID_HANDLE_VALUE)
        return lib;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
            continue;

        Game g;
        memset(&g, 0, sizeof(g));
        g.name = xstrdup(fd.cFileName);
        g.dir = path_join(root, fd.cFileName);
        scan_dir_files(&g, g.dir);
        if (!g.shortcut && !g.exe)
            scan_nested_launchers(&g, g.dir);

        if (g.cfg_path)
            parse_game_cfg(&g);

        if (g.shortcut && _stricmp(path_basename(g.shortcut), "game.lnk") == 0) {
            if (g.desc_path)
                g.desc_text = file_read_text(g.desc_path);
            if (g.meta_path)
                parse_meta(&g);
            lib->games = xrealloc(lib->games, (size_t)(lib->count + 1) * sizeof(Game));
            lib->games[lib->count++] = g;
        } else {
            free(g.name);
            free(g.dir);
            free(g.exe);
            free(g.icon);
            free(g.cover);
            free(g.video);
            free(g.desc_path);
            free(g.meta_path);
            free(g.cfg_path);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);

    if (lib->count > 1)
        qsort(lib->games, (size_t)lib->count, sizeof(Game), game_cmp);

    return lib;
}

void library_free(Library *lib)
{
    if (!lib)
        return;
    for (int i = 0; i < lib->count; i++) {
        Game *g = &lib->games[i];
        free(g->name);
        free(g->dir);
        free(g->shortcut);
        free(g->exe);
        free(g->icon);
        free(g->cover);
        free(g->video);
        free(g->desc_path);
        free(g->desc_text);
        free(g->meta_path);
        free(g->cfg_path);
        for (int k = 0; k < g->meta_count; k++) {
            free(g->meta_keys[k]);
            free(g->meta_vals[k]);
        }
        free(g->meta_keys);
        free(g->meta_vals);
    }
    free(lib->games);
    free(lib->root);
    free(lib);
}
