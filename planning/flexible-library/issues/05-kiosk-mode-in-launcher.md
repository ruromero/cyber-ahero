---
type: ticket
status: done
tags: [kiosk, build]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/04-launcher-starts-and-waits]]"]
---

# 05: Kiosk mode inside the launcher

**What to build:** Following ADR 0001, the launcher itself provides Kiosk mode, so laptops need nothing else installed. Started with the Kiosk switch, it applies the lockdown that the PowerShell script applies today: Task Manager, Run dialog and Windows key disabled (for the user, and for the machine with administrator rights), Start button removed, desktop shell not restarted by Windows, Ctrl+Alt+Del disabled (administrator), automatic start registered in this user's standard Run key (HKCU, so it works without administrator rights), and the desktop shell closed and kept closed. The keyboard is blocked by a low-level hook only while the menu is showing; it is released while a game runs.

A visible Quit button undoes every setting, removes the automatic start (also from the machine Run key and the wrong key older versions used), restarts the desktop shell and exits, without logging off. A second switch undoes Kiosk mode with no UI, for recovery.

The double-click scripts become thin wrappers using only what XP ships with: the kiosk script runs the launcher in Kiosk mode and starts it again whenever it exits with anything but the Quit exit code; the cleanup script runs the recovery switch. Kiosk mode writes a plain-text log next to the launcher. The PowerShell kiosk and cleanup scripts and the separate keyboard blocker are removed, and the build stops producing and copying them.

Without the Kiosk switch the launcher stays an ordinary window and Quit just exits.

**Blocked by:** [[flexible-library/issues/04-launcher-starts-and-waits]]

## Acceptance criteria

- [ ] On a Windows XP laptop with no PowerShell, double-clicking the kiosk script locks the laptop: no desktop, taskbar or Start menu; Win key, Run, Task Manager blocked.
- [ ] Keys do nothing in the menu; inside a launched game the keyboard works; back in the menu keys are blocked again.
- [ ] Killing the launcher process in Kiosk mode brings it back within a few seconds.
- [ ] After a reboot and logon, Kiosk mode starts by itself.
- [ ] Quit restores the desktop immediately without logging off; after the next logon every setting is back to normal and Kiosk mode no longer starts automatically.
- [ ] The cleanup script restores a locked laptop even when the menu is not running.
- [ ] Without administrator rights, Kiosk mode still starts and the log says which settings could not be applied.
- [ ] The log records settings applied, games started and ended, and failures.
- [ ] The same checklist passes on a modern 64-bit laptop.
- [x] The repo and build no longer contain the PowerShell kiosk scripts or the separate keyboard blocker.
- [x] README's Kiosk mode section describes the new behaviour and recovery.

## Comments

