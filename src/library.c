#include "library.h"
#include "config.h"
#include "util.h"
#include "compat.h"

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

/* Everything found while scanning one Entry folder. */
/* What the scan knows about this laptop (ticket 02). */
typedef struct Laptop {
    const LaptopConfig *cfg;
    char **installs;   /* Install folders, in listed order */
    /* Files up to two folder levels inside the Install folders, built on
     * first use: Install folders in listed order, shallower files first. */
    int indexed;
    char **files;
    int file_count;
} Laptop;

typedef struct Scan {
    const Fs *fs;
    Laptop *laptop;
    Game g;
    char **links;      /* shortcuts, in the order they are tried */
    int link_count;
    char *auto_exe;    /* best-scoring executable found in the folder */
    int auto_score;
    char *cfg_exe;     /* exe= from juego.cfg, resolved but maybe missing */
    char *cfg_exe_rel; /* exe= as written, when it is a relative path */
} Scan;

static void add_link(Scan *s, const char *path, int first)
{
    s->links = xrealloc(s->links, (size_t)(s->link_count + 1) * sizeof(char *));
    if (first) {
        memmove(s->links + 1, s->links, (size_t)s->link_count * sizeof(char *));
        s->links[0] = xstrdup(path);
    } else {
        s->links[s->link_count] = xstrdup(path);
    }
    s->link_count++;
}

static void consider_exe(Scan *s, const char *full)
{
    int sc = score_exe(path_basename(full), s->g.name);
    if (!s->auto_exe || sc > s->auto_score) {
        free(s->auto_exe);
        s->auto_exe = xstrdup(full);
        s->auto_score = sc;
    }
}

