---
type: ticket
status: done
tags: [ui]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/01-show-entries-with-launch-target]]", "[[flexible-library/issues/03-idle-menu-low-cpu]]"]
---

# 10: Attract mode

**What to build:** When nobody touches the launcher for the configured time (`attract=` minutes in the laptop configuration, 3 by default, 0 to turn it off), it starts Attract mode: Available Entries cycle full screen in Library order, each for 20 seconds or until its preview ends, whichever comes first; Entries without a preview show their cover for 8 seconds. Any mouse, keyboard or gamepad input returns to the menu and is consumed, so it never starts a game. Attract mode never starts while a game is running.

**Blocked by:** [[flexible-library/issues/01-show-entries-with-launch-target]], [[flexible-library/issues/03-idle-menu-low-cpu]]

## Acceptance criteria

- [x] After 3 idle minutes with default settings, Attract mode starts; with `attract=0` it never does; with `attract=1` it starts after one minute.
- [x] Each Entry shows for 20 seconds or until its preview ends; cover-only Entries for 8 seconds.
- [x] Only Available Entries appear.
- [x] Moving the mouse, pressing a key or a pad button returns to the menu without starting a game.
- [x] Attract mode does not start while a game is running.
- [ ] Playback stays smooth on a vintage XP laptop.
- [x] README documents `attract=`.

## Comments

- Timing lives in the portable `src/attract.c` (`attract.h`), with ticks passed in. `tests/test_attract.c` covers the idle start (3/1/0 minutes, wrap-around), 20 s per preview or until it ends, 8 s per cover, Library order and wrap, starting from the first Entry each time, input consumed and the idle timer reset, a 500 ms wake-up guard (one pad press arrives as both a joystick event and a controller event, and a double-click as two clicks, so the second one must not launch), no start while a game runs, and the redraw deadline. `attract=` is parsed by `laptop_config_attract_minutes()` in `config.c`: absent or invalid gives 3, `0` turns it off, values are capped at one week. Tested in `test_config`.
- `video.c` gained `video_end_count()`, which counts how often the preview reached its end and rewound. Attract mode moves on at the first end.
- app.c only shows Available Entries: since ticket 01, `lib->games` holds nothing else. `draw_attract` clears to black, then draws the preview (decoded as in the menu, audio off) or the cover scaled into the screen, with the Entry's name underneath. `menu_wait_ms` wakes at `attract_deadline()`, so an idle menu sleeps until Attract mode is due and a cover-only Entry draws once per 8 s. Only a playing preview animates.
- **Hook for ticket 04:** `attract_game_running(&a.attract, 1, now)` before the wait and `attract_game_running(&a.attract, 0, now)` after it. The idle time counts again from when the game ends, and the call returns 1 if Attract mode was showing, in which case the caller should `stop_video`. Calling it with 0 every frame is harmless. For now app.c uses window focus as a stand-in (`SDL_WINDOWEVENT_FOCUS_LOST` means a game is running, `FOCUS_GAINED` means it ended). Ticket 04 may replace that.
- Mouse motion events with zero relative movement are ignored, so synthetic or warp events don't wake the laptop. Stick drift doesn't count either: pad sticks and hats count only once they produce a menu action.
- **Windows-runtime-only, not checked here (no Wine):** that it starts and exits on real hardware with mouse, keyboard and pads, that FOCUS_LOST/GAINED fire as expected when a game is launched and closed on XP, and the "smooth on a vintage XP laptop" criterion (left unticked). The latter needs a check on the XP laptop (Seam 2 checklist).
