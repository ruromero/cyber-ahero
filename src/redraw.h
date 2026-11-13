#ifndef REDRAW_H
#define REDRAW_H

/*
 * Decides how long the idle menu may sleep before it must draw again.
 * Portable (no SDL): ticks are millisecond counters like SDL_GetTicks()
 * and may wrap around.
 *
 * After drawing a frame, the caller describes what is still going on:
 *   redraw_begin(&r, frame_tick);   tick at which that frame was drawn
 *   redraw_animating(&r);           a preview is playing: draw every frame
 *   redraw_at(&r, when);            something on screen changes at `when`
 * then sleeps for redraw_wait_ms(&r, now), waking early on any event.
 */

#include <stdint.h>

#define REDRAW_WAIT_FOREVER (-1)

typedef struct Redraw {
    uint32_t last_frame;
    int animating;
    int has_next;
    uint32_t next;
} Redraw;

void redraw_begin(Redraw *r, uint32_t last_frame);
void redraw_animating(Redraw *r);
/* Changes at or before the last frame were already drawn and are ignored. */
void redraw_at(Redraw *r, uint32_t when);
/* Milliseconds to wait for events before the next frame; 0 = draw now,
   REDRAW_WAIT_FOREVER = only an event can change the menu. */
int redraw_wait_ms(const Redraw *r, uint32_t now);

#endif
