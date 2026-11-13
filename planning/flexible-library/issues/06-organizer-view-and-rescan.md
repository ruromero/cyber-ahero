---
type: ticket
status: open
tags: [library, ui]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/02-one-library-for-every-laptop]]"]
---

# 06: Organizer view and rescan

**What to build:** An Organizer button, next to the theme button, opens an Organizer view of this laptop's Library: the hidden Entries with their reason (no launch information, or Installation not found), and the Available Entries missing a cover, a preview or details. A Reescanear button in the view rescans the Library and refreshes both the menu and the view. Outside Kiosk mode, F5 also rescans.

**Blocked by:** [[flexible-library/issues/02-one-library-for-every-laptop]]

## Acceptance criteria

- [ ] The Organizer view lists every hidden Entry with the right reason.
- [ ] Entries missing a cover, a preview or details are listed with what is missing.
- [ ] Installing a game, then pressing Reescanear, makes its Entry Available and removes it from the hidden list without restarting.
- [ ] F5 rescans when not in Kiosk mode.
- [ ] The view is usable with the mouse alone and fits on a 1024x768 screen.
- [ ] README describes the Organizer view.

## Comments
