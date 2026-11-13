/* The laptop configuration (cyber.cfg) and the choice of Library root. */
#include "test.h"
#include "config.h"

#include <stdlib.h>

#define EXE_DIR "C:\\Cyber"

static void test_values_are_read_by_key_ignoring_case_and_spaces(void)
{
    LaptopConfig *cfg = laptop_config_parse("# comentario\r\n  Library = D:\\Juegos  \r\nother=1\n");
    CHECK_STR(laptop_config_get(cfg, "library"), "D:\\Juegos");
    CHECK_STR(laptop_config_get(cfg, "OTHER"), "1");
    CHECK_STR(laptop_config_get(cfg, "missing"), NULL);
    laptop_config_free(cfg);
}

static void test_missing_file_gives_empty_configuration(void)
{
    LaptopConfig *cfg = laptop_config_parse(NULL);
    CHECK_STR(laptop_config_get(cfg, "library"), NULL);
    laptop_config_free(cfg);
}

static void test_first_value_of_a_key_wins(void)
{
    LaptopConfig *cfg = laptop_config_parse("library=A\nlibrary=B\n");
    CHECK_STR(laptop_config_get(cfg, "library"), "A");
    laptop_config_free(cfg);
}

static void test_library_root_prefers_argument(void)
{
    LaptopConfig *cfg = laptop_config_parse("library=D:\\Juegos\n");
    char *root = laptop_config_library_root(cfg, "E:\\Arg", "F:\\Env", EXE_DIR);
    CHECK_STR(root, "E:\\Arg");
    free(root);
    laptop_config_free(cfg);
}

static void test_library_root_then_environment(void)
{
    LaptopConfig *cfg = laptop_config_parse("library=D:\\Juegos\n");
    char *root = laptop_config_library_root(cfg, "", "F:\\Env", EXE_DIR);
    CHECK_STR(root, "F:\\Env");
    free(root);
    laptop_config_free(cfg);
}

static void test_library_root_then_configuration_relative_to_exe_dir(void)
{
    LaptopConfig *cfg = laptop_config_parse("library=biblio\n");
    char *root = laptop_config_library_root(cfg, NULL, NULL, EXE_DIR);
    CHECK_STR(root, EXE_DIR "\\biblio");
    free(root);
    laptop_config_free(cfg);

    cfg = laptop_config_parse("library=D:\\Juegos\n");
    root = laptop_config_library_root(cfg, NULL, "", EXE_DIR);
    CHECK_STR(root, "D:\\Juegos");
    free(root);
    laptop_config_free(cfg);
}

static void test_library_root_defaults_to_games_next_to_exe(void)
{
    LaptopConfig *cfg = laptop_config_parse("library=\n");
    char *root = laptop_config_library_root(cfg, NULL, NULL, EXE_DIR);
    CHECK_STR(root, EXE_DIR "\\games");
    free(root);
    laptop_config_free(cfg);
}

int main(void)
{
    RUN_TEST(test_values_are_read_by_key_ignoring_case_and_spaces);
    RUN_TEST(test_missing_file_gives_empty_configuration);
    RUN_TEST(test_first_value_of_a_key_wins);
    RUN_TEST(test_library_root_prefers_argument);
    RUN_TEST(test_library_root_then_environment);
    RUN_TEST(test_library_root_then_configuration_relative_to_exe_dir);
    RUN_TEST(test_library_root_defaults_to_games_next_to_exe);
    return TEST_REPORT();
}
