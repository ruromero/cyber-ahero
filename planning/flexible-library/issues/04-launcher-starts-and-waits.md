---
type: ticket
status: done
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

- [x] A game with `args=` receives its arguments; one with `dir=` starts in that folder; without `dir=` it starts in its executable's folder.
- [ ] While a game runs, the launcher window is not visible over the game and the launcher uses close to 0% CPU.
- [ ] When the game exits, the menu returns on its own and an Entry installed in the meantime shows up (rescan).
- [x] A game that launches through a helper (exits immediately) is waited for correctly when `wait=` is set; if the named process never appears within the time limit, the menu comes back.
- [ ] A shortcut-based Entry still launches and the menu returns when the game ends, where the shell provides a process to wait on.
- [x] If a game can't be started, the menu shows a clear message naming the Entry.
- [x] README documents `args=`, `dir=` and `wait=`.

## Comments

- Built test-first. Seam 1 (`tests/test_library.c`): each Available Entry carries `args`, `work_dir` (default the executable's folder; `dir=` relative to it or absolute; NULL for shortcuts, which keep their own) and `wait` (new `Game` fields in `src/library.h`). Launch flow (`tests/test_launch.c`): portable `src/launch.c` (`launch_and_wait`, `launch_command_line`) over an injected `ProcApi` with a fake clock: command line `"<exe>" <args>`, working folder, failure without hiding or waiting, `wait=` poll (every 500 ms, limit 60 s, name reduced to a bare `*.exe`), fallback to the started process when the named one never appears, shortcuts with and without a shell process handle.
- Win32 side: `src/launch_win.c` (`proc_win()`): `CreateProcessA`, `ShellExecuteExA` with `SEE_MASK_NOCLOSEPROCESS`, `CreateToolhelp32Snapshot`/`Process32First`/`Process32Next` + `OpenProcess(SYNCHRONIZE)`, and `MsgWaitForMultipleObjects(QS_ALLINPUT)` with a `PeekMessage` pump, used for both the game wait and the sleeps of the `wait=` poll. All exist on XP SP3 (checked in the i686 import table). `src/launcher.c/.h` (`ShellExecuteA`, no handle) were removed.
- `app.c`: `launch()` hides the window only once the game has started (`on_started` callback), blocks in `launch_and_wait` (no SDL loop, no rendering), then shows/raises the window, flushes key/mouse/pad input typed into the game, calls `app_rescan(a)` (ticket 06's single definition: frees and reallocates the texture caches, refreshes the Organizer rows), and the loop redraws. Ticket 10's hook is wired: `attract_game_running(&a->attract, 1, now)` when the game has started (stopping the preview if Attract mode was showing) and `(…, 0, now)` after the wait; the window-focus stand-in from ticket 10 was removed. On failure the status reads *No se pudo iniciar <name>*. A shortcut-based Entry passes `args=` as shell parameters.
- Ticket 09 hook: comments in `launch_and_wait` (`src/launch.c`) mark where the mapper helper starts (just before the game) and is closed (after the wait).
- Not verifiable here (no Windows/Wine on the dev host; cross-builds only), left unticked for a Windows/XP run: launcher window not visible over the game and ~0% CPU while it runs; the menu returns on its own and an Entry installed meanwhile shows up; shortcut-based Entries are waited for where the shell gives a process (a `.lnk` to an `.exe` should; others return at once unless `wait=` is set). The `wait=` time-limit flow is unit-tested; the real Toolhelp process lookup is not.
- A process matching `wait=` that was already running before JUGAR (e.g. a second copy) would be taken as the game. Not handled.
