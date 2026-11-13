/* Starting a game and waiting for it, through a fake process API. */
#include "test.h"
#include "launch.h"

#include <stdlib.h>

/* Handles are small integers cast to pointers; 0 is "none". */
#define H(n) ((ProcHandle)(intptr_t)(n))

typedef struct Fake {
    int start_ok;            /* start_exe / open_shell succeed */
    int shell_gives_proc;    /* open_shell returns a process */
    const char *real_name;   /* process find_process can find... */
    uint32_t real_appears;   /* ...from this tick on */
    uint32_t now;

    char exe[260], cmdline[512], work_dir[260], shell_path[260], shell_args[260];
    int shell_args_null;
    int started_calls;       /* on_started */
    int waited[8];
    int wait_count;
    int closed[8];
    int close_count;
    char searched[64];
    int events[16];          /* 'S' started, 'W' wait, in order */
    int event_count;
    /* mapper helper */
    int helper_ok;           /* start_helper succeeds */
    char helper_exe[260], helper_cmdline[512], helper_dir[260];
    int helper_starts, helper_stops;
} Fake;

static void copy(char *dst, size_t n, const char *s)
{
    if (!s)
        s = "(null)";
    strncpy(dst, s, n - 1);
    dst[n - 1] = 0;
}

static void event(Fake *f, int e)
{
    if (f->event_count < 16)
        f->events[f->event_count++] = e;
}

static ProcHandle f_start_exe(void *ctx, const char *exe, const char *cmdline, const char *wd)
{
    Fake *f = ctx;
    copy(f->exe, sizeof(f->exe), exe);
    copy(f->cmdline, sizeof(f->cmdline), cmdline);
    copy(f->work_dir, sizeof(f->work_dir), wd);
    event(f, 'E');
    return f->start_ok ? H(1) : NULL;
}

static int f_open_shell(void *ctx, const char *path, const char *args, const char *wd,
                        ProcHandle *proc)
{
    Fake *f = ctx;
    copy(f->shell_path, sizeof(f->shell_path), path);
    copy(f->shell_args, sizeof(f->shell_args), args);
    copy(f->work_dir, sizeof(f->work_dir), wd);
    *proc = (f->start_ok && f->shell_gives_proc) ? H(1) : NULL;
    return f->start_ok;
}

static ProcHandle f_find_process(void *ctx, const char *name)
{
    Fake *f = ctx;
    copy(f->searched, sizeof(f->searched), name);
    if (f->real_name && strcmp(name, f->real_name) == 0 && f->now >= f->real_appears)
        return H(2);
    return NULL;
}

static void f_wait_exit(void *ctx, ProcHandle p)
{
    Fake *f = ctx;
    if (f->wait_count < 8)
        f->waited[f->wait_count++] = (int)(intptr_t)p;
    if (f->event_count < 16)
        f->events[f->event_count++] = 'W';
}

static void f_sleep_ms(void *ctx, int ms)
{
    Fake *f = ctx;
    f->now += (uint32_t)ms;
}

static uint32_t f_ticks(void *ctx)
{
    return ((Fake *)ctx)->now;
}

static void f_close(void *ctx, ProcHandle p)
{
    Fake *f = ctx;
    if (f->close_count < 8)
        f->closed[f->close_count++] = (int)(intptr_t)p;
}

static ProcHandle f_start_helper(void *ctx, const char *exe, const char *cmdline, const char *wd)
{
    Fake *f = ctx;
    copy(f->helper_exe, sizeof(f->helper_exe), exe);
    copy(f->helper_cmdline, sizeof(f->helper_cmdline), cmdline);
    copy(f->helper_dir, sizeof(f->helper_dir), wd);
    f->helper_starts++;
    event(f, 'H');
    return f->helper_ok ? H(7) : NULL;
}

static void f_stop_helper(void *ctx, ProcHandle p)
{
    Fake *f = ctx;
    if ((int)(intptr_t)p == 7)
        f->helper_stops++;
    event(f, 'X');
}