static void classify(Scan *s, const char *full)
{
    Game *g = &s->g;
    const char *base = path_basename(full);
    if (path_has_ext(full, ".lnk")) {
        add_link(s, full, 0);
    } else if (path_has_ext(full, ".exe")) {
        consider_exe(s, full);
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

static void parse_meta(Scan *s)
{
    Game *g = &s->g;
    char *text = s->fs->read_text(s->fs->ctx, g->meta_path);
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
        char *st = line;
        while (*st == ' ' || *st == '\t')
            st++;
        if (!*st || *st == '#' || *st == ';')
            continue;
        char *c = strchr(st, ':');
        char *e = strchr(st, '=');
        char *sep = c;
        if (!sep || (e && e < sep))
            sep = e;
        if (!sep)
            continue;
        *sep = 0;
        char *key = st;
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

static int fs_exists(const Scan *s, const char *path)
{
    return s->fs->exists(s->fs->ctx, path);
}

/* Absolute as-is; else relative to the Entry folder, then to the folder of
 * juego.cfg. Returns the Entry-relative path when neither exists. */
static char *resolve_cfg_value(Scan *s, const char *cfg_dir, const char *val)
{
    Game *g = &s->g;
    if (path_is_absolute(val))
        return xstrdup(val);
    char *p1 = path_join(g->dir, val);
    if (fs_exists(s, p1))
        return p1;
    if (cfg_dir && _stricmp(cfg_dir, g->dir) != 0) {
        char *p2 = path_join(cfg_dir, val);
        if (fs_exists(s, p2)) {
            free(p1);
            return p2;
        }
        free(p2);
    }
    return p1;
}

static void apply_cfg_value(Scan *s, const char *cfg_dir, const char *key, const char *val)
{
    Game *g = &s->g;
    if (!*val)
        return;
    if (_stricmp(key, "nombre") == 0) {
        free(g->name);
        g->name = xstrdup(val);
    } else if (_stricmp(key, "exe") == 0) {
        free(s->cfg_exe);
        s->cfg_exe = resolve_cfg_value(s, cfg_dir, val);
        free(s->cfg_exe_rel);
        s->cfg_exe_rel = path_is_absolute(val) ? NULL : xstrdup(val);
    } else if (_stricmp(key, "shortcut") == 0) {
        char *p = resolve_cfg_value(s, cfg_dir, val);
        add_link(s, p, 1);
        free(p);
    } else if (_stricmp(key, "icon") == 0 || _stricmp(key, "portada") == 0) {
        free(g->icon);
        g->icon = resolve_cfg_value(s, cfg_dir, val);
        g->icon_score = 10000;
    } else if (_stricmp(key, "cover") == 0) {
        free(g->cover);
        g->cover = resolve_cfg_value(s, cfg_dir, val);
    } else if (_stricmp(key, "video") == 0 || _stricmp(key, "preview") == 0) {
        free(g->video);
        g->video = resolve_cfg_value(s, cfg_dir, val);
    } else if (_stricmp(key, "descripcion") == 0 || _stricmp(key, "desc") == 0) {
        free(g->desc_path);
        g->desc_path = resolve_cfg_value(s, cfg_dir, val);
    }
}

static void parse_game_cfg(Scan *s)
{
    Game *g = &s->g;
    if (!g->cfg_path)
        return;
    char *text = s->fs->read_text(s->fs->ctx, g->cfg_path);
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
        char *st = line;
        while (*st == ' ' || *st == '\t')
            st++;
        if (!*st || *st == '#' || *st == ';')
            continue;
        char *e = strchr(st, '=');
        if (!e)
            continue;
        *e = 0;
        char *key = st;
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
            apply_cfg_value(s, cfg_dir, key, val);
    }
    free(cfg_dir);
    free(text);
}

/* A growable list of names collected from Fs.list_dir. */
typedef struct Names {
    char **items;
    int count;
    int want_dirs;
} Names;

static void collect_name(void *user, const char *name, int is_dir, int is_hidden)
{
    Names *n = user;
    if (!is_dir != !n->want_dirs)
        return;
    if (n->want_dirs && is_hidden)
        return;
    n->items = xrealloc(n->items, (size_t)(n->count + 1) * sizeof(char *));
    n->items[n->count++] = xstrdup(name);
}

static Names list_names(const Fs *fs, const char *dir, int want_dirs)
{
    Names n;
    n.items = NULL;
    n.count = 0;
    n.want_dirs = want_dirs;
    fs->list_dir(fs->ctx, dir, collect_name, &n);
    return n;
}

static void names_free(Names *n)
{
    for (int i = 0; i < n->count; i++)
        free(n->items[i]);
    free(n->items);
}

static void scan_dir_files(Scan *s, const char *dir)
{
    Names files = list_names(s->fs, dir, 0);
    for (int i = 0; i < files.count; i++) {
        char *full = path_join(dir, files.items[i]);
        classify(s, full);
        free(full);
    }
    names_free(&files);
}

/* Shortcuts and executables one folder level below the Entry folder. */
static void scan_nested_launchers(Scan *s, const char *dir)
{
    Names subs = list_names(s->fs, dir, 1);
    for (int i = 0; i < subs.count; i++) {
        char *sub = path_join(dir, subs.items[i]);
        Names files = list_names(s->fs, sub, 0);
        for (int k = 0; k < files.count; k++) {
            char *full = path_join(sub, files.items[k]);
            if (path_has_ext(full, ".lnk"))
                add_link(s, full, 0);
            else if (path_has_ext(full, ".exe"))
                consider_exe(s, full);
            free(full);
        }
        names_free(&files);
        free(sub);
    }
    names_free(&subs);
}

/* A relative exe= joined to each Install folder in listed order; the first
 * that exists, or NULL. */
static char *find_in_installs(Scan *s, const char *rel)
{
    for (int i = 0; s->laptop->installs[i]; i++) {
        char *p = path_join(s->laptop->installs[i], rel);
        if (fs_exists(s, p))
            return p;
        free(p);
    }
    return NULL;
}

/* Files and folders of one directory, collected in a single listing. */
typedef struct DirListing {
    Names files;
    Names dirs;
} DirListing;

static void collect_entry(void *user, const char *name, int is_dir, int is_hidden)
{
    DirListing *d = user;
    collect_name(is_dir ? &d->dirs : &d->files, name, is_dir, is_hidden);
}

static void laptop_index_add(Laptop *l, char *path)
{
    l->files = xrealloc(l->files, (size_t)(l->file_count + 1) * sizeof(char *));
    l->files[l->file_count++] = path;
}

/* Lists every Install folder down to two levels below it, breadth first. */
static void laptop_build_index(Laptop *l, const Fs *fs)
{
    l->indexed = 1;
    for (int i = 0; l->installs[i]; i++) {
        Names level;
        level.items = xmalloc(sizeof(char *));
        level.items[0] = xstrdup(l->installs[i]);
        level.count = 1;
        for (int depth = 0; depth <= 2; depth++) {
            Names next;
            next.items = NULL;
            next.count = 0;
            for (int k = 0; k < level.count; k++) {
                DirListing d;
                memset(&d, 0, sizeof(d));
                d.dirs.want_dirs = 1;
                fs->list_dir(fs->ctx, level.items[k], collect_entry, &d);
                for (int f = 0; f < d.files.count; f++)
                    laptop_index_add(l, path_join(level.items[k], d.files.items[f]));
                for (int f = 0; depth < 2 && f < d.dirs.count; f++) {
                    next.items = xrealloc(next.items, (size_t)(next.count + 1) * sizeof(char *));
                    next.items[next.count++] = path_join(level.items[k], d.dirs.items[f]);
                }
                names_free(&d.files);
                names_free(&d.dirs);
            }
            names_free(&level);
            level = next;
        }
        names_free(&level);
    }
}

/* The executable's file name searched up to two folder levels deep inside
 * each Install folder, in listed order; the first match, or NULL. */
static char *search_installs_by_name(Scan *s, const char *rel)
{
    Laptop *l = s->laptop;
    if (!l->indexed)
        laptop_build_index(l, s->fs);
    const char *name = path_basename(rel);
    for (int i = 0; i < l->file_count; i++)
        if (_stricmp(path_basename(l->files[i]), name) == 0)
            return xstrdup(l->files[i]);
    return NULL;
}

/* Picks the Launch target, first match wins:
 *   1. the per-laptop override exe.<Entry folder name>= that exists;
 *   2. an absolute exe= that exists;
 *   3. a relative exe= in each Install folder, in listed order, then (kept
 *      from ticket 01) relative to the Entry folder or its juego.cfg;
 *   4. the file name of a relative exe= searched up to two folder levels
 *      deep inside each Install folder, in listed order;
 *   5. a shortcut whose target exists or can't be read;
 *   6. the automatic executable inside the Entry folder.
 * Returns 1 when one was found; otherwise sets *reason. */
static int resolve_launch_target(Scan *s, HiddenReason *reason)
{
    Game *g = &s->g;
    int had_info = 0;

    const char *over = laptop_config_exe_override(s->laptop->cfg, path_basename(g->dir));
    if (over) {
        had_info = 1;
        if (fs_exists(s, over)) {
            g->exe = xstrdup(over);
            return 1;
        }
    }
    if (s->cfg_exe_rel) {
        char *p = find_in_installs(s, s->cfg_exe_rel);
        if (p) {
            g->exe = p;
            return 1;
        }
    }
    if (s->cfg_exe) {
        had_info = 1;
        if (fs_exists(s, s->cfg_exe)) {
            g->exe = s->cfg_exe;
            s->cfg_exe = NULL;
            return 1;
        }
    }
    if (s->cfg_exe_rel) {
        char *p = search_installs_by_name(s, s->cfg_exe_rel);
        if (p) {
            g->exe = p;
            return 1;
        }
    }
    for (int i = 0; i < s->link_count; i++) {
        had_info = 1;
        if (!fs_exists(s, s->links[i]))
            continue;
        char *target = s->fs->shortcut_target(s->fs->ctx, s->links[i]);
        int ok = !target || fs_exists(s, target);
        free(target);
        if (ok) {
            g->shortcut = xstrdup(s->links[i]);
            return 1;
        }
    }
    if (s->auto_exe) {
        g->exe = s->auto_exe;
        g->exe_score = s->auto_score;
        s->auto_exe = NULL;
        return 1;
    }
    *reason = had_info ? HIDDEN_NOT_FOUND : HIDDEN_NO_LAUNCH_INFO;
    return 0;
}

static void game_free(Game *g)
{
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

static void scan_free(Scan *s)
{
    for (int i = 0; i < s->link_count; i++)
        free(s->links[i]);
    free(s->links);
    free(s->auto_exe);
    free(s->cfg_exe);
    free(s->cfg_exe_rel);
}

static int game_cmp(const void *a, const void *b)
{
    const Game *ga = (const Game *)a;
    const Game *gb = (const Game *)b;
    return _stricmp(ga->name, gb->name);
}

static int hidden_cmp(const void *a, const void *b)
{
    const HiddenEntry *ha = (const HiddenEntry *)a;
    const HiddenEntry *hb = (const HiddenEntry *)b;
    return _stricmp(ha->name, hb->name);
}

static void add_hidden(Library *lib, Game *g, HiddenReason reason)
{
    lib->hidden = xrealloc(lib->hidden, (size_t)(lib->hidden_count + 1) * sizeof(HiddenEntry));
    HiddenEntry *h = &lib->hidden[lib->hidden_count++];
    h->name = xstrdup(g->name);
    h->dir = xstrdup(g->dir);
    h->reason = reason;
}

Library *library_scan(const Fs *fs, const char *root, const struct LaptopConfig *cfg)
{
    Laptop laptop;
    laptop.cfg = cfg;
    laptop.installs = laptop_config_installs(cfg);
    laptop.indexed = 0;
    laptop.files = NULL;
    laptop.file_count = 0;
    Library *lib = xmalloc(sizeof(Library));
    memset(lib, 0, sizeof(*lib));
    lib->root = xstrdup(root);

    Names entries = list_names(fs, root, 1);
    for (int i = 0; i < entries.count; i++) {
        Scan s;
        memset(&s, 0, sizeof(s));
        s.fs = fs;
        s.laptop = &laptop;
        s.g.name = xstrdup(entries.items[i]);
        s.g.dir = path_join(root, entries.items[i]);
        scan_dir_files(&s, s.g.dir);
        if (!s.link_count && !s.auto_exe)
            scan_nested_launchers(&s, s.g.dir);
        if (s.g.cfg_path)
            parse_game_cfg(&s);

        HiddenReason reason;
        if (resolve_launch_target(&s, &reason)) {
            if (s.g.desc_path)
                s.g.desc_text = fs->read_text(fs->ctx, s.g.desc_path);
            if (s.g.meta_path)
                parse_meta(&s);
            lib->games = xrealloc(lib->games, (size_t)(lib->count + 1) * sizeof(Game));
            lib->games[lib->count++] = s.g;
        } else {
            add_hidden(lib, &s.g, reason);
            game_free(&s.g);
        }
        scan_free(&s);
    }
    names_free(&entries);
    laptop_config_list_free(laptop.installs);
    for (int i = 0; i < laptop.file_count; i++)
        free(laptop.files[i]);
    free(laptop.files);

    if (lib->count > 1)
        qsort(lib->games, (size_t)lib->count, sizeof(Game), game_cmp);
    if (lib->hidden_count > 1)
        qsort(lib->hidden, (size_t)lib->hidden_count, sizeof(HiddenEntry), hidden_cmp);
    return lib;
}

void library_free(Library *lib)
{
    if (!lib)
        return;
    for (int i = 0; i < lib->count; i++)
        game_free(&lib->games[i]);
    for (int i = 0; i < lib->hidden_count; i++) {
        free(lib->hidden[i].name);
        free(lib->hidden[i].dir);
    }
    free(lib->games);
    free(lib->hidden);
    free(lib->root);
    free(lib);
}
