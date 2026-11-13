---
type: ticket
status: done
tags: [gamepad]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/04-launcher-starts-and-waits]]"]
---

# 09: Gamepad mapper helper for games

**What to build:** For old games that only understand the keyboard, an Entry can name an external gamepad-to-keyboard mapper (for example JoyToKey) and its arguments, typically a profile, with `con=` and `con_args=`. The launcher starts the helper just before the game and closes it when the game ends.

**Blocked by:** [[flexible-library/issues/04-launcher-starts-and-waits]]

## Acceptance criteria

- [ ] With `con=` set, the helper is running while the game runs and pad presses reach the game as keys.
- [x] When the game ends, the helper is closed before the menu returns.
- [x] Entries without `con=` behave exactly as before.
- [x] If the helper can't be started, the game still starts and the log or status message says so.
- [ ] Works on Windows XP with JoyToKey.
- [x] README documents `con=`/`con_args=` with a JoyToKey example.

## Comments

- Built test-first. Seam 1 (`tests/test_library.c`): each Available Entry carries `con` and `con_args` (new `Game` fields). **`con=` resolves like `exe=`**: absolute as-is; relative in each Install folder in listed order, then relative to the Entry folder or its `juego.cfg`, then by file name two levels deep inside the Install folders. Unlike `exe=`, a `con=` that isn't found never hides the Entry: the Entry-relative path is kept and starting it fails at JUGAR, which is reported. No per-laptop override for `con=`.
- Launch flow (`tests/test_launch.c`, fake `ProcApi`): `launch_and_wait` gained an `int *helper_failed` out-parameter and `LaunchTarget` gained `con`/`con_args`; `ProcApi` gained `start_helper`/`stop_helper`. The helper starts (command line `"<con>" <con_args>`, working folder = the helper's own folder, where JoyToKey keeps its profiles) just before the game; it is stopped after the game (or the `wait=` process) ends, and also when the game fails to start. If the helper fails, the game still starts. No `con=`: no extra calls, same order as before.
- Win32 (`src/launch_win.c`): the helper handle keeps the process id; `stop_helper` posts `WM_CLOSE` to every top-level window of the process (`EnumWindows` + `GetWindowThreadProcessId`, hidden tray windows included), waits up to 2 s pumping messages, then `TerminateProcess`. All XP APIs (checked in the i686 import table built with `-mcrtdll=msvcrt-os`: msvcrt.dll, no api-ms-win-crt).
- `app.c` (3 small edits in `launch()`): passes `con`/`con_args`; on helper failure the message *No se pudo iniciar el mapeador del mando de <juego>* is built before the rescan (which frees the Entry), shown as status when the menu returns, and written with `SDL_Log`.
- Not verifiable here (no Windows/Wine; cross-builds only), left unticked for a Windows/XP run: the helper is running while the game runs and pad presses reach the game as keys; JoyToKey on XP. Also worth checking there: that JoyToKey exits on `WM_CLOSE` rather than minimising to the tray (if not, it is terminated after 2 s, which is fine), and that `con_args=<profile>` selects the profile in the JoyToKey version used.
- Known limit: if a copy of the mapper is already running (e.g. in Windows startup), a single-instance mapper may hand the profile to that copy and exit; the launcher then has nothing to close and the old copy stays. README advises not autostarting it.
- Pending manual check on Windows: With `con=` set, the helper is running while the game runs and pad presses reach the game as keys; Works on Windows XP with JoyToKey.
