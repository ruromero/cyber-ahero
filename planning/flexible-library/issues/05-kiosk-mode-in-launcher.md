---
type: ticket
status: open
tags: [kiosk, build]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/04-launcher-starts-and-waits]]"]
---

# 05: Kiosk mode inside the launcher

**What to build:** Following ADR 0001, the launcher itself provides Kiosk mode, so laptops need nothing else installed. Started with the Kiosk switch, it applies the lockdown that the PowerShell script applies today: Task Manager, Run dialog and Windows key disabled (for the user, and for the machine with administrator rights), Start button removed, desktop shell not restarted by Windows, Ctrl+Alt+Del disabled (administrator), automatic start registered in the standard Run key, and the desktop shell closed and kept closed. The keyboard is blocked by a low-level hook only while the menu is showing; it is released while a game runs.

A visible Quit button undoes every setting, removes the automatic start (also from the wrong key older versions used), restarts the desktop shell and exits, without logging off. A second switch undoes Kiosk mode with no UI, for recovery.

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
- [ ] The repo and build no longer contain the PowerShell kiosk scripts or the separate keyboard blocker.
- [ ] README's Kiosk mode section describes the new behaviour and recovery.

## Comments
