#include "launch.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

char *launch_command_line(const char *exe, const char *args)
{
    size_t el = strlen(exe);
    size_t al = args ? strlen(args) : 0;
    char *c = xmalloc(el + al + 4);
    c[0] = '"';
    memcpy(c + 1, exe, el);
    c[el + 1] = '"';
    if (al) {
        c[el + 2] = ' ';
        memcpy(c + el + 3, args, al + 1);
    } else {
        c[el + 2] = 0;
    }
    return c;
}

/* wait= as an executable file name: folders dropped, ".exe" added when the
 * name has no extension. */
static char *process_name(const char *wait)
{
    const char *base = path_basename(wait);
    if (strchr(base, '.'))
        return xstrdup(base);
    size_t n = strlen(base);
    char *s = xmalloc(n + 5);
    memcpy(s, base, n);
    memcpy(s + n, ".exe", 5);
    return s;
}

/* Polls for the named process for up to LAUNCH_WAIT_LIMIT_MS. */
static ProcHandle find_within_limit(const ProcApi *api, const char *wait)
{
    char *name = process_name(wait);
    uint32_t start = api->ticks(api->ctx);
    ProcHandle p = NULL;
    for (;;) {
        p = api->find_process(api->ctx, name);
        if (p || (uint32_t)(api->ticks(api->ctx) - start) >= LAUNCH_WAIT_LIMIT_MS)
            break;
        api->sleep_ms(api->ctx, LAUNCH_POLL_MS);
    }
    free(name);
    return p;
}

LaunchResult launch_and_wait(const ProcApi *api, const LaunchTarget *t,
                             void (*on_started)(void *user), void *user)
{
    ProcHandle proc = NULL;

    /* ticket 09: start the mapper helper (con=, con_args=) here, just before
     * the game, and close it at "game ended" below (also on failure). */

    if (t->exe) {
        char *cmd = launch_command_line(t->exe, t->args);
        proc = api->start_exe(api->ctx, t->exe, cmd, t->work_dir);
        free(cmd);
        if (!proc)
            return LAUNCH_FAILED;
    } else if (t->shortcut) {
        if (!api->open_shell(api->ctx, t->shortcut, t->args, t->work_dir, &proc))
            return LAUNCH_FAILED;
    } else {
        return LAUNCH_FAILED;
    }

    if (on_started)
        on_started(user);

    ProcHandle real = NULL;
    if (t->wait && *t->wait)
        real = find_within_limit(api, t->wait);

    LaunchResult r = LAUNCH_ENDED;
    if (real)
        api->wait_exit(api->ctx, real);
    else if (proc)
        api->wait_exit(api->ctx, proc);
    else
        r = LAUNCH_NOT_WAITED;

    /* game ended (ticket 09: close the mapper helper here) */
    if (real)
        api->close(api->ctx, real);
    if (proc)
        api->close(api->ctx, proc);
    return r;
}
