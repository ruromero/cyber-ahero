---
type: ticket
status: open
tags: [library]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/01-show-entries-with-launch-target]]"]
---

# 02: One Library for every laptop

**What to build:** The Organizer copies one Master Library to every laptop. Each laptop's configuration lists its Install folders with `installs=` (separated by `;`). An Entry's `exe=` may be relative, and is then looked for in each Install folder in listed order. If it isn't found there, the executable's file name is searched up to two folder levels deep inside each Install folder, in listed order. Absolute paths and shortcuts keep working. A per-laptop override, `exe.<Entry folder name>=<absolute path>` in the laptop configuration, wins over everything else.

Resolution order, first match wins: per-laptop override; absolute `exe=`; relative `exe=` in each Install folder; file-name search in each Install folder; shortcut with an existing target; automatic choice inside the Entry folder.

**Blocked by:** [[flexible-library/issues/01-show-entries-with-launch-target]]

## Acceptance criteria

- [ ] With `installs=C:\Juegos;D:\Games` and an Entry `exe=Quake3\quake3.exe`, the Entry is Available when the game is in either folder, and the first listed folder wins when it is in both.
- [ ] With the game installed under a differently named folder (e.g. `Quake 3 Arena\quake3.exe`), the Entry is still Available through the file-name search.
- [ ] An executable more than two levels deep inside an Install folder is not found by the search.
- [ ] With the game in no Install folder, the Entry is hidden with reason "Installation not found".
- [ ] A per-laptop override points one Entry at an absolute path and wins over its `exe=`.
- [ ] Absolute `exe=` paths and shortcuts behave as in ticket 01.
- [ ] Scanning a Library of about 40 Entries on a vintage laptop does not visibly delay start-up.
- [ ] README documents `installs=`, relative `exe=`, the search and the override, with an example of one Library on two laptops.

## Comments
