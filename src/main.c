#include "app.h"
#include "util.h"

#include <SDL.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

static char *config_get(const char *key)
{
    char *cfg = path_join(exe_dir(), "cyber.cfg");
    char *txt = file_read_text(cfg);
    free(cfg);
    if (!txt)
        return NULL;

    size_t klen = strlen(key);
    char *result = NULL;
    char *line = txt;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = 0;
        char *t = trim(line);
        if (_strnicmp(t, key, klen) == 0 && t[klen] == '=') {
            char *val = trim(t + klen + 1);
            if (*val)
                result = xstrdup(val);
            break;
        }
        line = nl ? nl + 1 : NULL;
    }
    free(txt);

    if (result && !path_is_absolute(result)) {
        char *full = path_join(exe_dir(), result);
        free(result);
        result = full;
    }
    return result;
}

static char *resolve_root(int argc, char **argv)
{
    if (argc > 1 && argv[1][0])
        return xstrdup(argv[1]);

    const char *env = getenv("CYBER_GAMES");
    if (env && *env)
        return xstrdup(env);

    char *cfg = config_get("library");
    if (cfg)
        return cfg;

    return path_join(exe_dir(), "games");
}

int main(int argc, char **argv)
{
    char *root = resolve_root(argc, argv);
    int rc = app_run(root);
    free(root);
    return rc;
}
