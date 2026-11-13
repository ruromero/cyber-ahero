---
type: ticket
status: open
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
