#ifndef KIOSK_H
#define KIOSK_H

/*
 * Kiosk mode (ADR 0001). Portable: the registry and the desktop shell are
 * reached through a KioskApi, so the lockdown can be tested with a fake.
 * The Win32 implementation is kiosk_win() (kiosk_win.c).
 */

/* ---- command line ---- */

typedef enum RunMode {
    MODE_WINDOW,   /* ordinary window */
    MODE_KIOSK,    /* --kiosko */
    MODE_RECOVER   /* --limpiar: undo Kiosk mode, no UI */
} RunMode;

typedef struct CliArgs {
    RunMode mode;
    const char *library;  /* first argument that is not a switch, or NULL */
} CliArgs;

/* Switches start with "--" and ignore case; unknown ones are ignored. */
CliArgs cli_parse(int argc, char **argv);

/* Exit code of the launcher when the Quit button ends Kiosk mode. The kiosk
 * script starts the launcher again after any other exit code. */
#define KIOSK_EXIT_QUIT 10

/* ---- lockdown ---- */

typedef enum KioskScope {
    KIOSK_USER,     /* HKEY_CURRENT_USER */
    KIOSK_MACHINE   /* HKEY_LOCAL_MACHINE: needs administrator rights */
} KioskScope;

typedef struct KioskApi {
    void *ctx;
    /* Registry writes under scope\key (created if missing). Return 1 on
     * success, 0 on failure. remove_value succeeds when the value is absent. */
    int (*set_dword)(void *ctx, KioskScope scope, const char *key, const char *name,
                     unsigned long v);
    int (*set_string)(void *ctx, KioskScope scope, const char *key, const char *name,
                      const char *v);
    int (*remove_value)(void *ctx, KioskScope scope, const char *key, const char *name);
    /* Closes the desktop shell and keeps it closed. */
    void (*stop_shell)(void *ctx);
    /* Stops keeping the shell closed and starts it. */
    void (*start_shell)(void *ctx);
    /* Ends any other running launcher with KIOSK_EXIT_QUIT (recovery). */
    void (*stop_launchers)(void *ctx);
    /* Appends one message to the log (the sink adds the time). */
    void (*log)(void *ctx, const char *msg);
} KioskApi;

/* Applies the lockdown, registers autostart_cmd in this user's standard Run
 * key (HKCU, so no administrator rights are needed) and closes the shell. Logs every setting. Returns how many settings could not
 * be applied (e.g. machine-wide ones without administrator rights). */
int kiosk_enter(const KioskApi *api, const char *autostart_cmd);

/* Undoes every setting kiosk_enter applies, removes the automatic start
 * (also from the machine Run key and the wrong key older versions used) and
 * starts the shell. Does
 * not log off. Returns how many settings could not be undone. */
int kiosk_leave(const KioskApi *api);

/* The recovery switch: ends any running launcher, then kiosk_leave. */
int kiosk_recover(const KioskApi *api);

/* Win32 (kiosk_win.c, app only): the real registry and explorer.exe, the
 * troubleshooting log (cyber.log, see cyberlog.h), and the keyboard hook. */
const KioskApi *kiosk_win(void);
/* 1: swallow every key (menu showing); 0: release the keyboard (game). */
void kiosk_win_keyboard(int block);

#endif
