#include "attract.h"

#include <string.h>

/* now has reached t (wrap-safe, like SDL_TICKS_PASSED). */
static int passed(uint32_t now, uint32_t t)
{
    return (int32_t)(t - now) <= 0;
}

void attract_init(Attract *a, int idle_minutes, uint32_t now)
{
    memset(a, 0, sizeof(*a));
    a->idle_ms = idle_minutes > 0 ? (uint32_t)idle_minutes * 60000u : 0;
    a->last_input = now;
}

int attract_input(Attract *a, uint32_t now)
{
    a->last_input = now;
    if (a->active) {
        a->active = 0;
        a->woke = 1;
        a->woke_at = now;
        return 1;
    }
    if (a->woke && !passed(now, a->woke_at + ATTRACT_WAKE_GUARD_MS))
        return 1;
    a->woke = 0;
    return 0;
}

static void show(Attract *a, uint32_t now, int entry)
{
    a->active = 1;
    a->entry = entry;
    a->entry_start = now;
    a->entry_ms = ATTRACT_COVER_MS;
    a->preview_ended = 0;
}

AttractEvent attract_update(Attract *a, uint32_t now, int entry_count)
{
    if (!a->active) {
        if (a->idle_ms && !a->game_running && entry_count > 0 && passed(now, a->last_input + a->idle_ms)) {
            show(a, now, 0);
            return ATTRACT_SHOW;
        }
        return ATTRACT_NONE;
    }
    if (entry_count <= 0) {
        a->active = 0;
        a->last_input = now;
        return ATTRACT_STOP;
    }
    if (a->preview_ended || passed(now, a->entry_start + a->entry_ms)) {
        int next = a->entry + 1;
        show(a, now, next < entry_count ? next : 0);
        return ATTRACT_SHOW;
    }
    return ATTRACT_NONE;
}

void attract_shown(Attract *a, int has_preview)
{
    a->entry_ms = has_preview ? ATTRACT_PREVIEW_MS : ATTRACT_COVER_MS;
}

void attract_preview_ended(Attract *a)
{
    if (a->active)
        a->preview_ended = 1;
}

int attract_game_running(Attract *a, int running, uint32_t now)
{
    int was_active = a->active;
    if (running) {
        a->game_running = 1;
        a->active = 0;
        return was_active;
    }
    if (a->game_running) {
        a->game_running = 0;
        a->last_input = now;
    }
    return 0;
}

int attract_active(const Attract *a)
{
    return a->active;
}

int attract_entry(const Attract *a)
{
    return a->entry;
}

int attract_deadline(const Attract *a, uint32_t *when)
{
    if (a->active) {
        *when = a->preview_ended ? a->entry_start : a->entry_start + a->entry_ms;
        return 1;
    }
    if (!a->idle_ms || a->game_running)
        return 0;
    *when = a->last_input + a->idle_ms;
    return 1;
}
