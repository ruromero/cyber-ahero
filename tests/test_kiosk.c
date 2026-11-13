/* Kiosk mode: command line, lockdown plan applied and undone
 * through a fake registry/shell, and the log lines. */
#include "test.h"
#include "kiosk.h"

#include <stdlib.h>

/* ---- command line ------------------------------------------------------- */

static void test_no_arguments_is_an_ordinary_window(void)
{
    char *argv[] = {"cyber_ahero.exe", NULL};
    CliArgs a = cli_parse(1, argv);
    CHECK_INT(a.mode, MODE_WINDOW);
    CHECK_STR(a.library, NULL);
}

static void test_kiosk_switch_is_not_taken_for_the_library(void)
{
    char *argv[] = {"cyber_ahero.exe", "--kiosko", NULL};
    CliArgs a = cli_parse(2, argv);
    CHECK_INT(a.mode, MODE_KIOSK);
    CHECK_STR(a.library, NULL);
}

static void test_library_path_alongside_kiosk_switch_in_any_order(void)
{
    char *argv1[] = {"x.exe", "D:\\juegos", "--KIOSKO", NULL};
    CliArgs a = cli_parse(3, argv1);
    CHECK_INT(a.mode, MODE_KIOSK);
    CHECK_STR(a.library, "D:\\juegos");

    char *argv2[] = {"x.exe", "--kiosko", "D:\\juegos", NULL};
    a = cli_parse(3, argv2);
    CHECK_INT(a.mode, MODE_KIOSK);
    CHECK_STR(a.library, "D:\\juegos");
}

static void test_library_path_alone_keeps_working(void)
{
    char *argv[] = {"x.exe", "C:\\games", NULL};
    CliArgs a = cli_parse(2, argv);
    CHECK_INT(a.mode, MODE_WINDOW);
    CHECK_STR(a.library, "C:\\games");
}

static void test_recovery_switch_wins_over_kiosk(void)
{
    char *argv[] = {"x.exe", "--kiosko", "--limpiar", NULL};
    CliArgs a = cli_parse(3, argv);
    CHECK_INT(a.mode, MODE_RECOVER);
}

static void test_unknown_switch_is_ignored_not_a_library(void)
{
    char *argv[] = {"x.exe", "--otro", NULL};
    CliArgs a = cli_parse(2, argv);
    CHECK_INT(a.mode, MODE_WINDOW);
    CHECK_STR(a.library, NULL);
}

/* ---- fake registry and shell -------------------------------------------- */

#define POL_SYS "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"
#define POL_EXP "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"
#define ADV "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"
#define WINLOGON "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define RUN "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RUN_WRONG "Software\\Microsoft\\Windows NT\\CurrentVersion\\Run"

typedef struct Value {
    char text[400]; /* "HKCU\<key>\<name>=<data>", data "dword:N" or a string */
} Value;

typedef struct Fake {
    int admin;          /* machine-scope writes succeed */
    Value values[64];
    int count;
    int shell_running;
    int shell_stops, shell_starts;
    int launchers_stopped;
    char events[32];    /* 'L' stop launchers, 'R' registry write, 'S' stop shell, 'X' start shell */
    int nevents;
    char log[64][300];
    int nlog;
} Fake;

static void event(Fake *f, char c)
{
    if (f->nevents < (int)sizeof(f->events) - 1)
        f->events[f->nevents++] = c;
}

static void value_prefix(char *out, size_t n, KioskScope scope, const char *key, const char *name)
{
    snprintf(out, n, "%s\\%s\\%s=", scope == KIOSK_MACHINE ? "HKLM" : "HKCU", key, name);
}

static int find_value(Fake *f, const char *prefix)
{
    for (int i = 0; i < f->count; i++)
        if (strncmp(f->values[i].text, prefix, strlen(prefix)) == 0)
            return i;
    return -1;
}

static void put(Fake *f, const char *prefix, const char *data)
{
    int i = find_value(f, prefix);
    if (i < 0)
        i = f->count++;
    snprintf(f->values[i].text, sizeof(f->values[i].text), "%s%s", prefix, data);
}

static int f_set_dword(void *ctx, KioskScope scope, const char *key, const char *name,
                       unsigned long v)
{
    Fake *f = ctx;
    event(f, 'R');
    if (scope == KIOSK_MACHINE && !f->admin)
        return 0;
    char p[300], d[32];
    value_prefix(p, sizeof(p), scope, key, name);
    snprintf(d, sizeof(d), "dword:%lu", v);
    put(f, p, d);
    return 1;
}

