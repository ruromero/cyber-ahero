---
type: ticket
status: open
tags: [gamepad]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/04-launcher-starts-and-waits]]"]
---

# 09: Gamepad mapper helper for games

**What to build:** For old games that only understand the keyboard, an Entry can name an external gamepad-to-keyboard mapper (for example JoyToKey) and its arguments, typically a profile, with `con=` and `con_args=`. The launcher starts the helper just before the game and closes it when the game ends.

**Blocked by:** [[flexible-library/issues/04-launcher-starts-and-waits]]

## Acceptance criteria

- [ ] With `con=` set, the helper is running while the game runs and pad presses reach the game as keys.
- [ ] When the game ends, the helper is closed before the menu returns.
- [ ] Entries without `con=` behave exactly as before.
- [ ] If the helper can't be started, the game still starts and the log or status message says so.
- [ ] Works on Windows XP with JoyToKey.
- [ ] README documents `con=`/`con_args=` with a JoyToKey example.

## Comments
