/* The Organizer report: the rows the Organizer view shows for a scanned
 * Library (through Seam 1, the library scan over a fake folder tree). */
#include "test.h"
#include "fake_fs.h"
#include "library.h"
#include "organizer.h"

#include <stdio.h>

#define ROOT "C:\\Cyber\\games"

static const OrganizerRow *find_row(const OrganizerReport *r, const char *name)
{
    for (int i = 0; i < r->count; i++)
        if (strcmp(r->rows[i].name, name) == 0)
            return &r->rows[i];
    return NULL;
}

/* An Entry with exe=D:\Juegos\ok.exe, cover, preview and details. */
static void complete_entry(FakeFs *fs, const char *name)
{
    char p[256];
    snprintf(p, sizeof(p), ROOT "\\%s\\juego.cfg", name);
    fake_fs_file(fs, p, "exe=D:\\Juegos\\ok.exe\n");
    snprintf(p, sizeof(p), ROOT "\\%s\\cover.png", name);
    fake_fs_file(fs, p, NULL);
    snprintf(p, sizeof(p), ROOT "\\%s\\preview.mpg", name);
    fake_fs_file(fs, p, NULL);
    snprintf(p, sizeof(p), ROOT "\\%s\\details.txt", name);
    fake_fs_file(fs, p, "Año: 1999\n");
}

static OrganizerReport *report_for(FakeFs *fs, Library **lib)
{
    *lib = library_scan(fake_fs_iface(fs), ROOT, NULL);
    return organizer_report(*lib);
}

static void test_hidden_entries_are_listed_with_their_reason(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Crazy Taxi\\cover.png", NULL);
    fake_fs_shortcut(fs, ROOT "\\Diablo II\\game.lnk", "E:\\Viejo\\Diablo II.exe");
    Library *lib;
    OrganizerReport *r = report_for(fs, &lib);

    CHECK_INT(r->hidden_count, 2);
    const OrganizerRow *taxi = find_row(r, "Crazy Taxi");
    const OrganizerRow *diablo = find_row(r, "Diablo II");
    CHECK(taxi != NULL && diablo != NULL);
    if (taxi && diablo) {
        CHECK_INT(taxi->kind, ORGANIZER_HIDDEN);
        CHECK_STR(taxi->text, "Oculto: sin información para lanzarlo");
        CHECK_INT(diablo->kind, ORGANIZER_HIDDEN);
        CHECK_STR(diablo->text, "Oculto: instalación no encontrada");
    }
    organizer_report_free(r);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_available_entry_missing_cover_preview_and_details_lists_all_three(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=D:\\Juegos\\ok.exe\n");
    fake_fs_file(fs, "D:\\Juegos\\ok.exe", NULL);
    Library *lib;
    OrganizerReport *r = report_for(fs, &lib);

    CHECK_INT(r->hidden_count, 0);
    CHECK_INT(r->incomplete_count, 1);
    const OrganizerRow *w = find_row(r, "Worms");
    CHECK(w != NULL);
    if (w) {
        CHECK_INT(w->kind, ORGANIZER_INCOMPLETE);
        CHECK_INT(w->missing, ORGANIZER_NO_COVER | ORGANIZER_NO_PREVIEW | ORGANIZER_NO_DETAILS);
        CHECK_STR(w->text, "Falta: portada, vista previa, ficha");
    }
    organizer_report_free(r);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_only_what_is_missing_is_named(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, "D:\\Juegos\\ok.exe", NULL);
    fake_fs_file(fs, ROOT "\\Worms\\juego.cfg", "exe=D:\\Juegos\\ok.exe\n");
    fake_fs_file(fs, ROOT "\\Worms\\cover.png", NULL);
    fake_fs_file(fs, ROOT "\\Worms\\details.txt", "Año: 1995\n");
    Library *lib;
    OrganizerReport *r = report_for(fs, &lib);

    const OrganizerRow *w = find_row(r, "Worms");
    CHECK(w != NULL);
    if (w) {
        CHECK_INT(w->missing, ORGANIZER_NO_PREVIEW);
        CHECK_STR(w->text, "Falta: vista previa");
    }
    organizer_report_free(r);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_complete_entries_are_not_listed(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, "D:\\Juegos\\ok.exe", NULL);
    complete_entry(fs, "Quake III");
    complete_entry(fs, "Doom");
    Library *lib;
    OrganizerReport *r = report_for(fs, &lib);

    CHECK_INT(r->available_count, 2);
    CHECK_INT(r->count, 0);
    organizer_report_free(r);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_hidden_rows_come_first_then_incomplete_by_name(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_file(fs, "D:\\Juegos\\ok.exe", NULL);
    fake_fs_file(fs, ROOT "\\Blood\\juego.cfg", "exe=D:\\Juegos\\ok.exe\n");
    fake_fs_file(fs, ROOT "\\Zork\\cover.png", NULL);
    fake_fs_file(fs, ROOT "\\Age of Empires\\juego.cfg", "exe=D:\\Juegos\\ok.exe\n");
    fake_fs_file(fs, ROOT "\\Myst\\juego.cfg", "exe=E:\\no\\myst.exe\n");
    Library *lib;
    OrganizerReport *r = report_for(fs, &lib);

    CHECK_INT(r->count, 4);
    if (r->count == 4) {
        CHECK_STR(r->rows[0].name, "Myst");
        CHECK_STR(r->rows[1].name, "Zork");
        CHECK_STR(r->rows[2].name, "Age of Empires");
        CHECK_STR(r->rows[3].name, "Blood");
    }
    organizer_report_free(r);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_installing_a_game_then_rescanning_makes_it_available(void)
{
    FakeFs *fs = fake_fs_new();
    complete_entry(fs, "Quake III");
    Library *lib;
    OrganizerReport *r = report_for(fs, &lib);
    CHECK_INT(r->hidden_count, 1);
    CHECK_INT(r->available_count, 0);
    organizer_report_free(r);
    library_free(lib);

    fake_fs_file(fs, "D:\\Juegos\\ok.exe", NULL); /* the game gets installed */
    r = report_for(fs, &lib);
    CHECK_INT(r->hidden_count, 0);
    CHECK_INT(r->available_count, 1);
    CHECK(find_row(r, "Quake III") == NULL);
    organizer_report_free(r);
    library_free(lib);
    fake_fs_free(fs);
}

static void test_empty_library_gives_empty_report(void)
{
    FakeFs *fs = fake_fs_new();
    fake_fs_dir(fs, ROOT, 0);
    Library *lib;
    OrganizerReport *r = report_for(fs, &lib);
    CHECK_INT(r->count, 0);
    CHECK_INT(r->hidden_count, 0);
    CHECK_INT(r->incomplete_count, 0);
    CHECK_INT(r->available_count, 0);
    organizer_report_free(r);
    library_free(lib);
    fake_fs_free(fs);
}

int main(void)
{
    RUN_TEST(test_hidden_entries_are_listed_with_their_reason);
    RUN_TEST(test_available_entry_missing_cover_preview_and_details_lists_all_three);
    RUN_TEST(test_only_what_is_missing_is_named);
    RUN_TEST(test_complete_entries_are_not_listed);
    RUN_TEST(test_hidden_rows_come_first_then_incomplete_by_name);
    RUN_TEST(test_installing_a_game_then_rescanning_makes_it_available);
    RUN_TEST(test_empty_library_gives_empty_report);
    return TEST_REPORT();
}
