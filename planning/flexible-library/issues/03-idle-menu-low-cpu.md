---
type: ticket
status: done
tags: [ui]
spec: "[[flexible-library/spec]]"
---

# 03: Idle menu uses almost no CPU

**What to build:** The menu only redraws when something changes: input, a preview that is playing, an animation in progress, or a status message on screen. When a Player is just looking at a still menu, the launcher waits for the next event instead of drawing 60 frames a second. Everything looks and responds as it does today.

**Blocked by:** None (can start immediately)

## Acceptance criteria

- [ ] With the menu idle and no preview playing, the launcher uses close to 0% CPU in Task Manager.
- [ ] Hovering a game still starts its preview after the usual delay, and the preview plays smoothly.
- [ ] Hover, selection, theme switching, scrolling, status messages and the grace period when leaving a tile all look and behave as before.
- [ ] Resizing and toggling fullscreen still redraw correctly.
- [ ] Works with the software renderer used on XP laptops without 3D acceleration.

## Comments

- Implemented: portable `src/redraw.c` decides how long the loop may wait
  (forever when still; 0 while a preview plays; until the preview delay,
  status expiry or hover-grace end otherwise), unit-tested in
  `tests/test_redraw.c`. `app_run` calls `SDL_WaitEventTimeout(NULL, ...)`
  (waits without removing the event) at the top of each loop iteration; the
  existing 16 ms frame cap and event handling are unchanged. Renderer-agnostic,
  so the software renderer path should be unaffected (only SDL calls, no new
  Win32 imports); not run on an XP laptop.
- Not verified at runtime (no Windows/Wine on the dev Mac; cross-builds only):
  Task Manager CPU near 0% when idle, preview delay/smoothness, hover/selection/
  theme/scroll/status/grace behaviour, and resize/fullscreen redraw. By reasoning:
  every SDL event (incl. window resize/expose, mouse, keys) wakes the loop and
  draws a frame; time-driven changes are scheduled from the same state `update()`
  checks; while a preview plays the loop runs at the previous 60 fps cap.
  Please tick these on a Windows/XP laptop.
