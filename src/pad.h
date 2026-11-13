#ifndef CYBER_PAD_H
#define CYBER_PAD_H

/*
 * Gamepad -> menu actions. Portable (no SDL): the SDL glue in app.c turns
 * controller/joystick events into these calls and applies the result.
 */

typedef enum PadAction {
    PAD_ACT_NONE = 0,
    PAD_ACT_LEFT,
    PAD_ACT_RIGHT,
    PAD_ACT_UP,
    PAD_ACT_DOWN,
    PAD_ACT_LAUNCH,
    PAD_ACT_CLOSE
} PadAction;

/* Buttons of a pad SDL recognises as a game controller (Xbox layout). */
typedef enum PadButton {
    PAD_BTN_A,
    PAD_BTN_B,
    PAD_BTN_DPAD_UP,
    PAD_BTN_DPAD_DOWN,
    PAD_BTN_DPAD_LEFT,
    PAD_BTN_DPAD_RIGHT,
    PAD_BTN_OTHER
} PadButton;

PadAction pad_button(PadButton b);

/* Buttons of a plain (DirectInput) joystick, by 0-based index. */
PadAction pad_joy_button(int index);

/* Hat bits; same values as SDL_HAT_*. */
#define PAD_HAT_CENTERED 0
#define PAD_HAT_UP 1
#define PAD_HAT_RIGHT 2
#define PAD_HAT_DOWN 4
#define PAD_HAT_LEFT 8

typedef enum PadAxis {
    PAD_AXIS_X,
    PAD_AXIS_Y
} PadAxis;

#define PAD_MAX_DEVICES 8

typedef struct PadDevice {
    int used;
    long id;
    int hat;
    int dir[2]; /* per PadAxis: -1, 0, +1 */
} PadDevice;

/* Per-device state so held directions move only once. Devices are keyed by
 * the caller's id (SDL joystick instance id). */
typedef struct PadNav {
    PadDevice dev[PAD_MAX_DEVICES];
} PadNav;

void pad_init(PadNav *nav);

/* Hat (D-pad) of a plain joystick moved to `hat` (PAD_HAT_* bits). */
PadAction pad_hat(PadNav *nav, long id, int hat);

/* Stick axis moved to `value` (-32768..32767, negative = left/up). A push
 * past half travel moves once; the stick must come back near the centre
 * before the same direction moves again. */
PadAction pad_axis(PadNav *nav, long id, PadAxis axis, int value);

/* Pad `id` was unplugged: drop its state. */
void pad_forget(PadNav *nav, long id);

#endif
