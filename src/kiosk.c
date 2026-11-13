#include "kiosk.h"
#include "compat.h"

#include <stdio.h>
#include <string.h>

CliArgs cli_parse(int argc, char **argv)
{
    CliArgs a;
    a.mode = MODE_WINDOW;
    a.library = NULL;
    int recover = 0;
    for (int i = 1; i < argc && argv[i]; i++) {
        const char *s = argv[i];
        if (strncmp(s, "--", 2) == 0) {
            if (_stricmp(s, "--kiosko") == 0 && a.mode == MODE_WINDOW)
                a.mode = MODE_KIOSK;
            else if (_stricmp(s, "--limpiar") == 0)
                recover = 1;
        } else if (!a.library) {
            a.library = s;
        }
    }
    if (recover)
        a.mode = MODE_RECOVER;
    return a;
}

/* ---- lockdown plan ------------------------------------------------------ */

#define POL_SYS "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"
#define POL_EXP "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"
#define ADV "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"
#define WINLOGON "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define RUN "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RUN_WRONG "Software\\Microsoft\\Windows NT\\CurrentVersion\\Run"
#define RUN_NAME "CiberAhKiosko"

typedef enum Undo { UNDO_REMOVE, UNDO_SET_DWORD } Undo;

typedef struct Setting {
    KioskScope scope;
    const char *key;
    const char *name;
    const char *str;      /* string data, or NULL for a DWORD */
    unsigned long dword;
    Undo undo;
    unsigned long undo_dword;
    const char *what;     /* for the log */
} Setting;

/* str == AUTOSTART marks the Run value (data given by the caller). */
static const char AUTOSTART[] = "";

static const Setting PLAN[] = {
    {KIOSK_USER, POL_SYS, "DisableTaskMgr", NULL, 1, UNDO_REMOVE, 0,
     "Administrador de tareas bloqueado (usuario)"},
    {KIOSK_USER, POL_EXP, "NoRun", NULL, 1, UNDO_REMOVE, 0, "Ejecutar bloqueado (usuario)"},
    {KIOSK_USER, POL_EXP, "NoWinKeys", NULL, 1, UNDO_REMOVE, 0, "Tecla Windows bloqueada (usuario)"},
    {KIOSK_USER, ADV, "Start", "NoStartMenu", 0, UNDO_REMOVE, 0, "Boton Inicio quitado"},
    {KIOSK_MACHINE, POL_SYS, "DisableTaskMgr", NULL, 1, UNDO_REMOVE, 0,
     "Administrador de tareas bloqueado (maquina)"},
    {KIOSK_MACHINE, POL_EXP, "NoRun", NULL, 1, UNDO_REMOVE, 0, "Ejecutar bloqueado (maquina)"},
    {KIOSK_MACHINE, POL_EXP, "NoWinKeys", NULL, 1, UNDO_REMOVE, 0,
     "Tecla Windows bloqueada (maquina)"},
    /* Windows' default for AutoRestartShell is 1: leaving puts it back. */
    {KIOSK_MACHINE, WINLOGON, "AutoRestartShell", NULL, 0, UNDO_SET_DWORD, 1,
     "Windows no reinicia el escritorio (AutoRestartShell=0)"},
    {KIOSK_USER, WINLOGON, "AutoRestartShell", NULL, 0, UNDO_REMOVE, 0,
     "AutoRestartShell=0 (usuario)"},
    {KIOSK_MACHINE, WINLOGON, "DisableCAD", NULL, 1, UNDO_REMOVE, 0, "Ctrl+Alt+Supr bloqueado"},
    {KIOSK_USER, RUN, RUN_NAME, AUTOSTART, 0, UNDO_REMOVE, 0,
     "Autoarranque del modo kiosko al iniciar sesion"},
};

#define PLAN_COUNT ((int)(sizeof(PLAN) / sizeof(PLAN[0])))

static void log_msg(const KioskApi *api, const char *msg)
{
    if (api->log)
        api->log(api->ctx, msg);
}

static void log_setting(const KioskApi *api, const char *verb, int ok, const Setting *s)
{
    char buf[300];
    if (ok)
        snprintf(buf, sizeof(buf), "%s: %s", verb, s->what);
    else if (s->scope == KIOSK_MACHINE)
        snprintf(buf, sizeof(buf), "NO %s (hace falta administrador): %s", verb, s->what);
    else
        snprintf(buf, sizeof(buf), "NO %s: %s", verb, s->what);
    log_msg(api, buf);
}

int kiosk_enter(const KioskApi *api, const char *autostart_cmd)
{
    int failed = 0;
    log_msg(api, "Modo kiosko: aplicando restricciones");
    for (int i = 0; i < PLAN_COUNT; i++) {
        const Setting *s = &PLAN[i];
        int ok;
        if (s->str == AUTOSTART)
            ok = api->set_string(api->ctx, s->scope, s->key, s->name, autostart_cmd);
        else if (s->str)
            ok = api->set_string(api->ctx, s->scope, s->key, s->name, s->str);
        else
            ok = api->set_dword(api->ctx, s->scope, s->key, s->name, s->dword);
        if (!ok)
            failed++;
        log_setting(api, "aplicado", ok, s);
    }
    api->stop_shell(api->ctx);
    log_msg(api, "Escritorio (explorer.exe) cerrado");
    return failed;
}

/* Where older (PowerShell) versions registered the automatic start: the
 * machine Run key, and by mistake a Run key under "Windows NT". */
static const Setting LEGACY[] = {
    {KIOSK_MACHINE, RUN, RUN_NAME, AUTOSTART, 0, UNDO_REMOVE, 0,
     "Autoarranque de versiones antiguas (HKLM Run)"},
    {KIOSK_MACHINE, RUN_WRONG, RUN_NAME, AUTOSTART, 0, UNDO_REMOVE, 0,
     "Autoarranque de versiones antiguas (clave Run erronea de Windows NT)"},
};

static int undo(const KioskApi *api, const Setting *s)
{
    int ok;
    if (s->undo == UNDO_SET_DWORD)
        ok = api->set_dword(api->ctx, s->scope, s->key, s->name, s->undo_dword);
    else
        ok = api->remove_value(api->ctx, s->scope, s->key, s->name);
    log_setting(api, "deshecho", ok, s);
    return ok;
}

int kiosk_leave(const KioskApi *api)
{
    int failed = 0;
    log_msg(api, "Modo kiosko: deshaciendo restricciones");
    for (int i = 0; i < PLAN_COUNT; i++)
        failed += !undo(api, &PLAN[i]);
    for (int i = 0; i < (int)(sizeof(LEGACY) / sizeof(LEGACY[0])); i++)
        failed += !undo(api, &LEGACY[i]);
    api->start_shell(api->ctx);
    log_msg(api, "Escritorio (explorer.exe) arrancado");
    return failed;
}

int kiosk_recover(const KioskApi *api)
{
    log_msg(api, "Recuperacion: cerrando el launcher en marcha");
    api->stop_launchers(api->ctx);
    return kiosk_leave(api);
}