static void on_started(void *user)
{
    Fake *f = user;
    f->started_calls++;
    if (f->event_count < 16)
        f->events[f->event_count++] = 'S';
}

static ProcApi fake_api(Fake *f)
{
    ProcApi api = {f, f_start_exe, f_open_shell, f_find_process, f_wait_exit,
                   f_sleep_ms, f_ticks, f_close, f_start_helper, f_stop_helper};
    return api;
}

static Fake *fake_new(void)
{
    Fake *f = calloc(1, sizeof(Fake));
    f->start_ok = 1;
    f->shell_gives_proc = 1;
    f->now = 1000;
    f->helper_ok = 1;
    return f;
}

static void test_executable_gets_its_arguments_and_working_folder(void)
{
    Fake *f = fake_new();
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\Juegos\\Quake3\\quake3.exe", NULL, "+set r_fullscreen 1",
                      "C:\\Juegos\\Quake3", NULL, NULL, NULL};

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK_STR(f->exe, "C:\\Juegos\\Quake3\\quake3.exe");
    CHECK_STR(f->cmdline, "\"C:\\Juegos\\Quake3\\quake3.exe\" +set r_fullscreen 1");
    CHECK_STR(f->work_dir, "C:\\Juegos\\Quake3");
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->waited[0], 1);
    CHECK_INT(f->close_count, 1);
    free(f);
}

static void test_executable_without_arguments_gets_only_its_quoted_path(void)
{
    char *c = launch_command_line("D:\\Mis Juegos\\worms.exe", NULL);
    CHECK_STR(c, "\"D:\\Mis Juegos\\worms.exe\"");
    free(c);
    c = launch_command_line("D:\\worms.exe", "");
    CHECK_STR(c, "\"D:\\worms.exe\"");
    free(c);
}

static void test_menu_hides_only_after_the_game_started_and_before_waiting(void)
{
    Fake *f = fake_new();
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\g.exe", NULL, NULL, "C:\\", NULL, NULL, NULL};

    launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(f->started_calls, 1);
    CHECK_INT(f->event_count, 3);
    CHECK_INT(f->events[0], 'E');
    CHECK_INT(f->events[1], 'S');
    CHECK_INT(f->events[2], 'W');
    free(f);
}

static void test_game_that_cannot_start_fails_without_waiting(void)
{
    Fake *f = fake_new();
    f->start_ok = 0;
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\g.exe", NULL, NULL, "C:\\", "real.exe", NULL, NULL};

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(r, LAUNCH_FAILED);
    CHECK_INT(f->started_calls, 0);
    CHECK_INT(f->wait_count, 0);
    CHECK_INT(f->now, 1000);
    free(f);
}

static void test_helper_launch_waits_for_the_named_process(void)
{
    Fake *f = fake_new();
    f->real_name = "empires2.exe";
    f->real_appears = 1000 + 3000;
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\AoE2\\launcher.exe", NULL, NULL, "C:\\AoE2", "empires2.exe", NULL, NULL};

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->waited[0], 2);
    CHECK(f->now >= 4000 && f->now < 4000 + LAUNCH_POLL_MS + 1);
    CHECK_INT(f->close_count, 2); /* helper and real game */
    free(f);
}

static void test_wait_name_without_extension_or_with_folder_matches_the_executable(void)
{
    Fake *f = fake_new();
    f->real_name = "empires2.exe";
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\AoE2\\launcher.exe", NULL, NULL, NULL, "empires2", NULL, NULL};

    launch_and_wait(&api, &t, NULL, NULL, NULL);
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->waited[0], 2);

    LaunchTarget t2 = {"C:\\AoE2\\launcher.exe", NULL, NULL, NULL, "AoE2\\empires2.exe", NULL, NULL};
    f->wait_count = 0;
    launch_and_wait(&api, &t2, NULL, NULL, NULL);
    CHECK_STR(f->searched, "empires2.exe");
    CHECK_INT(f->waited[0], 2);
    free(f);
}