- Built test-first in `tests/test_kiosk.c` against portable `src/kiosk.c`: `cli_parse` (`--kiosko`, `--limpiar` wins over it, any other `--` switch ignored; the first non-switch argument is still the Library root, in any order), the lockdown plan applied by `kiosk_enter` / undone by `kiosk_leave` / `kiosk_recover` through an injected `KioskApi` (registry set/remove, stop/start shell, stop other launchers, log) with a fake registry: full lockdown with admin; without admin the machine-scope settings are reported "NO aplicado (hace falta administrador)" and the rest still applies; enter then leave gives back exactly the registry it started from (including XP's stock `AutoRestartShell=1`); leave removes `CiberAhKiosko` from `HKLM\...\Windows\CurrentVersion\Run` and the wrong `HKLM\...\Windows NT\CurrentVersion\Run`; recovery ends running launchers first. Log lines come from `kiosk_log_line` ("YYYY-MM-DD HH:MM:SS  msg\r\n") and `kiosk_game_msg`.
- Deviation from the PowerShell script: the automatic start is registered in **HKCU** `Software\Microsoft\Windows\CurrentVersion\Run` (value `CiberAhKiosko` = `"<exe dir>\kiosko.bat"`), so it works without administrator rights ("Kiosk mode still starts"); the HKLM Run value of older versions is removed on Quit/recovery. `AutoRestartShell` in HKLM is set back to 1 (Windows' default) rather than deleted; everything else is deleted, as `limpiador.ps1` did.
- Win32 side in `src/kiosk_win.c` (`kiosk_win()`, `kiosk_win_log`, `kiosk_win_keyboard`): `RegCreateKeyExA`/`RegSetValueExA`/`RegDeleteValueA` (HKLM with `KEY_WOW64_64KEY` so the 32-bit build writes the real keys on 64-bit Windows), explorer.exe closed with Toolhelp + `TerminateProcess` and kept closed by a thread that closes it every 2 s until Quit, restarted with `CreateProcessA(%WINDIR%\explorer.exe)`; the `WH_KEYBOARD_LL` hook runs on its own thread with its own message loop. All imports checked in the i686 `-mcrtdll=msvcrt-os` build: msvcrt.dll only, no api-ms-win-crt, no post-XP Win32 functions.
- `main.c` parses the switches; `--limpiar` runs `kiosk_recover` with no UI and exits 0. `--kiosko` enters before the menu; Quit makes `app_run` return `APP_QUIT`, then main leaves Kiosk mode and exits with `KIOSK_EXIT_QUIT` (10). Any other exit leaves the lockdown in place and `kiosko.bat` starts the launcher again after ~2 s. Recovery terminates running launchers with exit code 10, so the `kiosko.bat` loop stops too.
- `app.c`: `app_in_kiosk` returns the mode (F5 off in Kiosk mode); Esc no longer quits in Kiosk mode; a Quit button (X) left of the Organizer button works in both modes (outside Kiosk it just exits). Ticket 04's flow: the keyboard is released in `hide_menu` (once the game has started) and blocked again after the wait; games started/ended/failed are logged. `src/launch.c` untouched.
- Not verifiable here (no Windows/Wine; cross-builds and unit tests only), left unticked for a run on an XP laptop and a modern 64-bit laptop: the lockdown actually taking effect (no desktop/taskbar/Start, Win key/Run/Task Manager blocked), keys swallowed in the menu and working in a game, the relaunch after killing the process, autostart after reboot/logon, Quit restoring the desktop without logoff and the next logon being normal, `limpiar.bat` recovering a locked laptop, the log contents on a non-admin account, and the `.bat` quoting/`start /min` behaviour under XP's cmd.exe.
- Pending manual check on Windows: On a Windows XP laptop with no PowerShell, double-clicking the kiosk script locks the laptop: no desktop, taskbar or Start menu; Win key, Run, Task Manager blocked; Keys do nothing in the menu; inside a launched game the keyboard works; back in the menu keys are blocked again; Killing the launcher process in Kiosk mode brings it back within a few seconds; After a reboot and logon, Kiosk mode starts by itself; Quit restores the desktop immediately without logging off; after the next logon every setting is back to normal and Kiosk mode no longer starts automatically; The cleanup script restores a locked laptop even when the menu is not running; Without administrator rights, Kiosk mode still starts and the log says which settings could not be applied; The log records settings applied, games started and ended, and failures; The same checklist passes on a modern 64-bit laptop.
- Review decision: the per-user (HKCU) Run key is kept on purpose, since Kiosk mode is for fun and must work without administrator rights. Quit and recovery still remove the HKLM value and the wrong `Windows NT` key of older versions.
- Review fix: the Kiosk log is now the general troubleshooting log `cyber.log` (`src/cyberlog.c`, written by `cyber_log` in `src/cyberlog_win.c`), which replaces `kiosko.log`. `kiosk_log_line` / `kiosk_game_msg` / `kiosk_win_log` became `cyberlog_line` / `cyberlog_game` / `cyber_log`, and the game lines now also give the exact path launched.
