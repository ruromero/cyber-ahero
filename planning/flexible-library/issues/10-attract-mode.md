---
type: ticket
status: open
tags: [ui]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/01-show-entries-with-launch-target]]", "[[flexible-library/issues/03-idle-menu-low-cpu]]"]
---

# 10: Attract mode

**What to build:** When nobody touches the launcher for the configured time (`attract=` minutes in the laptop configuration, 3 by default, 0 to turn it off), it starts Attract mode: Available Entries cycle full screen in Library order, each for 20 seconds or until its preview ends, whichever comes first; Entries without a preview show their cover for 8 seconds. Any mouse, keyboard or gamepad input returns to the menu and is consumed, so it never starts a game. Attract mode never starts while a game is running.

**Blocked by:** [[flexible-library/issues/01-show-entries-with-launch-target]], [[flexible-library/issues/03-idle-menu-low-cpu]]

## Acceptance criteria

- [ ] After 3 idle minutes with default settings, Attract mode starts; with `attract=0` it never does; with `attract=1` it starts after one minute.
- [ ] Each Entry shows for 20 seconds or until its preview ends; cover-only Entries for 8 seconds.
- [ ] Only Available Entries appear.
- [ ] Moving the mouse, pressing a key or a pad button returns to the menu without starting a game.
- [ ] Attract mode does not start while a game is running.
- [ ] Playback stays smooth on a vintage XP laptop.
- [ ] README documents `attract=`.

## Comments
