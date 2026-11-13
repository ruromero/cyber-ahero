#ifndef CONFIG_H
#define CONFIG_H

/* This laptop's configuration (cyber.cfg next to the launcher), read once at
 * start-up. Lines are key=value; '#' and ';' start comments; keys ignore
 * case; the first value of a key wins. */
typedef struct LaptopConfig {
    char **keys;
    char **vals;
    int count;
} LaptopConfig;

/* text may be NULL (no file): gives an empty configuration. */
LaptopConfig *laptop_config_parse(const char *text);
void laptop_config_free(LaptopConfig *cfg);

/* The value of key, or NULL when absent or empty. */
const char *laptop_config_get(const LaptopConfig *cfg, const char *key);

/* Library root: arg, else env, else library= (relative to exe_dir), else
 * exe_dir\games. Empty strings count as absent. Returns a malloc'd path. */
char *laptop_config_library_root(const LaptopConfig *cfg, const char *arg, const char *env,
                                 const char *exe_dir);

#endif
