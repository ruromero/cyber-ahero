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

/* --- One Library for every laptop (ticket 02) --- */

char **laptop_config_installs(const LaptopConfig *cfg)
{
    char **list = xmalloc(sizeof(char *));
    int n = 0;
    list[0] = NULL;
    const char *val = laptop_config_get(cfg, "installs");
    if (!val)
        return list;
    char *buf = xstrdup(val);
    char *part = buf;
    while (part) {
        char *semi = strchr(part, ';');
        if (semi)
            *semi = 0;
        char *t = trim(part);
        if (*t) {
            list = xrealloc(list, (size_t)(n + 2) * sizeof(char *));
            list[n++] = xstrdup(t);
            list[n] = NULL;
        }
        part = semi ? semi + 1 : NULL;
    }
    free(buf);
    return list;
}

void laptop_config_list_free(char **list)
{
    if (!list)
        return;
    for (int i = 0; list[i]; i++)
        free(list[i]);
    free(list);
}

const char *laptop_config_exe_override(const LaptopConfig *cfg, const char *entry_folder)
{
    char *key = xmalloc(strlen(entry_folder) + 5);
    strcpy(key, "exe.");
    strcat(key, entry_folder);
    const char *val = laptop_config_get(cfg, key);
    free(key);
    return val;
}

/* ticket 10 */
int laptop_config_attract_minutes(const LaptopConfig *cfg)
{
    const char *v = laptop_config_get(cfg, "attract");
    if (!v)
        return ATTRACT_DEFAULT_MINUTES;
    long n = 0;
    for (const char *p = v; *p; p++) {
        if (*p < '0' || *p > '9')
            return ATTRACT_DEFAULT_MINUTES;
        if (n < ATTRACT_MAX_MINUTES)
            n = n * 10 + (*p - '0');
    }
    return n > ATTRACT_MAX_MINUTES ? ATTRACT_MAX_MINUTES : (int)n;
}
