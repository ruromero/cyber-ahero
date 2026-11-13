#include "pad.h"

#include <string.h>

PadAction pad_button(PadButton b)
{
    switch (b) {
    case PAD_BTN_A:          return PAD_ACT_LAUNCH;
    case PAD_BTN_B:          return PAD_ACT_CLOSE;
    case PAD_BTN_DPAD_UP:    return PAD_ACT_UP;
    case PAD_BTN_DPAD_DOWN:  return PAD_ACT_DOWN;
    case PAD_BTN_DPAD_LEFT:  return PAD_ACT_LEFT;
    case PAD_BTN_DPAD_RIGHT: return PAD_ACT_RIGHT;
    default:                 return PAD_ACT_NONE;
    }
}

/* Old pads have no fixed layout; buttons "1" and "2" are the usual
 * accept/back pair and are reported as indices 0 and 1. */
PadAction pad_joy_button(int index)
{
    if (index == 0)
        return PAD_ACT_LAUNCH;
    if (index == 1)
        return PAD_ACT_CLOSE;
    return PAD_ACT_NONE;
}

void pad_init(PadNav *nav)
{
    memset(nav, 0, sizeof(*nav));
}

/* Slot for device `id`, claiming a free one on first use. With more pads than
 * slots, one slot is recycled (that pad just loses its held-state). */
static PadDevice *device(PadNav *nav, long id)
{
    PadDevice *free_slot = NULL;
    for (int i = 0; i < PAD_MAX_DEVICES; i++) {
        PadDevice *d = &nav->dev[i];
        if (d->used && d->id == id)
            return d;
        if (!d->used && !free_slot)
            free_slot = d;
    }
    if (!free_slot) {
        unsigned long k = (unsigned long)id % PAD_MAX_DEVICES;
        free_slot = &nav->dev[k];
    }
    memset(free_slot, 0, sizeof(*free_slot));
    free_slot->used = 1;
    free_slot->id = id;
    return free_slot;
}

PadAction pad_hat(PadNav *nav, long id, int hat)
{
    PadDevice *d = device(nav, id);
    int pressed = hat & ~d->hat;
    d->hat = hat;
    if (pressed & PAD_HAT_UP)
        return PAD_ACT_UP;
    if (pressed & PAD_HAT_DOWN)
        return PAD_ACT_DOWN;
    if (pressed & PAD_HAT_LEFT)
        return PAD_ACT_LEFT;
    if (pressed & PAD_HAT_RIGHT)
        return PAD_ACT_RIGHT;
    return PAD_ACT_NONE;
}

/* Hysteresis: engage past half travel, re-arm only near the centre, so a
 * stick resting near the threshold does not chatter. */
#define AXIS_PRESS 16384
#define AXIS_RELEASE 8000

PadAction pad_axis(PadNav *nav, long id, PadAxis axis, int value)
{
    if (axis != PAD_AXIS_X && axis != PAD_AXIS_Y)
        return PAD_ACT_NONE;
    PadDevice *d = device(nav, id);
    int *dir = &d->dir[axis];
    int now = 0;
    if (value >= AXIS_PRESS)
        now = 1;
    else if (value <= -AXIS_PRESS)
        now = -1;

    if (now == 0) {
        if (value < AXIS_RELEASE && value > -AXIS_RELEASE)
            *dir = 0;
        return PAD_ACT_NONE;
    }
    if (now == *dir)
        return PAD_ACT_NONE;
    *dir = now;
    if (axis == PAD_AXIS_X)
        return now < 0 ? PAD_ACT_LEFT : PAD_ACT_RIGHT;
    return now < 0 ? PAD_ACT_UP : PAD_ACT_DOWN;
}

void pad_forget(PadNav *nav, long id)
{
    for (int i = 0; i < PAD_MAX_DEVICES; i++) {
        if (nav->dev[i].used && nav->dev[i].id == id)
            memset(&nav->dev[i], 0, sizeof(nav->dev[i]));
    }
}
