# Labels

The triage roles (needs-triage, needs-info, ready-for-agent, ready-for-human, wontfix)
are not used in this repo. When a skill says to apply a triage label, skip that step.

Notes carry category tags instead, in the `tags:` frontmatter:

| Tag | Covers |
| --- | --- |
| `library` | Entries, Install folders, Launch targets, the library scan and check |
| `kiosk` | Kiosk mode: lockdown, launching and waiting, Quit and Organizer buttons |
| `discovery` | The Organizer tool that finds Installations and creates Entries |
| `gamepad` | Menu navigation by pad and the in-game mapper helper |
| `ui` | Menu, Info panel, themes, Attract mode |
| `build` | Toolchain, build and packaging scripts |
| `docs` | README, glossary, ADRs |

Add a tag here before using a new one.
