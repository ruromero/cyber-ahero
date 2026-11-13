/* The troubleshooting log's text: cyber.log lines, scan blocks, game lines. */
#include "test.h"
#include "cyberlog.h"

#include <stdlib.h>

/* ---- lines ---------------------------------------------------------------- */

static void test_log_line_is_timestamped_plain_text(void)
{
    LogTime t = {2026, 11, 3, 9, 5, 7};
    char buf[128];
    cyberlog_line(buf, sizeof(buf), &t, "Juego iniciado: Quake III");
    CHECK_STR(buf, "2026-11-03 09:05:07  Juego iniciado: Quake III\r\n");
}

static void test_log_line_is_truncated_but_still_ends_the_line(void)
{
    LogTime t = {2026, 1, 1, 0, 0, 0};
    char buf[30];
    cyberlog_line(buf, sizeof(buf), &t, "a very long message that does not fit");
    CHECK_INT(strlen(buf), sizeof(buf) - 1);
    CHECK_STR(buf + strlen(buf) - 2, "\r\n");
}

static void test_log_is_rotated_once_it_reaches_one_megabyte(void)
{
    CHECK_INT(cyberlog_rotation_due(0), 0);
    CHECK_INT(cyberlog_rotation_due(1024L * 1024L - 1), 0);
    CHECK_INT(cyberlog_rotation_due(1024L * 1024L), 1);
}

/* ---- scan block ------------------------------------------------------------ */

typedef struct Lines {
    char *text[16];
    int count;
} Lines;

static void collect(void *user, const char *msg)
{
    Lines *l = user;
    if (l->count < 16) {
        size_t n = strlen(msg) + 1;
        l->text[l->count] = malloc(n);
        memcpy(l->text[l->count++], msg, n);
    }
}

static void lines_free(Lines *l)
{
    for (int i = 0; i < l->count; i++)
        free(l->text[i]);
}

static Game available(const char *name, const char *dir, const char *exe, const char *lnk,
                      TargetRule rule, int install)
{
    Game g;
    memset(&g, 0, sizeof(g));
    g.name = (char *)name;
    g.dir = (char *)dir;
    g.exe = (char *)exe;
    g.shortcut = (char *)lnk;
    g.rule = rule;
    g.rule_install = install;
    return g;
}

static void test_scan_block_lists_each_entry_with_its_launch_target_and_rule(void)
{
    Game games[7] = {
        available("Doom", "C:\\G\\Doom", NULL, "C:\\G\\Doom\\Doom.lnk", RULE_SHORTCUT, 0),
        available("Quake III", "C:\\G\\Quake III", "D:\\Games\\Quake3\\quake3.exe", NULL,
                  RULE_EXE_INSTALL, 2),
        available("Worms", "C:\\G\\Worms", "E:\\Raro\\worms.exe", NULL, RULE_OVERRIDE, 0),
        available("Abs", "C:\\G\\Abs", "F:\\Abs\\abs.exe", NULL, RULE_EXE_ABSOLUTE, 0),
        available("Puzzle", "C:\\G\\Puzzle", "C:\\G\\Puzzle\\AP\\ap.exe", NULL, RULE_EXE_ENTRY, 0),
        available("Q2", "C:\\G\\Q2", "D:\\Games\\Quake 2\\quake2.exe", NULL, RULE_NAME_SEARCH, 0),
        available("Tetris", "C:\\G\\Tetris", "C:\\G\\Tetris\\tetris.exe", NULL, RULE_AUTOMATIC, 0),
    };
    HiddenEntry hidden[2] = {
        {"Diablo II", "C:\\G\\Diablo II", HIDDEN_NOT_FOUND, "Diablo II\\Game.exe",
         "E:\\D2\\Game.exe"},
        {"Vacio", "C:\\G\\Vacio", HIDDEN_NO_LAUNCH_INFO, NULL, NULL},
    };
    Library lib = {"C:\\G", games, 7, hidden, 2};
    Lines l;
    memset(&l, 0, sizeof(l));

    cyberlog_scan(&lib, SCAN_AFTER_GAME, collect, &l);

    CHECK_INT(l.count, 11);
    if (l.count == 11) {
        CHECK_STR(l.text[0], "Escaneo de la biblioteca (tras un juego): C:\\G");
        CHECK_STR(l.text[1], "  Disponible: Doom -> C:\\G\\Doom\\Doom.lnk (acceso directo)");
        CHECK_STR(l.text[2], "  Disponible: Quake III -> D:\\Games\\Quake3\\quake3.exe "
                             "(exe= relativo, carpeta de instalacion 2)");
        CHECK_STR(l.text[3], "  Disponible: Worms -> E:\\Raro\\worms.exe "
                             "(exe.Worms= de cyber.cfg)");
        CHECK_STR(l.text[4], "  Disponible: Abs -> F:\\Abs\\abs.exe (exe= absoluto)");
        CHECK_STR(l.text[5], "  Disponible: Puzzle -> C:\\G\\Puzzle\\AP\\ap.exe "
                             "(exe= relativo, carpeta del juego)");
        CHECK_STR(l.text[6], "  Disponible: Q2 -> D:\\Games\\Quake 2\\quake2.exe "
                             "(busqueda por nombre en las carpetas de instalacion)");
        CHECK_STR(l.text[7], "  Disponible: Tetris -> C:\\G\\Tetris\\tetris.exe "
                             "(eleccion automatica)");
        CHECK_STR(l.text[8], "  Oculto: Diablo II: instalacion no encontrada "
                             "(buscaba exe.Diablo II=E:\\D2\\Game.exe, exe=Diablo II\\Game.exe)");
        CHECK_STR(l.text[9], "  Oculto: Vacio: sin informacion para lanzarlo");
        CHECK_STR(l.text[10], "Escaneo terminado: 7 disponibles, 2 ocultos");
    }
    lines_free(&l);
}

