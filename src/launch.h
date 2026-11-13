#ifndef LAUNCH_H
#define LAUNCH_H

/*
 * Starting a game and waiting for it to end (ticket 04). Portable: every
 * call into the operating system goes through a ProcApi, so the flow can be
 * tested with a fake. The Win32 implementation is proc_win() (launch_win.c).
 *
 *   1. start the Launch target: an executable gets a command line
 *      ("<exe>" <args>) and its working folder; a shortcut is opened
 *      through the system shell, which may or may not hand back a process;
 *   2. on_started(user): the game is running (the menu hides its window);
 *   3. with wait= set, poll for a process of that name for up to
 *      LAUNCH_WAIT_LIMIT_MS and wait for it instead;
 *   4. wait for the process to end.
 *
 * Waiting must keep handling window messages (the Win32 ProcApi does).
 */

#include <stdint.h>

#define LAUNCH_WAIT_LIMIT_MS 60000
#define LAUNCH_POLL_MS 500

typedef void *ProcHandle;

typedef struct ProcApi {
    void *ctx;
    /* Starts exe with the full command line in work_dir (NULL = inherit).
     * Returns the process, or NULL on failure. */
    ProcHandle (*start_exe)(void *ctx, const char *exe, const char *cmdline,
                            const char *work_dir);
    /* Opens path through the shell. Returns 1 when started, 0 on failure;
     * *proc is the started process or NULL when the shell gives none. */
    int (*open_shell)(void *ctx, const char *path, const char *args, const char *work_dir,
                      ProcHandle *proc);
    /* A running process whose executable file name is exe_name (ignoring
     * case), or NULL. */
    ProcHandle (*find_process)(void *ctx, const char *exe_name);
    /* Blocks until proc ends. */
    void (*wait_exit)(void *ctx, ProcHandle proc);
    /* Blocks for ms milliseconds. */
    void (*sleep_ms)(void *ctx, int ms);
    uint32_t (*ticks)(void *ctx);
    void (*close)(void *ctx, ProcHandle proc);
} ProcApi;

/* Real Windows processes (launch_win.c, app only). */
const ProcApi *proc_win(void);

typedef struct LaunchTarget {
    const char *exe;       /* executable, or NULL */
    const char *shortcut;  /* shortcut, used when exe is NULL */
    const char *args;      /* may be NULL */
    const char *work_dir;  /* may be NULL */
    const char *wait;      /* wait=: process to wait for, may be NULL */
} LaunchTarget;

typedef enum LaunchResult {
    LAUNCH_FAILED,      /* could not be started; nothing to wait for */
    LAUNCH_ENDED,       /* started, and the process waited for has ended */
    LAUNCH_NOT_WAITED   /* started, but there was no process to wait for */
} LaunchResult;

/* "<exe>" <args>, malloc'd. */
char *launch_command_line(const char *exe, const char *args);

/* Runs steps 1-4 above. on_started may be NULL. */
LaunchResult launch_and_wait(const ProcApi *api, const LaunchTarget *t,
                             void (*on_started)(void *user), void *user);

#endif
