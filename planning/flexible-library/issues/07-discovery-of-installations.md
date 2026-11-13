---
type: ticket
status: open
tags: [discovery]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/02-one-library-for-every-laptop]]"]
---

# 07: Discovery of new Installations

**What to build:** The Organizer runs a discovery mode of the existing scraper tool on a modern PC that holds the master copy of the Installations. Each top-level folder of an Install folder that no Entry's Launch target resolves into is a candidate. Discovery picks the most likely executable using the launcher's rules (prefer names like the folder; avoid installers, uninstallers, setup, servers and similar), matches the game in the online database, and writes a new Entry into the Master Library: options with a relative `exe=` (and `args=` if given), plus cover, details, description and, on request, a preview video. It asks on the console only when the match or the executable is ambiguous: name, which executable, launch arguments.

A dry run prints the planned Entries without writing. Existing Entries are only rewritten when explicitly asked.

**Blocked by:** [[flexible-library/issues/02-one-library-for-every-laptop]]

## Acceptance criteria

- [ ] With two Install folders containing five games, two of which already have Entries, discovery proposes exactly the three others.
- [ ] For an unambiguous game, discovery writes a complete Entry without asking anything.
- [ ] For an ambiguous one, it asks only the questions it can't answer and uses the answers.
- [ ] A written Entry is Available in the launcher on another laptop whose Install folder has the game under the same folder name.
- [ ] The dry run writes nothing.
- [ ] Running discovery twice creates no duplicates and doesn't touch existing Entries unless asked.
- [ ] README documents discovery and its options.

## Comments
