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

/* Folder part of path, without the trailing separator except for a root
 * (C:\ or a leading \); NULL when path has no folder. */
static char *folder_of(const char *path)
{
    const char *base = path_basename(path);
    if (base == path)
        return NULL;
    size_t n = (size_t)(base - path) - 1;
    if (n == 0 || path[n - 1] == ':')
        n++;
    return xstrndup(path, n);
}

/* Starts the mapper helper in its own folder (where JoyToKey and the like
 * keep their profiles). NULL when there is none or it failed (*failed). */
static ProcHandle start_helper(const ProcApi *api, const LaunchTarget *t, int *failed)
{
    if (!t->con || !*t->con)
        return NULL;
    char *dir = folder_of(t->con);
    char *cmd = launch_command_line(t->con, t->con_args);
    ProcHandle h = api->start_helper(api->ctx, t->con, cmd, dir);
    free(cmd);
    free(dir);
    if (!h)
        *failed = 1;
    return h;
}

static LaunchResult run_game(const ProcApi *api, const LaunchTarget *t,
                             void (*on_started)(void *user), void *user);

LaunchResult launch_and_wait(const ProcApi *api, const LaunchTarget *t,
                             void (*on_started)(void *user), void *user,
                             int *helper_failed)
{
    int failed = 0;
    ProcHandle helper = start_helper(api, t, &failed);
    if (helper_failed)
        *helper_failed = failed;
    LaunchResult r = run_game(api, t, on_started, user);
    if (helper)
        api->stop_helper(api->ctx, helper);
    return r;
}

/* Steps 1-4. */
static LaunchResult run_game(const ProcApi *api, const LaunchTarget *t,
                             void (*on_started)(void *user), void *user)
{
    ProcHandle proc = NULL;

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

    /* game ended; launch_and_wait closes the mapper helper next */
    if (real)
        api->close(api->ctx, real);
    if (proc)
        api->close(api->ctx, proc);
    return r;
}