static void test_scan_block_names_why_the_library_was_scanned(void)
{
    Library lib = {"C:\\G", NULL, 0, NULL, 0};
    Lines l;
    memset(&l, 0, sizeof(l));
    cyberlog_scan(&lib, SCAN_STARTUP, collect, &l);
    cyberlog_scan(&lib, SCAN_MANUAL, collect, &l);
    CHECK_INT(l.count, 4);
    if (l.count == 4) {
        CHECK_STR(l.text[0], "Escaneo de la biblioteca (inicio): C:\\G");
        CHECK_STR(l.text[1], "Escaneo terminado: 0 disponibles, 0 ocultos");
        CHECK_STR(l.text[2], "Escaneo de la biblioteca (reescaneo manual): C:\\G");
    }
    lines_free(&l);
}

/* ---- games ------------------------------------------------------------------ */

static void test_game_lines_name_the_exact_path_launched(void)
{
    LaunchTarget t = {"C:\\J\\Doom\\doom.exe", NULL, NULL, "C:\\J\\Doom", NULL, NULL, NULL};
    char buf[512];
    cyberlog_game(buf, sizeof(buf), LOG_GAME_STARTED, "Doom", &t);
    CHECK_STR(buf, "Juego iniciado: Doom -> C:\\J\\Doom\\doom.exe (carpeta C:\\J\\Doom)");
    cyberlog_game(buf, sizeof(buf), LOG_GAME_ENDED, "Doom", &t);
    CHECK_STR(buf, "Juego terminado: Doom -> C:\\J\\Doom\\doom.exe (carpeta C:\\J\\Doom)");
    cyberlog_game(buf, sizeof(buf), LOG_GAME_FAILED, "Doom", &t);
    CHECK_STR(buf, "ERROR: no se pudo iniciar Doom -> C:\\J\\Doom\\doom.exe (carpeta C:\\J\\Doom)");
}

static void test_game_line_shows_arguments_wait_and_mapper_helper(void)
{
    LaunchTarget t = {"C:\\J\\Worms\\worms.exe", NULL, "-nointro", "C:\\J\\Worms", "wa.exe",
                      "C:\\Pad\\JoyToKey.exe", "Worms"};
    char buf[512];
    cyberlog_game(buf, sizeof(buf), LOG_GAME_STARTED, "Worms", &t);
    CHECK_STR(buf, "Juego iniciado: Worms -> C:\\J\\Worms\\worms.exe -nointro "
                   "(carpeta C:\\J\\Worms; wait=wa.exe; con=C:\\Pad\\JoyToKey.exe Worms)");
}

static void test_game_line_of_a_shortcut_names_the_shortcut(void)
{
    LaunchTarget t = {NULL, "C:\\G\\Doom\\Doom.lnk", NULL, NULL, NULL, NULL, NULL};
    char buf[512];
    cyberlog_game(buf, sizeof(buf), LOG_GAME_STARTED, "Doom", &t);
    CHECK_STR(buf, "Juego iniciado: Doom -> C:\\G\\Doom\\Doom.lnk");
}

int main(void)
{
    RUN_TEST(test_log_line_is_timestamped_plain_text);
    RUN_TEST(test_log_line_is_truncated_but_still_ends_the_line);
    RUN_TEST(test_log_is_rotated_once_it_reaches_one_megabyte);
    RUN_TEST(test_scan_block_lists_each_entry_with_its_launch_target_and_rule);
    RUN_TEST(test_scan_block_names_why_the_library_was_scanned);
    RUN_TEST(test_game_lines_name_the_exact_path_launched);
    RUN_TEST(test_game_line_shows_arguments_wait_and_mapper_helper);
    RUN_TEST(test_game_line_of_a_shortcut_names_the_shortcut);
    return TEST_REPORT();
}
