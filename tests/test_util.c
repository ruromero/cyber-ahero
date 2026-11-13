/* Smoke test of the portable helpers in src/util.c; proves the harness. */
#include "test.h"
#include "util.h"

#include <stdlib.h>

static void test_path_join_adds_one_backslash(void)
{
    char *p = path_join("C:\\Juegos", "Quake3\\quake3.exe");
    CHECK_STR(p, "C:\\Juegos\\Quake3\\quake3.exe");
    free(p);

    p = path_join("C:\\Juegos\\", "quake3.exe");
    CHECK_STR(p, "C:\\Juegos\\quake3.exe");
    free(p);

    p = path_join("D:/Games/", "x.exe");
    CHECK_STR(p, "D:/Games/x.exe");
    free(p);

    p = path_join("", "x.exe");
    CHECK_STR(p, "x.exe");
    free(p);
}

static void test_path_has_ext_is_case_insensitive(void)
{
    CHECK(path_has_ext("game.LNK", ".lnk"));
    CHECK(path_has_ext("C:\\a\\Quake3.Exe", ".exe"));
    CHECK(!path_has_ext("game.lnk.txt", ".lnk"));
    CHECK(!path_has_ext("k", ".exe"));
}

static void test_path_is_absolute(void)
{
    CHECK(path_is_absolute("C:\\Juegos"));
    CHECK(path_is_absolute("\\\\server\\share"));
    CHECK(path_is_absolute("/tmp"));
    CHECK(!path_is_absolute("Quake3\\quake3.exe"));
    CHECK(!path_is_absolute(""));
    CHECK(!path_is_absolute(NULL));
}

static void test_path_basename_handles_both_separators(void)
{
    CHECK_STR(path_basename("C:\\Juegos\\Quake3\\quake3.exe"), "quake3.exe");
    CHECK_STR(path_basename("D:/Games/x.exe"), "x.exe");
    CHECK_STR(path_basename("a/b\\c.exe"), "c.exe");
    CHECK_STR(path_basename("plain.exe"), "plain.exe");
}

static void test_xstrdup_and_xstrndup(void)
{
    char *s = xstrdup("hola");
    CHECK_STR(s, "hola");
    free(s);
    CHECK_STR(xstrdup(NULL), NULL);

    s = xstrndup("quake3.exe", 6);
    CHECK_STR(s, "quake3");
    free(s);
}

int main(void)
{
    RUN_TEST(test_path_join_adds_one_backslash);
    RUN_TEST(test_path_has_ext_is_case_insensitive);
    RUN_TEST(test_path_is_absolute);
    RUN_TEST(test_path_basename_handles_both_separators);
    RUN_TEST(test_xstrdup_and_xstrndup);
    return TEST_REPORT();
}
