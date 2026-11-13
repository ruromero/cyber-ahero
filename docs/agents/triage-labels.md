# Labels

The triage roles (needs-triage, needs-info, ready-for-agent, ready-for-human, wontfix)
are not used in this repo. When a skill says to apply a triage label, skip that step.

Issues carry category labels instead (GitHub labels, all already created):

| Label | Covers |
| --- | --- |
| `library` | Entries, Install folders, Launch targets, the library scan and check |
| `kiosk` | Kiosk mode: lockdown, launching and waiting, Quit and Organizer buttons |
| `discovery` | The Organizer tool that finds Installations and creates Entries |
| `gamepad` | Menu navigation by pad and the in-game mapper helper |
| `ui` | Menu, Info panel, themes, Attract mode |
| `build` | Toolchain, build and packaging scripts |
| `docs` | README, glossary, ADRs |

Plus `spec`, which marks a feature spec (the parent issue of its tickets).

Add a label here, and create it with `gh label create`, before using a new one.
