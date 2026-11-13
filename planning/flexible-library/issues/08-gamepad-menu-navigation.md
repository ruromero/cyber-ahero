---
type: ticket
status: done
tags: [gamepad, ui]
spec: "[[flexible-library/spec]]"
---

# 08: Gamepad menu navigation

**What to build:** Players can drive the menu with a gamepad: the D-pad or left stick moves the selection, A starts the selected game (like JUGAR), B closes the Info panel. Modern Xbox-style pads and older DirectInput pads both work, and pads can be connected or disconnected while the launcher runs. Pads are only read while the menu is showing. No configuration screen.

**Blocked by:** None (can start immediately)

## Acceptance criteria

- [ ] An Xbox-style pad and an older DirectInput pad can each move the selection, start a game and close the Info panel.
- [ ] Plugging a pad in while the menu is open makes it work without restarting; unplugging it causes no error.
- [x] Pad navigation scrolls the grid to keep the selection visible, like the arrow keys.
- [ ] Works on Windows XP.
- [x] README's controls table lists the gamepad buttons.

## Comments

- Pad input is translated to menu actions by the portable `src/pad.c` (buttons, hat edge detection, stick deadzone with hysteresis, per-pad state, unplug), covered natively by `tests/test_pad.c`. SDL glue (hotplug, controller vs. plain joystick, skipping duplicate joystick events of controllers) is in `src/pad_sdl.c`; `app.c` only adds the `PadNav` field, one init call, `pad_menu_action` (reuses `move_selection`, so the grid scrolls like with the arrows) and grouped `case` labels at the end of the event switch.
- Not verifiable on the macOS dev host (no Windows, no pads), left unticked: real Xbox (XInput) and DirectInput pads moving/launching/closing, live plug/unplug, and running on Windows XP. Reasoning: SDL 2.32 reads XInput and DirectInput pads; already-connected and newly plugged pads both arrive as `SDL_JOYDEVICEADDED` and are opened as game controllers when SDL has a mapping, else as plain joysticks (button 1 = A, button 2 = B, hat/first two axes = move); `SDL_JOYDEVICEREMOVED` closes them. No new Win32 imports, so XP compatibility is the same as SDL2's own. Both i686 and x86_64 cross-builds pass with no warnings.
- "Only read while the menu is showing": `SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS=0`, so pad input is dropped while a game has focus.
- For ticket 03 (event-wait loop): pad events arrive through the SDL queue, so `SDL_WaitEvent*` wakes on them and they count as input. Stick motion is noisy (many `AXISMOTION` events); only those that produce an action need a redraw.
