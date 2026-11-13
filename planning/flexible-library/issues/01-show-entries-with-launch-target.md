---
type: ticket
status: open
tags: [library]
spec: "[[flexible-library/spec]]"
---

# 01: Show only Entries with a working Launch target

**What to build:** A Player only sees Entries that can actually start. An Entry is listed when it has a Launch target that exists on this laptop: any shortcut in the Entry folder whose target file exists (no longer only one named `game.lnk`), an `exe=` in the Entry's options that exists, or the existing automatic choice of an executable in the Entry folder. Entries without a Launch target are hidden.

Prefactor first: the laptop configuration (`cyber.cfg`) is read once into one laptop configuration, passed to the library scan, instead of re-reading the file for each key. This gives ticket 02 a place to add Install folders.

The library scan returns both the Available Entries and the hidden ones with a reason ("no launch information" or "Installation not found"), even though only the first list is shown for now. Ticket 06 will display the second.

**Blocked by:** None (can start immediately)

## Acceptance criteria

- [ ] An Entry whose only shortcut has a name other than `game.lnk` is listed and starts.
- [ ] An Entry whose shortcut points to a file that no longer exists is hidden.
- [ ] An Entry with `exe=` in its options that exists, and no shortcut, is listed and starts.
- [ ] An Entry folder with only a cover and details (no launch information) is hidden.
- [ ] A shortcut whose target can't be read is treated as Available.
- [ ] The scan result carries hidden Entries with their reason.
- [ ] The laptop configuration is read once at start-up; the Library root is still chosen in the same order as today (argument, environment variable, configuration, default).
- [ ] README describes the new listing rule; the old `game.lnk` requirement is gone from code and docs.

## Comments
