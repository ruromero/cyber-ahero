---
type: ticket
status: done
tags: [library]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/01-show-entries-with-launch-target]]"]
---

# 02: One Library for every laptop

**What to build:** The Organizer copies one Master Library to every laptop. Each laptop's configuration lists its Install folders with `installs=` (separated by `;`). An Entry's `exe=` may be relative, and is then looked for in each Install folder in listed order. If it isn't found there, the executable's file name is searched up to two folder levels deep inside each Install folder, in listed order. Absolute paths and shortcuts keep working. A per-laptop override, `exe.<Entry folder name>=<absolute path>` in the laptop configuration, wins over everything else.

Resolution order, first match wins: per-laptop override; absolute `exe=`; relative `exe=` in each Install folder; file-name search in each Install folder; shortcut with an existing target; automatic choice inside the Entry folder.

**Blocked by:** [[flexible-library/issues/01-show-entries-with-launch-target]]

## Acceptance criteria

- [x] With `installs=C:\Juegos;D:\Games` and an Entry `exe=Quake3\quake3.exe`, the Entry is Available when the game is in either folder, and the first listed folder wins when it is in both.
- [x] With the game installed under a differently named folder (e.g. `Quake 3 Arena\quake3.exe`), the Entry is still Available through the file-name search.
- [x] An executable more than two levels deep inside an Install folder is not found by the search.
- [x] With the game in no Install folder, the Entry is hidden with reason "Installation not found".
- [x] A per-laptop override points one Entry at an absolute path and wins over its `exe=`.
- [x] Absolute `exe=` paths and shortcuts behave as in ticket 01.
- [ ] Scanning a Library of about 40 Entries on a vintage laptop does not visibly delay start-up.
- [x] README documents `installs=`, relative `exe=`, the search and the override, with an example of one Library on two laptops.

## Comments

- Built test-first through Seam 1: `tests/test_library.c` scans the fake file system with a `LaptopConfig` parsed from text (installs order, file-name search, depth limit, "Installation not found", override over `exe=` and over shortcuts, absolute `exe=` and shortcuts with Install folders set). `tests/test_config.c` covers splitting `installs=`.
- `src/config.c` gains `laptop_config_installs` / `laptop_config_list_free` and `laptop_config_exe_override` (block at the end of the file). No change to `app.c` or `main.c`: they already pass the `LaptopConfig` to `library_scan`.
- A relative `exe=` that is not in any Install folder is still tried relative to the Entry folder (and its `juego.cfg`), as in ticket 01, before the file-name search, so existing Entries that carry their game inside the Entry folder keep working.
- The file-name search only applies to a relative `exe=`. A missing absolute `exe=` still hides the Entry, as in ticket 01. An override whose file doesn't exist is skipped (first match wins) and the rest of the order is tried. Override keys use the Entry folder name, not `nombre=`.
- The search covers the Install folder itself and two folder levels below it. Hidden folders are skipped. It lists each Install folder tree once per scan, on first need, and then matches in memory. It does no per-Entry directory walks.
- Not verifiable here (no Windows or Wine on the dev host): "Scanning a Library of about 40 Entries on a vintage laptop does not visibly delay start-up" needs a run on the XP laptop. It is left unticked. Cost: one `FindFirstFileA` listing per folder up to two levels below each Install folder, done only when some Entry needs the search.
