#include "fake_fs.h"
#include "util.h"
#include "compat.h"

#include <stdlib.h>
#include <string.h>

typedef struct FakeNode {
    char *path;
    char *text;
    char *target;
    int is_dir;
    int hidden;
    int is_link;
} FakeNode;

struct FakeFs {
    Fs iface;
    FakeNode *nodes;
    int count;
};

static FakeNode *add_node(FakeFs *f, const char *path)
{
    f->nodes = xrealloc(f->nodes, (size_t)(f->count + 1) * sizeof(FakeNode));
    FakeNode *n = &f->nodes[f->count++];
    memset(n, 0, sizeof(*n));
    n->path = xstrdup(path);
    return n;
}

void fake_fs_file(FakeFs *f, const char *path, const char *text)
{
    add_node(f, path)->text = xstrdup(text ? text : "");
}

void fake_fs_shortcut(FakeFs *f, const char *path, const char *target)
{
    FakeNode *n = add_node(f, path);
    n->is_link = 1;
    n->target = xstrdup(target);
}

void fake_fs_dir(FakeFs *f, const char *path, int hidden)
{
    FakeNode *n = add_node(f, path);
    n->is_dir = 1;
    n->hidden = hidden;
}

/* Length of the prefix "dir\" of p, or 0 if p isn't strictly under dir. */
static size_t under(const char *p, const char *dir)
{
    size_t dl = strlen(dir);
    while (dl && dir[dl - 1] == '\\')
        dl--;
    if (_strnicmp(p, dir, dl) != 0 || p[dl] != '\\' || !p[dl + 1])
        return 0;
    return dl + 1;
}

static int is_hidden_dir(FakeFs *f, const char *path, size_t len)
{
    for (int i = 0; i < f->count; i++)
        if (f->nodes[i].is_dir && f->nodes[i].hidden && strlen(f->nodes[i].path) == len &&
            _strnicmp(f->nodes[i].path, path, len) == 0)
            return 1;
    return 0;
}

static void fake_list_dir(void *ctx, const char *dir, FsVisit visit, void *user)
{
    FakeFs *f = ctx;
    char **seen = NULL;
    int nseen = 0;
    for (int i = 0; i < f->count; i++) {
        const char *p = f->nodes[i].path;
        size_t off = under(p, dir);
        if (!off)
            continue;
        const char *rest = p + off;
        const char *sep = strchr(rest, '\\');
        size_t nl = sep ? (size_t)(sep - rest) : strlen(rest);
        char *name = xstrndup(rest, nl);
        int dup = 0;
        for (int k = 0; k < nseen; k++)
            if (_stricmp(seen[k], name) == 0)
                dup = 1;
        if (dup) {
            free(name);
            continue;
        }
        seen = xrealloc(seen, (size_t)(nseen + 1) * sizeof(char *));
        seen[nseen++] = name;
        int is_dir = sep != NULL || f->nodes[i].is_dir;
        visit(user, name, is_dir, is_dir && is_hidden_dir(f, p, off + nl));
    }
    for (int k = 0; k < nseen; k++)
        free(seen[k]);
    free(seen);
}

static FakeNode *find(FakeFs *f, const char *path)
{
    for (int i = 0; i < f->count; i++)
        if (_stricmp(f->nodes[i].path, path) == 0)
            return &f->nodes[i];
    return NULL;
}

static int fake_exists(void *ctx, const char *path)
{
    FakeFs *f = ctx;
    if (find(f, path))
        return 1;
    for (int i = 0; i < f->count; i++)
        if (under(f->nodes[i].path, path))
            return 1;
    return 0;
}

static char *fake_read_text(void *ctx, const char *path)
{
    FakeNode *n = find(ctx, path);
    return n && n->text ? xstrdup(n->text) : NULL;
}

static char *fake_shortcut_target(void *ctx, const char *lnk)
{
    FakeNode *n = find(ctx, lnk);
    return n && n->is_link && n->target ? xstrdup(n->target) : NULL;
}

FakeFs *fake_fs_new(void)
{
    FakeFs *f = xmalloc(sizeof(FakeFs));
    memset(f, 0, sizeof(*f));
    f->iface.ctx = f;
    f->iface.list_dir = fake_list_dir;
    f->iface.exists = fake_exists;
    f->iface.read_text = fake_read_text;
    f->iface.shortcut_target = fake_shortcut_target;
    return f;
}

const Fs *fake_fs_iface(FakeFs *f)
{
    return &f->iface;
}

void fake_fs_free(FakeFs *f)
{
    for (int i = 0; i < f->count; i++) {
        free(f->nodes[i].path);
        free(f->nodes[i].text);
        free(f->nodes[i].target);
    }
    free(f->nodes);
    free(f);
}
