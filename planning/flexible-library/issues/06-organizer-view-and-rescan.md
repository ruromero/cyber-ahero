---
type: ticket
status: done
tags: [library, ui]
spec: "[[flexible-library/spec]]"
blocked-by: ["[[flexible-library/issues/02-one-library-for-every-laptop]]"]
---

# 06: Organizer view and rescan

**What to build:** An Organizer button, next to the theme button, opens an Organizer view of this laptop's Library: the hidden Entries with their reason (no launch information, or Installation not found), and the Available Entries missing a cover, a preview or details. A Reescanear button in the view rescans the Library and refreshes both the menu and the view. Outside Kiosk mode, F5 also rescans.

**Blocked by:** [[flexible-library/issues/02-one-library-for-every-laptop]]

## Acceptance criteria

- [x] The Organizer view lists every hidden Entry with the right reason.
- [x] Entries missing a cover, a preview or details are listed with what is missing.
- [ ] Installing a game, then pressing Reescanear, makes its Entry Available and removes it from the hidden list without restarting.
- [ ] F5 rescans when not in Kiosk mode.
- [ ] The view is usable with the mouse alone and fits on a 1024x768 screen.
- [x] README describes the Organizer view.

## Comments

- Built test-first. The rows the view shows come from a portable module, `src/organizer.c` (`organizer_report(const Library *)`): hidden Entries first with their reason ("Oculto: sin información para lanzarlo" / "Oculto: instalación no encontrada"), then Available Entries lacking a cover, preview or details with "Falta: portada, vista previa, ficha" (only what is missing). Complete Entries are not listed. `tests/test_organizer.c` drives it through Seam 1 (library scan over the fake file system), including "install the game, scan again: the Entry becomes Available and leaves the hidden list".
- "Details" means a `details.txt` with at least one row (`meta_count > 0`). A `cover=`/`video=` in `juego.cfg` counts as present even if that file doesn't exist, because the scan keeps the joined path (library.c `resolve_cfg_value`).
- app.c: the rescan is one function, `app_rescan(App *a)` (stops the preview, frees the per-Entry icon/cover texture caches, frees and rescans the Library, reallocates the caches, rebuilds the Organizer rows if the view is open, shows "Biblioteca reescaneada: N juegos"). Ticket 04 is meant to call the same function after a game. Cache allocation/free moved into `app_caches_alloc` / `app_caches_free`, which start-up and shutdown use as well.
- Kiosk hook for ticket 05: `app_in_kiosk(const App *)` in app.c returns 0 for now; F5 is `if (!app_in_kiosk(&a)) app_rescan(&a);`. The Organizer button and the Reescanear button work in either mode.
- The view is modal: while open, clicks, wheel, keys (except F5, F11, Alt+Enter) and gamepad actions don't reach the menu behind it (pad B closes it). The title bar (theme button) keeps working. Opening it deselects the Entry, which stops the preview, so the ticket 03 event wait goes back to sleeping forever. No new timers.
- Not verifiable here (no Windows or Wine on the dev host), left unticked, checked only by reasoning and the cross-builds: that installing a game and pressing Reescanear in the running launcher makes it Available (the scan side is covered by `test_installing_a_game_then_rescanning_makes_it_available`); that F5 rescans; that the view is usable with the mouse alone and fits 1024x768 (layout: at 1024x768 the scale is 1.07, the panel is about 975x637 px below the title bar, the Reescanear and Cerrar buttons are bottom-right inside it, and the list pages with `^`/`v` buttons or the wheel). These belong to Seam 2's manual checklist.
