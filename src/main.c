#include "app.h"
#include "config.h"
#include "cyberlog.h"
#include "kiosk.h"
#include "util.h"

#include <SDL.h>

#include <stdio.h>
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

/* Kiosk mode starts again at logon through kiosko.bat next to the launcher,
 * which restarts the launcher whenever it ends without Quit. */
static void enter_kiosk(void)
{
    char *bat = path_join(exe_dir(), "kiosko.bat");
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "\"%s\"", bat);
    free(bat);
    int failed = kiosk_enter(kiosk_win(), cmd);
    if (failed) {
        char msg[128];
        snprintf(msg, sizeof(msg), "AVISO: %d ajustes del modo kiosko no se aplicaron", failed);
        cyber_log(msg);
    }
}

int main(int argc, char **argv)
{
    CliArgs args = cli_parse(argc, argv);
    if (args.mode == MODE_RECOVER) {
        cyber_log("Recuperacion (--limpiar)");
        kiosk_recover(kiosk_win());
        return 0;
    }

    int kiosk = args.mode == MODE_KIOSK;
    if (kiosk) {
        cyber_log("Modo kiosko iniciado");
        enter_kiosk();
    } else {
        cyber_log("Launcher iniciado");
    }

    LaptopConfig *cfg = load_laptop_config();
    char *root = laptop_config_library_root(cfg, args.library, getenv("CYBER_GAMES"), exe_dir());
    int rc = app_run(root, cfg, kiosk);
    free(root);
    laptop_config_free(cfg);

    if (rc == APP_QUIT) {
        rc = 0;
        if (kiosk) {
            cyber_log("Salir: se deja el modo kiosko");
            kiosk_leave(kiosk_win());
            rc = KIOSK_EXIT_QUIT;
        } else {
            cyber_log("Launcher cerrado");
        }
    } else if (!kiosk) {
        cyber_log("Launcher cerrado");
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "El launcher termino sin Salir (codigo %d)", rc);
        cyber_log(msg);
    }
    return rc;
}
