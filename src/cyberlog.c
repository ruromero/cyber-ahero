#include "cyberlog.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

void cyberlog_line(char *buf, size_t n, const LogTime *t, const char *msg)
{
    if (n < 3) {
        if (n)
            buf[0] = 0;
        return;
    }
    snprintf(buf, n - 2, "%04d-%02d-%02d %02d:%02d:%02d  %s", t->year, t->month, t->day,
             t->hour, t->minute, t->second, msg ? msg : "");
    strcat(buf, "\r\n");
}

int cyberlog_rotation_due(long size)
{
    return size >= CYBERLOG_MAX_BYTES;
}

/* ---- scan block ------------------------------------------------------------ */

static const char *cause_text(ScanCause why)
{
    switch (why) {
    case SCAN_STARTUP:    return "inicio";
    case SCAN_AFTER_GAME: return "tras un juego";
    default:              return "reescaneo manual";
    }
}

static void rule_text(char *buf, size_t n, const Game *g)
{
    switch (g->rule) {
    case RULE_OVERRIDE:
        snprintf(buf, n, "exe.%s= de cyber.cfg", path_basename(g->dir));
        break;
    case RULE_EXE_ABSOLUTE:
        snprintf(buf, n, "exe= absoluto");
        break;
    case RULE_EXE_INSTALL:
        snprintf(buf, n, "exe= relativo, carpeta de instalacion %d", g->rule_install);
        break;
    case RULE_EXE_ENTRY:
        snprintf(buf, n, "exe= relativo, carpeta del juego");
        break;
    case RULE_NAME_SEARCH:
        snprintf(buf, n, "busqueda por nombre en las carpetas de instalacion");
        break;
    case RULE_SHORTCUT:
        snprintf(buf, n, "acceso directo");
        break;
    default:
        snprintf(buf, n, "eleccion automatica");
        break;
    }
}

static void hidden_line(char *buf, size_t n, const HiddenEntry *h)
{
    const char *why = h->reason == HIDDEN_NO_LAUNCH_INFO ? "sin informacion para lanzarlo"
                                                         : "instalacion no encontrada";
    int len = snprintf(buf, n, "  Oculto: %s: %s", h->name, why);
    if (len < 0 || (size_t)len >= n || (!h->exe && !h->exe_override))
        return;
    char looked[1024];
    looked[0] = 0;
    if (h->exe_override)
        snprintf(looked, sizeof(looked), "exe.%s=%s%s", path_basename(h->dir), h->exe_override,
                 h->exe ? ", " : "");
    if (h->exe) {
        size_t used = strlen(looked);
        snprintf(looked + used, sizeof(looked) - used, "exe=%s", h->exe);
    }
    snprintf(buf + len, n - (size_t)len, " (buscaba %s)", looked);
}

void cyberlog_scan(const Library *lib, ScanCause why, LogEmit emit, void *user)
{
    char line[2048];
    snprintf(line, sizeof(line), "Escaneo de la biblioteca (%s): %s", cause_text(why), lib->root);
    emit(user, line);
    for (int i = 0; i < lib->count; i++) {
        const Game *g = &lib->games[i];
        char rule[512];
        rule_text(rule, sizeof(rule), g);
        snprintf(line, sizeof(line), "  Disponible: %s -> %s (%s)", g->name,
                 g->exe ? g->exe : g->shortcut, rule);
        emit(user, line);
    }
    for (int i = 0; i < lib->hidden_count; i++) {
        hidden_line(line, sizeof(line), &lib->hidden[i]);
        emit(user, line);
    }
    snprintf(line, sizeof(line), "Escaneo terminado: %d disponibles, %d ocultos", lib->count,
             lib->hidden_count);
    emit(user, line);
}

/* ---- games ------------------------------------------------------------------ */

void cyberlog_game(char *buf, size_t n, LogGameEvent ev, const char *entry,
                   const LaunchTarget *t)
{
    const char *what = ev == LOG_GAME_STARTED ? "Juego iniciado: "
                     : ev == LOG_GAME_ENDED   ? "Juego terminado: "
                                              : "ERROR: no se pudo iniciar ";
    const char *path = t ? (t->exe ? t->exe : t->shortcut) : NULL;
    int len = snprintf(buf, n, "%s%s", what, entry);
    if (!path || len < 0 || (size_t)len >= n)
        return;
    char extra[2048];
    extra[0] = 0;
    size_t used = 0;
    const char *sep = "";
    if (t->work_dir) {
        used += (size_t)snprintf(extra + used, sizeof(extra) - used, "carpeta %s", t->work_dir);
        sep = "; ";
    }
    if (t->wait && used < sizeof(extra)) {
        used += (size_t)snprintf(extra + used, sizeof(extra) - used, "%swait=%s", sep, t->wait);
        sep = "; ";
    }
    if (t->con && used < sizeof(extra))
        snprintf(extra + used, sizeof(extra) - used, "%scon=%s%s%s", sep, t->con,
                 t->con_args ? " " : "", t->con_args ? t->con_args : "");
    snprintf(buf + len, n - (size_t)len, " -> %s%s%s%s%s%s", path, t->args ? " " : "",
             t->args ? t->args : "", extra[0] ? " (" : "", extra, extra[0] ? ")" : "");
}
