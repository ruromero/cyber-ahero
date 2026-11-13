#include "redraw.h"

/* Tick comparisons use signed differences so they survive wraparound. */

void redraw_begin(Redraw *r, uint32_t last_frame)
{
    r->last_frame = last_frame;
    r->animating = 0;
    r->has_next = 0;
    r->next = 0;
}

void redraw_animating(Redraw *r)
{
    r->animating = 1;
}

void redraw_at(Redraw *r, uint32_t when)
{
    if ((int32_t)(when - r->last_frame) <= 0)
        return;
    if (!r->has_next || (int32_t)(when - r->next) < 0)
        r->next = when;
    r->has_next = 1;
}

int redraw_wait_ms(const Redraw *r, uint32_t now)
{
    if (r->animating)
        return 0;
    if (r->has_next) {
        int32_t left = (int32_t)(r->next - now);
        return left > 0 ? (int)left : 0;
    }
    return REDRAW_WAIT_FOREVER;
}
