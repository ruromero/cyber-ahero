#include "config.h"
#include "util.h"
#include "compat.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        e--;
    *e = 0;
    return s;
}

LaptopConfig *laptop_config_parse(const char *text)
{
    LaptopConfig *cfg = xmalloc(sizeof(LaptopConfig));
    memset(cfg, 0, sizeof(*cfg));
    if (!text)
        return cfg;

    char *buf = xstrdup(text);
    char *line = buf;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = 0;
        char *t = trim(line);
        char *eq = strchr(t, '=');
        if (*t && *t != '#' && *t != ';' && eq) {
            *eq = 0;
            char *key = trim(t);
            char *val = trim(eq + 1);
            if (*key && !laptop_config_get(cfg, key)) {
                cfg->keys = xrealloc(cfg->keys, (size_t)(cfg->count + 1) * sizeof(char *));
                cfg->vals = xrealloc(cfg->vals, (size_t)(cfg->count + 1) * sizeof(char *));
                cfg->keys[cfg->count] = xstrdup(key);
                cfg->vals[cfg->count] = xstrdup(val);
                cfg->count++;
            }
        }
        line = nl ? nl + 1 : NULL;
    }
    free(buf);
    return cfg;
}

void laptop_config_free(LaptopConfig *cfg)
{
    if (!cfg)
        return;
    for (int i = 0; i < cfg->count; i++) {
        free(cfg->keys[i]);
        free(cfg->vals[i]);
    }
    free(cfg->keys);
    free(cfg->vals);
    free(cfg);
}

const char *laptop_config_get(const LaptopConfig *cfg, const char *key)
{
    if (!cfg)
        return NULL;
    for (int i = 0; i < cfg->count; i++)
        if (_stricmp(cfg->keys[i], key) == 0)
            return cfg->vals[i][0] ? cfg->vals[i] : NULL;
    return NULL;
}

char *laptop_config_library_root(const LaptopConfig *cfg, const char *arg, const char *env,
                                 const char *exe_dir)
{
    if (arg && *arg)
        return xstrdup(arg);
    if (env && *env)
        return xstrdup(env);
    const char *lib = laptop_config_get(cfg, "library");
    if (lib)
        return path_is_absolute(lib) ? xstrdup(lib) : path_join(exe_dir, lib);
    return path_join(exe_dir, "games");
}
