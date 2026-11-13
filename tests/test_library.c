/* Seam 1: the library scan over a fake folder tree. */
#include "test.h"
#include "fake_fs.h"
#include "library.h"
#include "config.h"

#include <stdlib.h>

#define ROOT "C:\\Cyber\\games"

static const Game *find_game(const Library *lib, const char *name)
{
    for (int i = 0; i < lib->count; i++)
        if (strcmp(lib->games[i].name, name) == 0)
            return &lib->games[i];
    return NULL;
}

static void test_shortcut_with_any_name_is_listed(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Quake III\\Quake 3 Arena.lnk", "D:\\Juegos\\Quake3\\quake3.exe");
    fake_fs_file(fs, "D:\\Juegos\\Quake3\\quake3.exe", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    const Game *g = find_game(lib, "Quake III");
    CHECK(g != NULL);
    if (g) {
        CHECK_STR(g->shortcut, ROOT "\\Quake III\\Quake 3 Arena.lnk");
        CHECK_STR(g->exe, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_shortcut_to_missing_file_is_hidden_as_not_found(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Diablo II\\game.lnk", "E:\\Viejo\\Diablo II\\Diablo II.exe");
    fake_fs_file(fs, ROOT "\\Diablo II\\cover.png", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 0);
    CHECK_INT(lib->hidden_count, 1);
    if (lib->hidden_count == 1) {
        CHECK_STR(lib->hidden[0].name, "Diablo II");
        CHECK_STR(lib->hidden[0].dir, ROOT "\\Diablo II");
        CHECK_INT(lib->hidden[0].reason, HIDDEN_NOT_FOUND);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_existing_exe_option_without_shortcut_is_listed(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Action Puzzle\\juego.cfg",
                 "# opciones\r\nexe=ActionPuzzle100\\ActionPuzzle.exe\r\n");
    fake_fs_file(fs, ROOT "\\Action Puzzle\\ActionPuzzle100\\ActionPuzzle.exe", NULL);
    fake_fs_file(fs, ROOT "\\Action Puzzle\\ActionPuzzle100\\unins000.exe", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    CHECK_INT(lib->hidden_count, 0);
    const Game *g = find_game(lib, "Action Puzzle");
    CHECK(g != NULL);
    if (g) {
        CHECK_STR(g->exe, ROOT "\\Action Puzzle\\ActionPuzzle100\\ActionPuzzle.exe");
        CHECK_STR(g->shortcut, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_absolute_exe_option_is_listed(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=D:\\Juegos\\Worms\\worms.exe\n");
    fake_fs_file(fs, "D:\\Juegos\\Worms\\worms.exe", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].exe, "D:\\Juegos\\Worms\\worms.exe");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_missing_exe_option_is_hidden_as_not_found(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=D:\\Juegos\\Worms\\worms.exe\n");
    fake_fs_file(fs, ROOT "\\Worms\\cover.png", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 0);
    CHECK_INT(lib->hidden_count, 1);
    if (lib->hidden_count == 1)
        CHECK_INT(lib->hidden[0].reason, HIDDEN_NOT_FOUND);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_existing_exe_option_wins_over_shortcut(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=D:\\Juegos\\Worms\\worms.exe\n");
    fake_fs_file(fs, "D:\\Juegos\\Worms\\worms.exe", NULL);
    fake_fs_shortcut(fs, ROOT "\\Worms\\game.lnk", "C:\\Otro\\worms.exe");
    fake_fs_file(fs, "C:\\Otro\\worms.exe", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].exe, "D:\\Juegos\\Worms\\worms.exe");
        CHECK_STR(lib->games[0].shortcut, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_folder_with_only_cover_and_details_is_hidden_as_no_launch_info(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Crazy Taxi\\cover.png", NULL);
    fake_fs_file(fs, ROOT "\\Crazy Taxi\\details.txt", "Año: 2000\n");
    fake_fs_file(fs, ROOT "\\Crazy Taxi\\preview.mpg", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 0);
    CHECK_INT(lib->hidden_count, 1);
    if (lib->hidden_count == 1) {
        CHECK_STR(lib->hidden[0].name, "Crazy Taxi");
        CHECK_INT(lib->hidden[0].reason, HIDDEN_NO_LAUNCH_INFO);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_unreadable_shortcut_is_available(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Steam Game\\Steam Game.lnk", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    CHECK_INT(lib->hidden_count, 0);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].shortcut, ROOT "\\Steam Game\\Steam Game.lnk");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_first_working_shortcut_is_chosen(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Doom\\a viejo.lnk", "E:\\Ya no\\doom.exe");
    fake_fs_shortcut(fs, ROOT "\\Doom\\b bueno.lnk", "D:\\Juegos\\Doom\\doom.exe");
    fake_fs_file(fs, "D:\\Juegos\\Doom\\doom.exe", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].shortcut, ROOT "\\Doom\\b bueno.lnk");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_executable_in_entry_subfolder_is_chosen_automatically(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Half-Life\\cover.png", NULL);
    fake_fs_file(fs, ROOT "\\Half-Life\\Half-Life\\hl.exe", NULL);
    fake_fs_file(fs, ROOT "\\Half-Life\\Half-Life\\uninstall.exe", NULL);
    fake_fs_file(fs, ROOT "\\Half-Life\\Half-Life\\Half-Life.exe", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].exe, ROOT "\\Half-Life\\Half-Life\\Half-Life.exe");
        CHECK_STR(lib->games[0].cover, ROOT "\\Half-Life\\cover.png");
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_hidden_folders_are_skipped(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_dir(fs, ROOT "\\.secreto", 1);
    fake_fs_file(fs, ROOT "\\.secreto\\algo.exe", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 0);
    CHECK_INT(lib->hidden_count, 0);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_available_entry_keeps_details_and_description(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\doom.exe", NULL);
    fake_fs_file(fs, ROOT "\\Doom\\details.txt", "Año: 1993\nGénero = Shooter\n");
    fake_fs_file(fs, ROOT "\\Doom\\descripcion.txt", "Marines en Marte.");

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        const Game *g = &lib->games[0];
        CHECK_STR(g->desc_text, "Marines en Marte.");
        CHECK_INT(g->meta_count, 2);
        if (g->meta_count == 2) {
            CHECK_STR(g->meta_keys[0], "Año");
            CHECK_STR(g->meta_vals[0], "1993");
            CHECK_STR(g->meta_keys[1], "Género");
            CHECK_STR(g->meta_vals[1], "Shooter");
        }
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_entries_and_hidden_entries_are_sorted_by_name(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\worms\\worms.exe", NULL);
    fake_fs_file(fs, ROOT "\\Age of Empires\\empires.exe", NULL);
    fake_fs_file(fs, ROOT "\\Zeta\\cover.png", NULL);
    fake_fs_file(fs, ROOT "\\beta\\cover.png", NULL);

    Library *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);

    CHECK_INT(lib->count, 2);
    CHECK_INT(lib->hidden_count, 2);
    if (lib->count == 2 && lib->hidden_count == 2) {
        CHECK_STR(lib->games[0].name, "Age of Empires");
        CHECK_STR(lib->games[1].name, "worms");
        CHECK_STR(lib->hidden[0].name, "beta");
        CHECK_STR(lib->hidden[1].name, "Zeta");
    }
    library_free(lib);
    fake_fs_free(fs);
}

/* --- One Library for every laptop --- */

#define LAPTOP "installs=C:\\Juegos;D:\\Games\n"

static Library *scan_with(FakeFs *fs, const char *laptop_cfg)
{
    LaptopConfig *cfg = laptop_config_parse(laptop_cfg);
    Library *lib = library_scan(fake_fs_iface(fs), ROOT, cfg);
    laptop_config_free(cfg);
    return lib;
}

static void test_relative_exe_is_found_in_a_later_install_folder(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "D:\\Games\\Quake3\\quake3.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].exe, "D:\\Games\\Quake3\\quake3.exe");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_first_listed_install_folder_wins(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "D:\\Games\\Quake3\\quake3.exe", NULL);
    fake_fs_file(fs, "C:\\Juegos\\Quake3\\quake3.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].exe, "C:\\Juegos\\Quake3\\quake3.exe");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_differently_named_install_is_found_by_file_name(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "D:\\Games\\Quake 3 Arena\\quake3.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].exe, "D:\\Games\\Quake 3 Arena\\quake3.exe");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_file_name_search_goes_two_levels_deep_in_listed_order(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "D:\\Games\\Q3\\quake3.exe", NULL);
    fake_fs_file(fs, "C:\\Juegos\\id\\Quake 3 Arena\\QUAKE3.EXE", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].exe, "C:\\Juegos\\id\\Quake 3 Arena\\QUAKE3.EXE");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_executable_deeper_than_two_levels_is_not_found(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "C:\\Juegos\\id\\Quake 3 Arena\\bin\\quake3.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 0);
    CHECK_INT(lib->hidden_count, 1);
    if (lib->hidden_count == 1)
        CHECK_INT(lib->hidden[0].reason, HIDDEN_NOT_FOUND);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_game_in_no_install_folder_is_hidden_as_not_found(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, ROOT "\\Quake III\\cover.png", NULL);
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 0);
    CHECK_INT(lib->hidden_count, 1);
    if (lib->hidden_count == 1) {
        CHECK_STR(lib->hidden[0].name, "Quake III");
        CHECK_INT(lib->hidden[0].reason, HIDDEN_NOT_FOUND);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_per_laptop_override_wins_over_exe_option(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "C:\\Juegos\\Quake3\\quake3.exe", NULL);
    fake_fs_file(fs, "E:\\Raro\\Q3A\\quake3.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP "exe.Quake III=E:\\Raro\\Q3A\\quake3.exe\n");

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].exe, "E:\\Raro\\Q3A\\quake3.exe");
        CHECK_STR(lib->games[0].shortcut, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_per_laptop_override_wins_over_shortcut(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Worms\\game.lnk", "C:\\Otro\\worms.exe");
    fake_fs_file(fs, "C:\\Otro\\worms.exe", NULL);
    fake_fs_file(fs, "E:\\Worms\\worms.exe", NULL);

    Library *lib = scan_with(fs, "EXE.worms = E:\\Worms\\worms.exe\n");

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].exe, "E:\\Worms\\worms.exe");
        CHECK_STR(lib->games[0].shortcut, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_absolute_exe_and_shortcuts_still_work_with_install_folders(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=F:\\Worms\\worms.exe\n");
    fake_fs_file(fs, "F:\\Worms\\worms.exe", NULL);
    fake_fs_file(fs, "C:\\Juegos\\Worms\\worms.exe", NULL);
    fake_fs_shortcut(fs, ROOT "\\Doom\\Doom.lnk", "F:\\Doom\\doom.exe");
    fake_fs_file(fs, "F:\\Doom\\doom.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 2);
    const Game *w = find_game(lib, "Worms");
    const Game *d = find_game(lib, "Doom");
    CHECK(w && d);
    if (w && d) {
        CHECK_STR(w->exe, "F:\\Worms\\worms.exe");
        CHECK_STR(d->shortcut, ROOT "\\Doom\\Doom.lnk");
    }
    library_free(lib);
    fake_fs_free(fs);
}

/* --- Launching options --- */

static void test_entry_carries_its_launch_arguments(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg",
                 "exe=Doom\\dosbox.exe\nargs=-conf doom.conf -noconsole\n");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\dosbox.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].args, "-conf doom.conf -noconsole");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_working_folder_defaults_to_the_executables_folder(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\bin\\quake3.exe\n");
    fake_fs_file(fs, "D:\\Games\\Quake3\\bin\\quake3.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].work_dir, "D:\\Games\\Quake3\\bin");
        CHECK_STR(lib->games[0].args, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_relative_working_folder_is_taken_from_the_executables_folder(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\bin\\quake3.exe\ndir=..\\data\n");
    fake_fs_file(fs, "D:\\Games\\Quake3\\bin\\quake3.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].work_dir, "D:\\Games\\Quake3\\bin\\..\\data");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_absolute_working_folder_is_used_as_is(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg", "exe=DOSBox\\dosbox.exe\ndir=C:\\Juegos\\Doom\n");
    fake_fs_file(fs, "D:\\Games\\DOSBox\\dosbox.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].work_dir, "C:\\Juegos\\Doom");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_automatic_executable_starts_in_its_own_folder(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\Juego\\worms.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].work_dir, ROOT "\\Worms\\Juego");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_shortcut_keeps_its_own_working_folder(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Doom\\Doom.lnk", "C:\\Juegos\\Doom\\doom.exe");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg", "dir=C:\\Otra\n");

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].shortcut, ROOT "\\Doom\\Doom.lnk");
        CHECK_STR(lib->games[0].work_dir, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_entry_names_the_process_to_wait_for(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Age II\\juego.cfg", "exe=AoE2\\launcher.exe\nwait=empires2.exe\n");
    fake_fs_file(fs, "C:\\Juegos\\AoE2\\launcher.exe", NULL);
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=Worms\\worms.exe\n");
    fake_fs_file(fs, "C:\\Juegos\\Worms\\worms.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 2);
    if (lib->count == 2) {
        CHECK_STR(lib->games[0].wait, "empires2.exe");
        CHECK_STR(lib->games[1].wait, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

/* --- Gamepad mapper helper --- */

static void test_entry_names_its_mapper_helper_and_its_arguments(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg",
                 "exe=Doom\\doom.exe\ncon=C:\\JoyToKey\\JoyToKey.exe\ncon_args=Doom\n");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);
    fake_fs_file(fs, "C:\\JoyToKey\\JoyToKey.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].con, "C:\\JoyToKey\\JoyToKey.exe");
        CHECK_STR(lib->games[0].con_args, "Doom");
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_relative_mapper_helper_is_found_in_the_install_folders(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg",
                 "exe=Doom\\doom.exe\ncon=JoyToKey\\JoyToKey.exe\n");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);
    fake_fs_file(fs, "D:\\Games\\JoyToKey\\JoyToKey.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].con, "D:\\Games\\JoyToKey\\JoyToKey.exe");
        CHECK_STR(lib->games[0].con_args, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_mapper_helper_inside_the_entry_folder_is_found(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg",
                 "exe=Doom\\doom.exe\ncon=mando\\JoyToKey.exe\ncon_args=Doom\n");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);
    fake_fs_file(fs, ROOT "\\Doom\\mando\\JoyToKey.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].con, ROOT "\\Doom\\mando\\JoyToKey.exe");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_differently_installed_mapper_helper_is_found_by_file_name(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg",
                 "exe=Doom\\doom.exe\ncon=JoyToKey\\JoyToKey.exe\n");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);
    fake_fs_file(fs, "D:\\Games\\Utilidades\\JoyToKey_es\\JoyToKey.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1)
        CHECK_STR(lib->games[0].con, "D:\\Games\\Utilidades\\JoyToKey_es\\JoyToKey.exe");
    library_free(lib);
    fake_fs_free(fs);
}

static void test_missing_mapper_helper_keeps_the_entry_available(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Doom\\juego.cfg",
                 "exe=Doom\\doom.exe\ncon=JoyToKey\\JoyToKey.exe\n");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    CHECK_INT(lib->hidden_count, 0);
    if (lib->count == 1) {
        CHECK_STR(lib->games[0].exe, "C:\\Juegos\\Doom\\doom.exe");
        /* kept as written next to the Entry; starting it fails at JUGAR */
        CHECK_STR(lib->games[0].con, ROOT "\\Doom\\JoyToKey\\JoyToKey.exe");
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_entry_without_mapper_helper_has_none(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=Worms\\worms.exe\ncon_args=Worms\n");
    fake_fs_file(fs, "C:\\Juegos\\Worms\\worms.exe", NULL);
    fake_fs_shortcut(fs, ROOT "\\Doom\\Doom.lnk", "C:\\Juegos\\Doom\\doom.exe");
    fake_fs_file(fs, "C:\\Juegos\\Doom\\doom.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 2);
    for (int i = 0; i < lib->count; i++)
        CHECK_STR(lib->games[i].con, NULL);
    library_free(lib);
    fake_fs_free(fs);
}

/* --- Which rule found the Launch target (troubleshooting log) --- */

static void check_rule(const char *laptop_cfg, FakeFs *fs, TargetRule rule, int install)
{
    Library *lib = scan_with(fs, laptop_cfg);
    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        CHECK_INT(lib->games[0].rule, rule);
        CHECK_INT(lib->games[0].rule_install, install);
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_override_is_reported_as_the_rule(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "E:\\Raro\\quake3.exe", NULL);
    check_rule(LAPTOP "exe.Quake III=E:\\Raro\\quake3.exe\n", fs, RULE_OVERRIDE, 0);
}

static void test_absolute_exe_is_reported_as_the_rule(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=F:\\Worms\\worms.exe\n");
    fake_fs_file(fs, "F:\\Worms\\worms.exe", NULL);
    check_rule(LAPTOP, fs, RULE_EXE_ABSOLUTE, 0);
}

static void test_install_folder_number_is_reported_for_a_relative_exe(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "D:\\Games\\Quake3\\quake3.exe", NULL);
    check_rule(LAPTOP, fs, RULE_EXE_INSTALL, 2);
}

static void test_relative_exe_in_the_entry_folder_is_reported_as_the_rule(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Action Puzzle\\juego.cfg", "exe=AP\\ActionPuzzle.exe\n");
    fake_fs_file(fs, ROOT "\\Action Puzzle\\AP\\ActionPuzzle.exe", NULL);
    check_rule(LAPTOP, fs, RULE_EXE_ENTRY, 0);
}

static void test_name_search_is_reported_as_the_rule(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, "D:\\Games\\Quake 3 Arena\\quake3.exe", NULL);
    check_rule(LAPTOP, fs, RULE_NAME_SEARCH, 0);
}

static void test_shortcut_is_reported_as_the_rule(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Doom\\Doom.lnk", "F:\\Doom\\doom.exe");
    fake_fs_file(fs, "F:\\Doom\\doom.exe", NULL);
    check_rule(LAPTOP, fs, RULE_SHORTCUT, 0);
}

static void test_automatic_choice_is_reported_as_the_rule(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\worms.exe", NULL);
    check_rule(LAPTOP, fs, RULE_AUTOMATIC, 0);
}

static void test_hidden_entry_reports_the_exe_it_looked_for(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Quake III\\juego.cfg", "exe=Quake3\\quake3.exe\n");
    fake_fs_file(fs, ROOT "\\Doom\\cover.png", NULL);

    Library *lib = scan_with(fs, LAPTOP "exe.Quake III=E:\\Raro\\quake3.exe\n");

    CHECK_INT(lib->hidden_count, 2);
    if (lib->hidden_count == 2) {
        CHECK_STR(lib->hidden[0].name, "Doom");
        CHECK_STR(lib->hidden[0].exe, NULL);
        CHECK_STR(lib->hidden[0].exe_override, NULL);
        CHECK_STR(lib->hidden[1].name, "Quake III");
        CHECK_STR(lib->hidden[1].exe, "Quake3\\quake3.exe");
        CHECK_STR(lib->hidden[1].exe_override, "E:\\Raro\\quake3.exe");
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_launch_target_of_an_entry_carries_everything_to_start_it(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg",
                 "exe=Worms\\worms.exe\nargs=-nointro\nwait=wa.exe\n"
                 "con=C:\\Pad\\JoyToKey.exe\ncon_args=Worms\n");
    fake_fs_file(fs, "C:\\Juegos\\Worms\\worms.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        LaunchTarget t = game_launch_target(&lib->games[0]);
        CHECK_STR(t.exe, "C:\\Juegos\\Worms\\worms.exe");
        CHECK_STR(t.shortcut, NULL);
        CHECK_STR(t.args, "-nointro");
        CHECK_STR(t.work_dir, "C:\\Juegos\\Worms");
        CHECK_STR(t.wait, "wa.exe");
        CHECK_STR(t.con, "C:\\Pad\\JoyToKey.exe");
        CHECK_STR(t.con_args, "Worms");
    }
    library_free(lib);
    fake_fs_free(fs);
}

static void test_launch_target_of_a_shortcut_entry_opens_the_shortcut(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_shortcut(fs, ROOT "\\Doom\\Doom.lnk", "F:\\Doom\\doom.exe");
    fake_fs_file(fs, "F:\\Doom\\doom.exe", NULL);

    Library *lib = scan_with(fs, LAPTOP);

    CHECK_INT(lib->count, 1);
    if (lib->count == 1) {
        LaunchTarget t = game_launch_target(&lib->games[0]);
        CHECK_STR(t.exe, NULL);
        CHECK_STR(t.shortcut, ROOT "\\Doom\\Doom.lnk");
        CHECK_STR(t.work_dir, NULL);
    }
    library_free(lib);
    fake_fs_free(fs);
}

int main(void)
{
    RUN_TEST(test_override_is_reported_as_the_rule);
    RUN_TEST(test_absolute_exe_is_reported_as_the_rule);
    RUN_TEST(test_install_folder_number_is_reported_for_a_relative_exe);
    RUN_TEST(test_relative_exe_in_the_entry_folder_is_reported_as_the_rule);
    RUN_TEST(test_name_search_is_reported_as_the_rule);
    RUN_TEST(test_shortcut_is_reported_as_the_rule);
    RUN_TEST(test_automatic_choice_is_reported_as_the_rule);
    RUN_TEST(test_hidden_entry_reports_the_exe_it_looked_for);
    RUN_TEST(test_launch_target_of_an_entry_carries_everything_to_start_it);
    RUN_TEST(test_launch_target_of_a_shortcut_entry_opens_the_shortcut);
    RUN_TEST(test_differently_installed_mapper_helper_is_found_by_file_name);
    RUN_TEST(test_missing_mapper_helper_keeps_the_entry_available);
    RUN_TEST(test_entry_without_mapper_helper_has_none);
    RUN_TEST(test_mapper_helper_inside_the_entry_folder_is_found);
    RUN_TEST(test_relative_mapper_helper_is_found_in_the_install_folders);
    RUN_TEST(test_entry_names_its_mapper_helper_and_its_arguments);
    RUN_TEST(test_entry_carries_its_launch_arguments);
    RUN_TEST(test_working_folder_defaults_to_the_executables_folder);
    RUN_TEST(test_relative_working_folder_is_taken_from_the_executables_folder);
    RUN_TEST(test_absolute_working_folder_is_used_as_is);
    RUN_TEST(test_automatic_executable_starts_in_its_own_folder);
    RUN_TEST(test_shortcut_keeps_its_own_working_folder);
    RUN_TEST(test_entry_names_the_process_to_wait_for);
    RUN_TEST(test_file_name_search_goes_two_levels_deep_in_listed_order);
    RUN_TEST(test_executable_deeper_than_two_levels_is_not_found);
    RUN_TEST(test_game_in_no_install_folder_is_hidden_as_not_found);
    RUN_TEST(test_per_laptop_override_wins_over_exe_option);
    RUN_TEST(test_per_laptop_override_wins_over_shortcut);
    RUN_TEST(test_absolute_exe_and_shortcuts_still_work_with_install_folders);
    RUN_TEST(test_relative_exe_is_found_in_a_later_install_folder);
    RUN_TEST(test_first_listed_install_folder_wins);
    RUN_TEST(test_differently_named_install_is_found_by_file_name);
    RUN_TEST(test_existing_exe_option_without_shortcut_is_listed);
    RUN_TEST(test_absolute_exe_option_is_listed);
    RUN_TEST(test_missing_exe_option_is_hidden_as_not_found);
    RUN_TEST(test_existing_exe_option_wins_over_shortcut);
    RUN_TEST(test_folder_with_only_cover_and_details_is_hidden_as_no_launch_info);
    RUN_TEST(test_unreadable_shortcut_is_available);
    RUN_TEST(test_first_working_shortcut_is_chosen);
    RUN_TEST(test_executable_in_entry_subfolder_is_chosen_automatically);
    RUN_TEST(test_hidden_folders_are_skipped);
    RUN_TEST(test_available_entry_keeps_details_and_description);
    RUN_TEST(test_entries_and_hidden_entries_are_sorted_by_name);
    RUN_TEST(test_shortcut_with_any_name_is_listed);
    RUN_TEST(test_shortcut_to_missing_file_is_hidden_as_not_found);
    return TEST_REPORT();
}
