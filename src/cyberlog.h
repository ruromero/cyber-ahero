#ifndef CYBERLOG_H
#define CYBERLOG_H

/*
 * The troubleshooting log: one plain-text file, cyber.log next to the
 * launcher, written in normal and Kiosk mode and appended to across runs.
 * Once it reaches CYBERLOG_MAX_BYTES it is renamed to cyber.old.log (any
 * older one is replaced) and a new cyber.log is started.
 *
 * This module only builds the text, so it is portable and tested; the file
 * itself is written by cyber_log() and friends (cyberlog_win.c, app only).
 */

#include <stddef.h>

#include "library.h"
#include "launch.h"

#define CYBERLOG_FILE "cyber.log"
#define CYBERLOG_OLD_FILE "cyber.old.log"
#define CYBERLOG_MAX_BYTES (1024L * 1024L)

typedef struct LogTime {
    int year, month, day, hour, minute, second;
} LogTime;

/* "YYYY-MM-DD HH:MM:SS  <msg>\r\n" into buf (truncated, always ending the
 * line). */
void cyberlog_line(char *buf, size_t n, const LogTime *t, const char *msg);

/* 1 when a log of this size must be rotated before writing to it. */
int cyberlog_rotation_due(long size);

/* ---- scan block ---- */

typedef enum ScanCause {
    SCAN_STARTUP,     /* the launcher started */
    SCAN_AFTER_GAME,  /* a Player came back from a game */
    SCAN_MANUAL       /* F5 or the Reescanear button */
} ScanCause;

typedef void (*LogEmit)(void *user, const char *msg);

/* One message per line: a heading, each Available Entry with its Launch
 * target and the resolution rule that found it, each hidden Entry with its
 * reason (and the exe= it looked for), and a summary. */
void cyberlog_scan(const Library *lib, ScanCause why, LogEmit emit, void *user);

/* ---- games ---- */

typedef enum LogGameEvent {
    LOG_GAME_STARTED,
    LOG_GAME_ENDED,
    LOG_GAME_FAILED
} LogGameEvent;

/* The line for an Entry's game starting, ending or failing to start, with
 * the exact path launched, its arguments, working folder, wait= and mapper
 * helper (con=). */
void cyberlog_game(char *buf, size_t n, LogGameEvent ev, const char *entry,
                   const LaunchTarget *t);

/* ---- the file (cyberlog_win.c, app only) ---- */

/* Appends one timestamped line to cyber.log. */
void cyber_log(const char *msg);
/* Appends a scan block (see cyberlog_scan). */
void cyber_log_scan(const Library *lib, ScanCause why);

#endif