static void test_menu_comes_back_when_the_named_process_never_appears(void)
{
    Fake *f = fake_new();
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\AoE2\\launcher.exe", NULL, NULL, NULL, "empires2.exe", NULL, NULL};

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK(f->now >= 1000 + LAUNCH_WAIT_LIMIT_MS);
    CHECK(f->now < 1000 + LAUNCH_WAIT_LIMIT_MS + 2 * LAUNCH_POLL_MS);
    /* falls back to the started process, which has already ended */
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->waited[0], 1);
    free(f);
}

static void test_shortcut_is_opened_through_the_shell_and_waited_for(void)
{
    Fake *f = fake_new();
    ProcApi api = fake_api(f);
    LaunchTarget t = {NULL, "C:\\Cyber\\games\\Doom\\Doom.lnk", NULL, NULL, NULL, NULL, NULL};

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK_STR(f->shell_path, "C:\\Cyber\\games\\Doom\\Doom.lnk");
    CHECK_STR(f->work_dir, "(null)");
    CHECK_INT(f->started_calls, 1);
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->waited[0], 1);
    free(f);
}

static void test_shortcut_without_a_process_returns_at_once(void)
{
    Fake *f = fake_new();
    f->shell_gives_proc = 0;
    ProcApi api = fake_api(f);
    LaunchTarget t = {NULL, "C:\\Cyber\\games\\Doom\\Doom.lnk", NULL, NULL, NULL, NULL, NULL};

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(r, LAUNCH_NOT_WAITED);
    CHECK_INT(f->wait_count, 0);
    CHECK_INT(f->close_count, 0);
    free(f);
}

static void test_shortcut_without_a_process_still_waits_for_the_named_process(void)
{
    Fake *f = fake_new();
    f->shell_gives_proc = 0;
    f->real_name = "doom95.exe";
    f->real_appears = 1500;
    ProcApi api = fake_api(f);
    LaunchTarget t = {NULL, "C:\\Doom.lnk", NULL, NULL, "doom95.exe", NULL, NULL};

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->waited[0], 2);
    free(f);
}

static void test_shortcut_that_cannot_open_fails(void)
{
    Fake *f = fake_new();
    f->start_ok = 0;
    ProcApi api = fake_api(f);
    LaunchTarget t = {NULL, "C:\\Doom.lnk", NULL, NULL, NULL, NULL, NULL};

    CHECK_INT(launch_and_wait(&api, &t, on_started, f, NULL), LAUNCH_FAILED);
    CHECK_INT(f->started_calls, 0);
    free(f);
}

static void test_no_launch_target_fails(void)
{
    Fake *f = fake_new();
    ProcApi api = fake_api(f);
    LaunchTarget t = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    CHECK_INT(launch_and_wait(&api, &t, on_started, f, NULL), LAUNCH_FAILED);
    free(f);
}

/* --- Gamepad mapper helper --- */

static void test_mapper_helper_starts_just_before_the_game_and_stops_after_it(void)
{
    Fake *f = fake_new();
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\Juegos\\Doom\\doom.exe", NULL, NULL, "C:\\Juegos\\Doom", NULL,
                      "C:\\JoyToKey\\JoyToKey.exe", "Doom"};
    int helper_failed = -1;

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, &helper_failed);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK_INT(helper_failed, 0);
    CHECK_STR(f->helper_exe, "C:\\JoyToKey\\JoyToKey.exe");
    CHECK_STR(f->helper_cmdline, "\"C:\\JoyToKey\\JoyToKey.exe\" Doom");
    CHECK_STR(f->helper_dir, "C:\\JoyToKey");
    CHECK_INT(f->helper_stops, 1);
    /* helper, game, menu hidden, wait, helper closed */
    CHECK_INT(f->event_count, 5);
    CHECK_INT(f->events[0], 'H');
    CHECK_INT(f->events[1], 'E');
    CHECK_INT(f->events[2], 'S');
    CHECK_INT(f->events[3], 'W');
    CHECK_INT(f->events[4], 'X');
    free(f);
}

