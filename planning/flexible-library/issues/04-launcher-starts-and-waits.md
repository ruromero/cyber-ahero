---
type: ticket
status: open
tags: [library, kiosk]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/02-one-library-for-every-laptop]]", "[[flexible-library/issues/03-idle-menu-low-cpu]]"]
---

# 04: The launcher starts games and waits for them

**What to build:** When a Player presses JUGAR, the launcher starts the game itself: executables with the Entry's `args=` and working folder `dir=` (relative to the executable's folder or absolute; by default the executable's folder), shortcuts through the system shell. It keeps hold of the started process, hides its window, stops rendering and waits. When the game ends, the window comes back, the Library is rescanned and the menu redraws.

For games that start a helper and exit straight away, `wait=` in the Entry's options names the real game's executable; the launcher waits, with a time limit, for that process to appear and then waits for it instead.

This works with or without Kiosk mode.

**Blocked by:** [[flexible-library/issues/02-one-library-for-every-laptop]], [[flexible-library/issues/03-idle-menu-low-cpu]]

## Acceptance criteria

- [ ] A game with `args=` receives its arguments; one with `dir=` starts in that folder; without `dir=` it starts in its executable's folder.
- [ ] While a game runs, the launcher window is not visible over the game and the launcher uses close to 0% CPU.
- [ ] When the game exits, the menu returns on its own and an Entry installed in the meantime shows up (rescan).
- [ ] A game that launches through a helper (exits immediately) is waited for correctly when `wait=` is set; if the named process never appears within the time limit, the menu comes back.
- [ ] A shortcut-based Entry still launches and the menu returns when the game ends, where the shell provides a process to wait on.
- [ ] If a game can't be started, the menu shows a clear message naming the Entry.
- [ ] README documents `args=`, `dir=` and `wait=`.

## Comments
