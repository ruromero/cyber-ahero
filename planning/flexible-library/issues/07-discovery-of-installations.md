---
type: ticket
status: done
tags: [discovery]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/02-one-library-for-every-laptop]]"]
---

# 07: Discovery of new Installations

**What to build:** The Organizer runs a discovery mode of the existing scraper tool on a modern PC that holds the master copy of the Installations. Each top-level folder of an Install folder that no Entry's Launch target resolves into is a candidate. Discovery picks the most likely executable using the launcher's rules (prefer names like the folder; avoid installers, uninstallers, setup, servers and similar), matches the game in the online database, and writes a new Entry into the Master Library: options with a relative `exe=` (and `args=` if given), plus cover, details, description and, on request, a preview video. It asks on the console only when the match or the executable is ambiguous: name, which executable, launch arguments.

A dry run prints the planned Entries without writing. Existing Entries are only rewritten when explicitly asked.

**Blocked by:** [[flexible-library/issues/02-one-library-for-every-laptop]]

## Acceptance criteria

- [x] With two Install folders containing five games, two of which already have Entries, discovery proposes exactly the three others.
- [x] For an unambiguous game, discovery writes a complete Entry without asking anything.
- [x] For an ambiguous one, it asks only the questions it can't answer and uses the answers.
- [ ] A written Entry is Available in the launcher on another laptop whose Install folder has the game under the same folder name.
- [x] The dry run writes nothing.
- [x] Running discovery twice creates no duplicates and doesn't touch existing Entries unless asked.
- [x] README documents discovery and its options.

## Comments

- Built test-first with Pester (`tools/tests/scrape.Discover.Tests.ps1`, run with `Invoke-Pester tools/tests`). The tests run the whole script with `-Discover` over a temp tree of fake Installations and a fake Master Library. The online database (`-Lookup`), the cover download (`-Download`) and the console question (`-Ask`) are injected as scriptblocks, so the tests use no network. They cover candidates (relative `exe=`, the file-name search, the per-laptop override and `installs=` from the laptop configuration), the executable choice, the questions asked only when unsure, dry run, re-run, `-Force`, and a written Entry resolving against a second laptop's Install folder.
- The resolution mirror is `Resolve-EntryTarget` in `tools/scrape.ps1`. It follows `resolve_launch_target` in `src/library.c` in the same order, and `Get-ExeScore` ports `score_exe`. An Installation counts as covered when the resolved Launch target lies inside it. A folder name covered in one Install folder also counts as covered in the others, because a relative `exe=` would reach either copy, so a second copy is not a new game. Candidates with the same folder name in two Install folders are proposed once, from the first.
- Executable choice: the candidate's own `.exe` files, else one folder level below, else two. The choice is sure when there is one executable, when the best one has a folder-name word (score >= 20) and beats the rest, or when it is the only one without a penalty. Otherwise discovery asks which executable and then the launch arguments. Arguments are only asked together with an ambiguous executable, because nothing else on disk says the game needs them.
- Match: sure when exactly one database result has the folder's cleaned name. Otherwise discovery asks: a number picks a result, a typed name searches again, Enter writes the Entry without database details.
- The Entry folder takes the Installation folder's name. `nombre=` is written when the database name differs. "Existing Entries are only rewritten when explicitly asked" uses the scraper's existing `-Force`. A candidate whose Entry folder name already exists is skipped without it. The dry run asks nothing and prints the questions it would ask.
- Fixed in passing: `$root` (repo folder) and the `-Root` parameter are the same variable in PowerShell, so `-Root` and `library=` were always overwritten by the repo folder. The repo folder is now `$repoRoot`. `library=` now takes the first value, like the launcher. The normal scrape mode now shares `Write-GameInfo` / `Select-BestGame` with discovery and accepts the same `-Lookup` / `-Download` seams. It was smoke-tested here with a fake database only.
- Not verifiable here (no Windows, no IGDB credentials used):
  - "A written Entry is Available in the launcher on another laptop" was checked only through the script's mirror of the launcher's rules (the written Entry covers the same folder under another Install folder). A run of the real launcher on a second laptop is still needed, so the criterion stays unticked.
  - Windows PowerShell 5.1 itself wasn't run. The script avoids PS7-only syntax and parses cleanly, but a run on Windows (paths with `\` and drive letters, `.lnk` targets through `WScript.Shell`, which off Windows count as unreadable) is still needed.
  - The real IGDB/SteamGridDB lookups and `-Video` (yt-dlp + ffmpeg) in discovery mode.
- Pending manual check on Windows: A written Entry is Available in the launcher on another laptop whose Install folder has the game under the same folder name.