static int f_set_string(void *ctx, KioskScope scope, const char *key, const char *name,
                        const char *v)
{
    Fake *f = ctx;
    event(f, 'R');
    if (scope == KIOSK_MACHINE && !f->admin)
        return 0;
    char p[300];
    value_prefix(p, sizeof(p), scope, key, name);
    put(f, p, v);
    return 1;
}

static int f_remove_value(void *ctx, KioskScope scope, const char *key, const char *name)
{
    Fake *f = ctx;
    event(f, 'R');
    char p[300];
    value_prefix(p, sizeof(p), scope, key, name);
    int i = find_value(f, p);
    if (i < 0)
        return 1; /* already absent */
    if (scope == KIOSK_MACHINE && !f->admin)
        return 0;
    f->values[i] = f->values[--f->count];
    return 1;
}

static void f_stop_shell(void *ctx)
{
    Fake *f = ctx;
    event(f, 'S');
    f->shell_running = 0;
    f->shell_stops++;
}

static void f_start_shell(void *ctx)
{
    Fake *f = ctx;
    event(f, 'X');
    f->shell_running = 1;
    f->shell_starts++;
}

static void f_stop_launchers(void *ctx)
{
    Fake *f = ctx;
    event(f, 'L');
    f->launchers_stopped++;
}

static void f_log(void *ctx, const char *msg)
{
    Fake *f = ctx;
    if (f->nlog < 64)
        snprintf(f->log[f->nlog++], sizeof(f->log[0]), "%s", msg);
}

static KioskApi fake_api(Fake *f, int admin)
{
    memset(f, 0, sizeof(*f));
    f->admin = admin;
    f->shell_running = 1;
    KioskApi api = {f, f_set_dword, f_set_string, f_remove_value,
                    f_stop_shell, f_start_shell, f_stop_launchers, f_log};
    return api;
}

static int has(Fake *f, const char *text)
{
    for (int i = 0; i < f->count; i++)
        if (strcmp(f->values[i].text, text) == 0)
            return 1;
    return 0;
}

static int log_has(Fake *f, const char *a, const char *b)
{
    for (int i = 0; i < f->nlog; i++)
        if (strstr(f->log[i], a) && (!b || strstr(f->log[i], b)))
            return 1;
    return 0;
}

#define AUTOSTART "\"C:\\kiosko\\kiosko.bat\""

/* ---- entering Kiosk mode ------------------------------------------------ */

static void test_entering_with_admin_applies_the_whole_lockdown(void)
{
    Fake f;
    KioskApi api = fake_api(&f, 1);
    CHECK_INT(kiosk_enter(&api, AUTOSTART), 0);

    CHECK(has(&f, "HKCU\\" POL_SYS "\\DisableTaskMgr=dword:1"));
    CHECK(has(&f, "HKCU\\" POL_EXP "\\NoRun=dword:1"));
    CHECK(has(&f, "HKCU\\" POL_EXP "\\NoWinKeys=dword:1"));
    CHECK(has(&f, "HKCU\\" ADV "\\Start=NoStartMenu"));
    CHECK(has(&f, "HKLM\\" POL_SYS "\\DisableTaskMgr=dword:1"));
    CHECK(has(&f, "HKLM\\" POL_EXP "\\NoRun=dword:1"));
    CHECK(has(&f, "HKLM\\" POL_EXP "\\NoWinKeys=dword:1"));
    CHECK(has(&f, "HKLM\\" WINLOGON "\\AutoRestartShell=dword:0"));
    CHECK(has(&f, "HKCU\\" WINLOGON "\\AutoRestartShell=dword:0"));
    CHECK(has(&f, "HKLM\\" WINLOGON "\\DisableCAD=dword:1"));
    CHECK(has(&f, "HKCU\\" RUN "\\CiberAhKiosko=" AUTOSTART));
    CHECK_INT(f.count, 11);

    CHECK_INT(f.shell_running, 0);
    CHECK_INT(f.launchers_stopped, 0);
}

static void test_without_admin_the_machine_settings_are_reported_not_applied(void)
{
    Fake f;
    KioskApi api = fake_api(&f, 0);
    CHECK_INT(kiosk_enter(&api, AUTOSTART), 5);

    /* the user-scope lockdown and the automatic start still apply */
    CHECK(has(&f, "HKCU\\" POL_SYS "\\DisableTaskMgr=dword:1"));
    CHECK(has(&f, "HKCU\\" RUN "\\CiberAhKiosko=" AUTOSTART));
    CHECK_INT(f.count, 6);
    CHECK_INT(f.shell_running, 0);

    CHECK(log_has(&f, "NO aplicado (hace falta administrador)", "Ctrl+Alt+Supr"));
    CHECK(log_has(&f, "NO aplicado (hace falta administrador)", "AutoRestartShell"));
    CHECK(log_has(&f, "NO aplicado (hace falta administrador)", "Ejecutar bloqueado (maquina)"));
    CHECK(log_has(&f, "aplicado: ", "Ejecutar bloqueado (usuario)"));
    CHECK(!log_has(&f, "NO aplicado", "(usuario)"));
}

