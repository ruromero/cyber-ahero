#ifndef CYBER_PAD_SDL_H
#define CYBER_PAD_SDL_H

#include <SDL.h>
#include "pad.h"

/* Starts SDL's game controller/joystick support. Without it pads are simply
 * ignored. Call after SDL_Init. */
void pad_sdl_init(PadNav *nav);

/* Feeds one SDL event: opens/closes pads on hotplug and returns the menu
 * action for pad input (PAD_ACT_NONE for anything else). */
PadAction pad_sdl_event(PadNav *nav, const SDL_Event *e);

#endif
