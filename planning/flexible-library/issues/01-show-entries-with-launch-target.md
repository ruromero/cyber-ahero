---
type: ticket
status: done
tags: [library]
spec: "[[flexible-library/spec]]"
---

# 01: Show only Entries with a working Launch target

**What to build:** A Player only sees Entries that can actually start. An Entry is listed when it has a Launch target that exists on this laptop: any shortcut in the Entry folder whose target file exists (no longer only one named `game.lnk`), an `exe=` in the Entry's options that exists, or the existing automatic choice of an executable in the Entry folder. Entries without a Launch target are hidden.

Prefactor first: the laptop configuration (`cyber.cfg`) is read once into one laptop configuration, passed to the library scan, instead of re-reading the file for each key. This gives ticket 02 a place to add Install folders.

The library scan returns both the Available Entries and the hidden ones with a reason ("no launch information" or "Installation not found"), even though only the first list is shown for now. Ticket 06 will display the second.

**Blocked by:** None (can start immediately)

## Acceptance criteria

- [x] An Entry whose only shortcut has a name other than `game.lnk` is listed and starts.
- [x] An Entry whose shortcut points to a file that no longer exists is hidden.
- [x] An Entry with `exe=` in its options that exists, and no shortcut, is listed and starts.
- [x] An Entry folder with only a cover and details (no launch information) is hidden.
- [x] A shortcut whose target can't be read is treated as Available.
- [x] The scan result carries hidden Entries with their reason.
- [x] The laptop configuration is read once at start-up; the Library root is still chosen in the same order as today (argument, environment variable, configuration, default).
- [x] README describes the new listing rule; the old `game.lnk` requirement is gone from code and docs.

## Comments

- Built test-first. The scan now goes through an `Fs` seam (`src/fs.h`): `src/fs_win.c` holds the Win32 implementation (FindFirstFileA, GetFileAttributesA, IShellLink::GetPath via ole32, all present on XP), and `tests/fake_fs.c` is an in-memory tree. `tests/test_library.c` covers Seam 1 (listing rule, hidden reasons, any shortcut name, stale and unreadable shortcuts, `exe=` absolute/relative/missing, auto executable in a subfolder, sorting). `tests/test_config.c` covers reading `cyber.cfg` and the Library root order.
- Verified by reasoning only, not run on Windows (no Windows or Wine on the dev host): that the listed Entries actually *start* (launching code in app.c is unchanged and still opens `shortcut` or `exe`; exactly one is set now), and that the real IShellLink returns the stored target for ordinary shortcuts and nothing for shortcuts to non-file shell items (these count as unreadable, so Available).
- The laptop configuration is `LaptopConfig` (`src/config.h`), parsed once in `main.c` and passed to `app_run` and `library_scan`. Ticket 02 reads `installs=` from it. `library_scan` ignores it for now.
- An `exe=` that exists now wins over shortcuts (the spec's resolution order). Before, a shortcut won over `exe=`.
- `tools/kiosko.ps1` still says `game.lnk` in two comments. Its code already uses any `.lnk`, and ticket 05 deletes the file, so it was left alone.
