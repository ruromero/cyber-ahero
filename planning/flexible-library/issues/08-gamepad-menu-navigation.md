---
type: ticket
status: open
tags: [gamepad, ui]
spec: "[[flexible-library/spec]]"
---

# 08: Gamepad menu navigation

**What to build:** Players can drive the menu with a gamepad: the D-pad or left stick moves the selection, A starts the selected game (like JUGAR), B closes the Info panel. Modern Xbox-style pads and older DirectInput pads both work, and pads can be connected or disconnected while the launcher runs. Pads are only read while the menu is showing. No configuration screen.

**Blocked by:** None (can start immediately)

## Acceptance criteria

- [ ] An Xbox-style pad and an older DirectInput pad can each move the selection, start a game and close the Info panel.
- [ ] Plugging a pad in while the menu is open makes it work without restarting; unplugging it causes no error.
- [ ] Pad navigation scrolls the grid to keep the selection visible, like the arrow keys.
- [ ] Works on Windows XP.
- [ ] README's controls table lists the gamepad buttons.

## Comments
