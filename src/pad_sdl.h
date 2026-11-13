#ifndef CYBER_PAD_SDL_H
#define CYBER_PAD_SDL_H

#include <SDL.h>
#include "pad.h"

/* Starts SDL's game controller/joystick support. Without it pads are simply
 * ignored. Call after SDL_Init. */
void pad_sdl_init(PadNav *nav);

/* The one classification of SDL events as pad events, shared by the menu
 * loop and Attract mode. */
typedef enum PadEventKind {
    PAD_EVENT_NONE,    /* not a pad event */
    PAD_EVENT_DEVICE,  /* a pad was plugged in or out */
    PAD_EVENT_BUTTON,  /* a button went down */
    PAD_EVENT_MOTION   /* a stick or hat moved */
} PadEventKind;

PadEventKind pad_sdl_kind(const SDL_Event *e);

/* Feeds one SDL event: opens/closes pads on hotplug and returns the menu
 * action for pad input (PAD_ACT_NONE for anything else). */
PadAction pad_sdl_event(PadNav *nav, const SDL_Event *e);

#endif
