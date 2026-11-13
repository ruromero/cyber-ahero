#include "app.h"
#include "config.h"
#include "util.h"

#include <SDL.h>

#include <stdlib.h>

/* Reads <exe_dir>\cyber.cfg once; a missing file gives an empty config. */
static LaptopConfig *load_laptop_config(void)
{
    char *path = path_join(exe_dir(), "cyber.cfg");
    char *text = file_read_text(path);
    free(path);
    LaptopConfig *cfg = laptop_config_parse(text);
    free(text);
    return cfg;
}

int main(int argc, char **argv)
{
    LaptopConfig *cfg = load_laptop_config();
    char *root = laptop_config_library_root(cfg, argc > 1 ? argv[1] : NULL,
                                            getenv("CYBER_GAMES"), exe_dir());
    int rc = app_run(root, cfg);
    free(root);
    laptop_config_free(cfg);
    return rc;
}