/* ---- leaving Kiosk mode ------------------------------------------------- */

static void snapshot(Fake *f, char out[][400], int *n)
{
    *n = f->count;
    for (int i = 0; i < f->count; i++)
        strcpy(out[i], f->values[i].text);
}

static int same_values(Fake *f, char before[][400], int n)
{
    if (f->count != n)
        return 0;
    for (int i = 0; i < n; i++)
        if (!has(f, before[i]))
            return 0;
    return 1;
}

static void test_leaving_undoes_exactly_what_entering_did(void)
{
    Fake f;
    KioskApi api = fake_api(&f, 1);
    /* a stock XP machine has AutoRestartShell=1 and an unrelated Run entry */
    put(&f, "HKLM\\" WINLOGON "\\AutoRestartShell=", "dword:1");
    put(&f, "HKCU\\" RUN "\\ctfmon=", "ctfmon.exe");
    char before[64][400];
    int n;
    snapshot(&f, before, &n);

    kiosk_enter(&api, AUTOSTART);
    CHECK_INT(kiosk_leave(&api), 0);

    CHECK(same_values(&f, before, n));
    CHECK_INT(f.shell_running, 1);
    CHECK_INT(f.shell_starts, 1);
    CHECK_INT(f.launchers_stopped, 0);
}

static void test_leaving_removes_the_automatic_start_left_by_older_versions(void)
{
    Fake f;
    KioskApi api = fake_api(&f, 1);
    put(&f, "HKLM\\" RUN "\\CiberAhKiosko=", "powershell.exe -File kiosko.ps1");
    put(&f, "HKLM\\" RUN_WRONG "\\CiberAhKiosko=", "powershell.exe -File kiosko.ps1");

    kiosk_leave(&api);

    CHECK_INT(f.count, 1);
    CHECK(has(&f, "HKLM\\" WINLOGON "\\AutoRestartShell=dword:1"));
}

static void test_leaving_without_admin_reports_what_could_not_be_undone(void)
{
    Fake f;
    KioskApi api = fake_api(&f, 1);
    kiosk_enter(&api, AUTOSTART); /* entered once as administrator */
    f.admin = 0;
    f.nlog = 0;

    CHECK(kiosk_leave(&api) > 0);

    CHECK(log_has(&f, "NO deshecho (hace falta administrador)", "Ctrl+Alt+Supr"));
    CHECK(log_has(&f, "deshecho: ", "Autoarranque"));
    CHECK(!has(&f, "HKCU\\" RUN "\\CiberAhKiosko=" AUTOSTART));
    CHECK(!has(&f, "HKCU\\" POL_EXP "\\NoRun=dword:1"));
    CHECK_INT(f.shell_running, 1);
}

static void test_recovery_ends_running_launchers_before_undoing(void)
{
    Fake f;
    KioskApi api = fake_api(&f, 1);
    kiosk_enter(&api, AUTOSTART);
    f.nevents = 0;

    kiosk_recover(&api);

    CHECK_INT(f.events[0], 'L');
    CHECK_INT(f.events[f.nevents - 1], 'X');
    CHECK_INT(f.count, 1); /* only AutoRestartShell=1 is left */
    CHECK_INT(f.shell_running, 1);
}

int main(void)
{
    RUN_TEST(test_no_arguments_is_an_ordinary_window);
    RUN_TEST(test_kiosk_switch_is_not_taken_for_the_library);
    RUN_TEST(test_library_path_alongside_kiosk_switch_in_any_order);
    RUN_TEST(test_library_path_alone_keeps_working);
    RUN_TEST(test_recovery_switch_wins_over_kiosk);
    RUN_TEST(test_unknown_switch_is_ignored_not_a_library);
    RUN_TEST(test_entering_with_admin_applies_the_whole_lockdown);
    RUN_TEST(test_without_admin_the_machine_settings_are_reported_not_applied);
    RUN_TEST(test_leaving_undoes_exactly_what_entering_did);
    RUN_TEST(test_leaving_removes_the_automatic_start_left_by_older_versions);
    RUN_TEST(test_leaving_without_admin_reports_what_could_not_be_undone);
    RUN_TEST(test_recovery_ends_running_launchers_before_undoing);
    return TEST_REPORT();
}
