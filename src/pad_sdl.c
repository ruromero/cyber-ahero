/*
 * SDL side of gamepad navigation. Pads SDL knows as game controllers (XInput
 * and mapped DirectInput pads) are read through the controller API with an
 * Xbox layout; anything else is read as a plain joystick (buttons by index,
 * hat, first two axes). SDL also reports controllers as joysticks, so joystick
 * events of a device opened as a controller are skipped.
 */
#include "pad_sdl.h"

void pad_sdl_init(PadNav *nav)
{
    pad_init(nav);
    /* Pads are for the menu only: drop their input while a game has focus. */
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "0");
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        SDL_Log("Mandos no disponibles: %s", SDL_GetError());
    /* Pads already connected arrive as SDL_JOYDEVICEADDED events. */
}

static void open_device(int index)
{
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(index);
    if (id >= 0 && SDL_JoystickFromInstanceID(id))
        return; /* already open */
    if (SDL_IsGameController(index) && SDL_GameControllerOpen(index))
        return;
    if (!SDL_JoystickOpen(index))
        SDL_Log("No se pudo abrir el mando %d: %s", index, SDL_GetError());
}

static void close_device(PadNav *nav, SDL_JoystickID id)
{
    pad_forget(nav, (long)id);
    SDL_GameController *gc = SDL_GameControllerFromInstanceID(id);
    if (gc) {
        SDL_GameControllerClose(gc);
        return;
    }
    SDL_Joystick *joy = SDL_JoystickFromInstanceID(id);
    if (joy)
        SDL_JoystickClose(joy);
}

static PadButton controller_button(Uint8 b)
{
    switch (b) {
    case SDL_CONTROLLER_BUTTON_A:          return PAD_BTN_A;
    case SDL_CONTROLLER_BUTTON_B:          return PAD_BTN_B;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:    return PAD_BTN_DPAD_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  return PAD_BTN_DPAD_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  return PAD_BTN_DPAD_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return PAD_BTN_DPAD_RIGHT;
    default:                               return PAD_BTN_OTHER;
    }
}

static int is_controller(SDL_JoystickID id)
{
    return SDL_GameControllerFromInstanceID(id) != NULL;
}

PadAction pad_sdl_event(PadNav *nav, const SDL_Event *e)
{
    switch (e->type) {
    case SDL_JOYDEVICEADDED:
        open_device(e->jdevice.which);
        break;
    case SDL_JOYDEVICEREMOVED:
        close_device(nav, e->jdevice.which);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        return pad_button(controller_button(e->cbutton.button));
    case SDL_CONTROLLERAXISMOTION:
        if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
            return pad_axis(nav, (long)e->caxis.which, PAD_AXIS_X, e->caxis.value);
        if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
            return pad_axis(nav, (long)e->caxis.which, PAD_AXIS_Y, e->caxis.value);
        break;
    case SDL_JOYBUTTONDOWN:
        if (!is_controller(e->jbutton.which))
            return pad_joy_button(e->jbutton.button);
        break;
    case SDL_JOYHATMOTION:
        if (!is_controller(e->jhat.which) && e->jhat.hat == 0)
            return pad_hat(nav, (long)e->jhat.which, e->jhat.value);
        break;
    case SDL_JOYAXISMOTION:
        if (!is_controller(e->jaxis.which) && e->jaxis.axis < 2)
            return pad_axis(nav, (long)e->jaxis.which,
                            e->jaxis.axis == 0 ? PAD_AXIS_X : PAD_AXIS_Y, e->jaxis.value);
        break;
    default:
        break;
    }
    return PAD_ACT_NONE;
}
