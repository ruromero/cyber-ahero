#ifndef CYBER_ATTRACT_H
#define CYBER_ATTRACT_H

/*
 * Attract mode timing. Portable (no SDL): ticks are millisecond
 * counters like SDL_GetTicks() and may wrap around.
 *
 * The menu loop drives it:
 *   attract_input(&at, now)        on any mouse/keyboard/pad input; returns 1
 *                                  when it left Attract mode (input consumed)
 *   attract_update(&at, now, n)    every frame, n = Available Entries; says
 *                                  when to start showing attract_entry(&at)
 *                                  or to go back to the menu
 *   attract_shown(&at, has_preview)   after opening that Entry: preview or
 *                                  cover only (decides 20 s vs 8 s)
 *   attract_preview_ended(&at)     the preview reached its end
 *   attract_game_running(&at, r, now)  hook for the launcher:
 *                                  Attract mode never starts while r != 0
 *   attract_deadline(&at, &when)   for the redraw wait: next time-driven change
 */

#include <stdint.h>

#define ATTRACT_PREVIEW_MS 20000u
#define ATTRACT_COVER_MS 8000u
/* Input this soon after waking is consumed too (a pad press arrives as two
   events, a double-click as two clicks). */
#define ATTRACT_WAKE_GUARD_MS 500u

typedef enum AttractEvent {
    ATTRACT_NONE = 0,
    ATTRACT_SHOW, /* show attract_entry() full screen (start or next Entry) */
    ATTRACT_STOP  /* back to the menu (e.g. no Entries left) */
} AttractEvent;

typedef struct Attract {
    uint32_t idle_ms; /* 0 = off */
    uint32_t last_input;
    int game_running;
    int active;
    int entry;
    uint32_t entry_start;
    uint32_t entry_ms;
    int preview_ended;
    int woke;
    uint32_t woke_at;
} Attract;

/* idle_minutes as from laptop_config_attract_minutes(); 0 = off. */
void attract_init(Attract *a, int idle_minutes, uint32_t now);

/* Someone touched the launcher. Returns 1 if the input is consumed: it left
   Attract mode (the caller goes back to the menu) or came within
   ATTRACT_WAKE_GUARD_MS of doing so. Either way the caller ignores it. */
int attract_input(Attract *a, uint32_t now);

AttractEvent attract_update(Attract *a, uint32_t now, int entry_count);

void attract_shown(Attract *a, int has_preview);
void attract_preview_ended(Attract *a);

/* Returns 1 if this left Attract mode (the caller goes back to the menu).
   The idle time counts again from when the game stops. */
int attract_game_running(Attract *a, int running, uint32_t now);

int attract_active(const Attract *a);
int attract_entry(const Attract *a);

/* 1 and *when = tick of the next change attract_update() will report; 0 when
   only input can change anything (off, or a game is running). A playing
   preview is animated by the caller anyway. */
int attract_deadline(const Attract *a, uint32_t *when);

#endif