static void test_game_still_starts_when_the_mapper_helper_cannot(void)
{
    Fake *f = fake_new();
    f->helper_ok = 0;
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\Juegos\\Doom\\doom.exe", NULL, NULL, "C:\\Juegos\\Doom", NULL,
                      "C:\\JoyToKey\\JoyToKey.exe", "Doom"};
    int helper_failed = 0;

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, &helper_failed);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK_INT(helper_failed, 1);
    CHECK_INT(f->started_calls, 1);
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->helper_stops, 0);
    CHECK_INT(f->events[f->event_count - 1], 'W'); /* nothing to close */
    free(f);
}

static void test_mapper_helper_is_closed_when_the_game_cannot_start(void)
{
    Fake *f = fake_new();
    f->start_ok = 0;
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\Juegos\\Doom\\doom.exe", NULL, NULL, NULL, NULL,
                      "C:\\JoyToKey\\JoyToKey.exe", NULL};
    int helper_failed = -1;

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, &helper_failed);

    CHECK_INT(r, LAUNCH_FAILED);
    CHECK_INT(helper_failed, 0);
    CHECK_STR(f->helper_cmdline, "\"C:\\JoyToKey\\JoyToKey.exe\"");
    CHECK_INT(f->helper_stops, 1);
    free(f);
}

static void test_without_mapper_helper_nothing_else_is_started(void)
{
    Fake *f = fake_new();
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\Juegos\\Doom\\doom.exe", NULL, NULL, NULL, NULL, NULL, "Doom"};
    int helper_failed = -1;

    LaunchResult r = launch_and_wait(&api, &t, on_started, f, &helper_failed);

    CHECK_INT(r, LAUNCH_ENDED);
    CHECK_INT(helper_failed, 0);
    CHECK_INT(f->helper_starts, 0);
    CHECK_INT(f->event_count, 3); /* game, menu hidden, wait: as before */
    free(f);
}

static void test_mapper_helper_stays_until_the_named_process_ends(void)
{
    Fake *f = fake_new();
    f->real_name = "empires2.exe";
    f->real_appears = 1000 + 2000;
    ProcApi api = fake_api(f);
    LaunchTarget t = {"C:\\AoE2\\launcher.exe", NULL, NULL, NULL, "empires2.exe",
                      "JoyToKey.exe", "AoE2"};

    launch_and_wait(&api, &t, on_started, f, NULL);

    CHECK_STR(f->helper_dir, "(null)"); /* bare name: inherit the folder */
    CHECK_INT(f->wait_count, 1);
    CHECK_INT(f->waited[0], 2);
    CHECK_INT(f->events[f->event_count - 2], 'W');
    CHECK_INT(f->events[f->event_count - 1], 'X');
    free(f);
}

int main(void)
{
    RUN_TEST(test_mapper_helper_starts_just_before_the_game_and_stops_after_it);
    RUN_TEST(test_game_still_starts_when_the_mapper_helper_cannot);
    RUN_TEST(test_mapper_helper_is_closed_when_the_game_cannot_start);
    RUN_TEST(test_without_mapper_helper_nothing_else_is_started);
    RUN_TEST(test_mapper_helper_stays_until_the_named_process_ends);
    RUN_TEST(test_executable_gets_its_arguments_and_working_folder);
    RUN_TEST(test_executable_without_arguments_gets_only_its_quoted_path);
    RUN_TEST(test_menu_hides_only_after_the_game_started_and_before_waiting);
    RUN_TEST(test_game_that_cannot_start_fails_without_waiting);
    RUN_TEST(test_helper_launch_waits_for_the_named_process);
    RUN_TEST(test_wait_name_without_extension_or_with_folder_matches_the_executable);
    RUN_TEST(test_menu_comes_back_when_the_named_process_never_appears);
    RUN_TEST(test_shortcut_is_opened_through_the_shell_and_waited_for);
    RUN_TEST(test_shortcut_without_a_process_returns_at_once);
    RUN_TEST(test_shortcut_without_a_process_still_waits_for_the_named_process);
    RUN_TEST(test_shortcut_that_cannot_open_fails);
    RUN_TEST(test_no_launch_target_fails);
    return TEST_REPORT();
}
