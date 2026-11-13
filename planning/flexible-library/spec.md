---
type: spec
status: done
tags: [library, kiosk, discovery, gamepad, ui]
---

# Flexible Library and self-contained Kiosk mode

Terms follow [[GLOSSARY]]. Kiosk decisions follow [[docs/adr/0001-launcher-owns-kiosk-mode|ADR 0001]].

## Problem Statement

The Organizer runs amateur LAN parties on a mix of modern and vintage (Windows XP 32-bit) laptops and wants each laptop to look and behave like a 2000s internet café terminal. Today that is hard:

- One Library can't serve several laptops. Entries point at their Installations through shortcuts holding absolute paths, so a Library prepared on one laptop is broken on the next, where games live on another drive or in another folder.
- An Entry only appears if its folder holds a shortcut named exactly `game.lnk`. Entries that say how to start the game in other ways (an executable, or `exe=` in the Entry's options) are silently left out, even though the documentation says they should work.
- When a laptop lacks a game, its Entry is still listed and simply fails when a Player presses JUGAR.
- Adding a new game means building its Entry folder by hand: shortcut, cover, details, preview.
- Kiosk mode needs PowerShell, which Windows XP doesn't include, so it can't run on the vintage laptops at all. It also guesses which new process is the game. Any unrelated program can be taken for one, and a game that starts through a helper process can be missed.
- The menu redraws constantly and uses about half the CPU, so Kiosk mode has to kill the launcher whenever a game starts.
- Leaving Kiosk mode means running a separate script that logs the user off, and the Organizer has no way to see from the menu why a game is missing.
- Players can only use mouse and keyboard, while many LAN-party games are more fun with gamepads, and the keyboard is blocked in the menu anyway.
- Between matches, idle laptops show a static menu rather than the lively café look.

## Solution

The Organizer keeps one Master Library and copies it to every laptop. Each laptop's own configuration lists its Install folders. An Entry names its Launch target relative to those folders, so the same Entry works wherever the game happens to be installed. If the folder is named differently on one laptop, the launcher still finds the game by its executable name. Absolute paths, shortcuts and a per-laptop override remain available for unusual cases. Players only ever see Available Entries; everything else is hidden.

The launcher itself becomes the Kiosk mode. Started in Kiosk mode, it locks the laptop down, starts games itself, waits for them while using almost no CPU, and comes back when they end. A visible Quit button leaves Kiosk mode and gives the desktop back. An Organizer button shows why Entries are hidden or incomplete and can rescan the Library. Nothing besides the launcher needs to be installed on a laptop.

An Organizer tool, run on a modern PC that holds the master copy of the Installations, discovers Installations that no Entry points to yet. It fetches their details and only asks the Organizer when it isn't sure. It then writes portable Entries into the Master Library.

Players can navigate the menu with a gamepad. Games can be paired with an external gamepad-to-keyboard mapper that starts and stops with the game. When nobody touches the launcher for a while, it plays an attract loop of the games, like an arcade cabinet.

## User Stories

### One Library for every laptop

1. As an Organizer, I want to copy the same Master Library to every laptop, so that I prepare the catalog once instead of once per laptop.
2. As an Organizer, I want to list a laptop's Install folders in that laptop's configuration, so that the Library doesn't need to know which drive each laptop uses.
3. As an Organizer, I want to list several Install folders on one laptop, so that games spread over two drives are all found.
4. As an Organizer, I want an Entry to name its executable relative to the Install folders, so that the same Entry launches the game on every laptop.
5. As an Organizer, I want the launcher to try the Install folders in the order I listed them, so that I control which copy wins when a game exists twice.
6. As an Organizer, I want the launcher to find a game by its executable name when its folder is named differently on one laptop, so that small differences between laptops don't break the Library.
7. As an Organizer, I want that name search to stay shallow, so that scanning stays fast on vintage laptops and doesn't pick up unrelated executables deep in a drive.
8. As an Organizer, I want to give an Entry an absolute path, so that a game that only lives in one fixed place still works.
9. As an Organizer, I want existing shortcuts to keep working, so that the current Library isn't broken by the change.
10. As an Organizer, I want any shortcut name to work, not only `game.lnk`, so that I don't have to rename shortcuts.
11. As an Organizer, I want a per-laptop override for one Entry's Launch target, so that a game installed by a normal installer in an odd place works on that laptop without changing the shared Library.
12. As an Organizer, I want to give a game launch arguments, so that games that need flags (or run through an emulator such as DOSBox) can be started from the menu.
13. As an Organizer, I want to set a game's working folder, so that games that look for their files next to their executable start correctly.
14. As an Organizer, I want the working folder to default to the folder of the game's executable, so that most games need no extra setting.
15. As an Organizer, I want the launcher to keep choosing a sensible executable automatically when an Entry gives none, so that simple Entries stay simple.

### Only Available Entries are shown

16. As a Player, I want to see only games that are installed on my laptop, so that every game I pick actually starts.
17. As an Organizer, I want Entries whose Installation is missing to be hidden rather than shown broken, so that the menu looks finished on every laptop.
18. As an Organizer, I want Entries without any launch information to be hidden, so that half-prepared Entries don't confuse Players.
19. As an Organizer, I want a shortcut whose target no longer exists to count as missing, so that stale shortcuts don't produce dead Entries.
20. As an Organizer, I want each laptop to reach its own result from the same Library, so that the vintage laptop shows its 15 games and the modern one its 30.

### Launching and coming back

21. As a Player, I want the game to start when I press JUGAR, so that playing is one click away.
22. As a Player, I want the launcher to get out of the way while I play, so that it doesn't cover the game or take CPU from it.
23. As a Player on a vintage laptop, I want the launcher to use almost no CPU while my game runs, so that the game runs as smoothly as it can.
24. As a Player, I want the menu to come back on its own when I close the game, so that I can pick the next one.
25. As an Organizer, I want games that start a helper and exit straight away to be handled with a per-Entry setting naming the real game process, so that the launcher waits for the right thing.
26. As an Organizer, I want a clear message on screen when a game fails to start, so that I know which Entry to fix.
27. As a Player, I want the menu to stay cheap on CPU while I'm only browsing, so that laptop fans and batteries aren't stressed between games.

### Rescanning

28. As an Organizer, I want the Library to be rescanned every time a Player comes back from a game, so that games I installed in the meantime appear without restarting.
29. As an Organizer, I want to rescan with F5 when not in Kiosk mode, so that I can check changes quickly while preparing a laptop.
30. As an Organizer, I want a rescan button in Kiosk mode, where the keyboard is blocked, so that I can rescan there too.

### Kiosk mode

31. As an Organizer, I want to put a laptop into Kiosk mode with one double-click, so that setup at the party is quick.
32. As an Organizer, I want Kiosk mode to work on Windows XP without installing anything, so that the vintage laptops can be kiosks too.
33. As an Organizer, I want Kiosk mode to hide the desktop, taskbar and Start menu, so that Players stay in the menu.
34. As an Organizer, I want Kiosk mode to block the Task Manager, the Run dialog, the Windows key and Ctrl+Alt+Del where permissions allow, so that Players can't wander off.
35. As an Organizer, I want the keyboard blocked while the menu is showing, so that Players can't escape the menu with shortcuts.
36. As a Player, I want the keyboard to work normally inside the game, so that I can play.
37. As an Organizer, I want Kiosk mode to start by itself when the laptop logs in, so that a reboot brings the kiosk back.
38. As an Organizer, I want the launcher to be restarted automatically if it ever crashes in Kiosk mode, so that a laptop never ends up on an empty screen.
39. As an Organizer, I want a log of what Kiosk mode did (settings applied, games started and ended, failures), so that I can find out what went wrong on a laptop.
40. As an Organizer, I want Kiosk mode to say in the log which settings couldn't be applied without administrator rights, so that I know how locked down a laptop really is.
41. As an Organizer, I want the launcher without Kiosk mode to stay an ordinary window, so that I can use it on my own PC without locking it.

### Leaving Kiosk mode

42. As an Organizer, I want a visible Quit button, so that I can leave Kiosk mode without tricks.
43. As an Organizer, I want Quit to undo all the lockdown settings and stop the automatic start, so that the laptop is back to normal.
44. As an Organizer, I want Quit to bring back the desktop immediately without logging off, so that I can keep working right away.
45. As an Organizer, I accept that a few settings only fully return after the next logon, so that Quit can stay fast.
46. As an Organizer, I want a separate way to undo Kiosk mode from outside the launcher, so that I can recover a laptop even if the launcher won't start.
47. As an Organizer, I accept that Players can also press Quit, because this is a friendly amateur party.

### Organizer view

48. As an Organizer, I want an Organizer button in the menu, so that I can check a laptop without leaving Kiosk mode.
49. As an Organizer, I want to see which Entries are hidden on this laptop and why (no launch information, or Installation not found), so that I know what to install or fix.
50. As an Organizer, I want to see which Entries lack a cover, a preview or details, so that I can complete the Library.
51. As an Organizer, I want a rescan button in the Organizer view, so that I can confirm a fix straight away.
52. As an Organizer, I want a plain-text log next to the launcher that lists, for every scan, each Available Entry with the exact file it will start and which rule found it, each hidden Entry with why it is hidden, and every game started and ended with the exact path launched, so that I can find out why a game is missing or picked the wrong executable on a laptop, even after a crash.

### Discovery

53. As an Organizer, I want a tool that finds Installations no Entry points to yet, so that adding new games doesn't mean building folders by hand.
54. As an Organizer, I want each top-level folder of an Install folder to count as one candidate, so that discovery is predictable.
55. As an Organizer, I want discovery to pick the most likely executable of each candidate, so that most games need no questions.
56. As an Organizer, I want discovery to match candidates against the online game database and fetch cover, details and description, so that new Entries look complete.
57. As an Organizer, I want discovery to ask me only when it isn't sure (game name, which executable, launch arguments), so that it stays quick for obvious games.
58. As an Organizer, I want discovery to write Entries with relative Launch targets, so that they work on every laptop.
59. As an Organizer, I want discovery to optionally fetch a preview video in the right format, so that new Entries play in the Info panel.
60. As an Organizer, I want a dry run that shows what discovery would create without writing anything, so that I can check before committing changes to the Master Library.
61. As an Organizer, I want discovery to leave existing Entries untouched unless I ask it to refresh them, so that my manual fixes aren't overwritten.
62. As an Organizer, I want discovery to run on a modern PC, so that it can use the online database, which needs modern HTTPS that XP lacks.

### Gamepad

63. As a Player, I want to move through the menu with a gamepad's D-pad or stick, so that I don't need the mouse.
64. As a Player, I want the A button to start the selected game and B to close the Info panel, so that the controls feel familiar.
65. As a Player, I want both modern (Xbox-style) and older pads to work, so that whatever pad I bring is usable.
66. As a Player, I want to plug a pad in while the menu is open, so that I don't have to restart anything.
67. As an Organizer, I want to pair a game with an external gamepad-to-keyboard mapper and its profile, so that Players can use pads in old games that only know the keyboard.
68. As an Organizer, I want that mapper to start with the game and close when the game ends, so that it never stays running in the menu or in the next game.
69. As an Organizer, I want this to work on Windows XP, so that vintage laptops get pads in-game too.

### Attract mode

70. As an Organizer, I want idle laptops to start Attract mode after a few minutes, so that the room looks like a café full of terminals.
71. As an Organizer, I want to set the idle time per laptop, or turn Attract mode off, so that I can adapt it to the event.
72. As a passer-by, I want Attract mode to show each Available Entry with its preview playing, so that I'm tempted to sit down and play.
73. As an Organizer, I want Entries without a preview to show their cover for a shorter time, so that the loop keeps moving.
74. As a Player, I want any mouse, keyboard or gamepad input to bring back the menu without starting a game, so that waking the laptop is safe.

## Implementation Decisions

### Library scan

- The library scan module becomes the one place that decides what an Entry is and whether it is Available. Its interface changes from "Library folder only" to "Library folder plus this laptop's configuration" (Install folders and per-laptop overrides).
- It returns two lists: Available Entries, and hidden Entries with a reason ("no launch information" or "Installation not found"). The menu shows the first. The Organizer view shows the second, and also flags Available Entries that lack a cover, preview or details.
- For each Available Entry, the result includes everything needed to launch it: the resolved Launch target, launch arguments, working folder, the optional process to wait for, and the optional mapper helper.
- Launch target resolution order, first match wins:
  1. a per-laptop override for this Entry;
  2. the Entry's `exe=` as an absolute path, if it exists;
  3. the Entry's `exe=` as a relative path, tried in each Install folder in listed order;
  4. the executable's file name searched up to two folder levels deep inside each Install folder, in listed order;
  5. any shortcut in the Entry folder whose target exists;
  6. the existing automatic choice of an executable inside the Entry folder or its first-level subfolders.
- Deliberate extra in that order: a relative `exe=` that isn't found in any Install folder is also tried in the Entry folder (and the folder of its `juego.cfg`) before the file-name search of rule 4. This keeps Entries that carry the game inside their own folder working, as they did before Install folders existed.
- A shortcut counts as Available when its target file exists. A shortcut whose target can't be read is treated as Available and left for the system to open.
- The rule that the shortcut must be named `game.lnk` is removed.
- Per-Entry options (`juego.cfg`) gain:
  - `args=`: launch arguments;
  - `dir=`: the working folder, relative to the resolved executable's folder or absolute. By default it is the executable's folder; for shortcuts it is whatever the shortcut says;
  - `wait=`: the executable name of the real game process;
  - `con=` and `con_args=`: the mapper helper and its arguments.
  Existing keys keep their meaning.
- Laptop configuration (`cyber.cfg`) gains:
  - `installs=`: Install folders separated by `;`;
  - `attract=`: idle minutes before Attract mode, with 0 meaning off;
  - per-laptop overrides as `exe.<Entry folder name>=<absolute path>`.
- The `games/` folder keeps its role as the Library root. The `scraps/` concept stays removed.

### Launching and waiting

- The launcher starts executables itself, passing arguments and working folder, and keeps a handle to the started process. Shortcuts are opened through the system shell in a way that still returns a process handle where possible.
- With `wait=` set, the launcher waits (with a time limit) for a process with that executable name to appear after the start, then waits for that process instead.
- If a mapper helper is configured, it is started just before the game and closed when the game ends.
- While a game runs, the launcher hides its window, stops rendering and blocks on the game process, while still handling window messages. When the game ends, it shows its window again, rescans the Library and redraws.
- The rescan after a game is silent. The "Biblioteca reescaneada: N juegos" status only shows for a manual rescan (F5 or Reescanear), so after a game the menu goes idle straight away.
- The menu only redraws when something changes: input, a playing preview, an animation in progress, or a status message. An idle menu with no preview playing uses next to no CPU.

### Kiosk mode (ADR 0001)

- A command-line switch turns the launcher into Kiosk mode. Without it, the launcher is an ordinary window, and Esc and F5 work as today.
- Entering Kiosk mode applies the same lockdown the PowerShell script applied today:
  - Task Manager, the Run dialog and the Windows key are disabled for the user, and for the machine when the launcher has administrator rights;
  - the Start button is removed;
  - Windows is told not to restart the desktop shell, and Ctrl+Alt+Del is disabled (both need administrator rights);
  - automatic start is registered in this user's standard Run key (`HKCU\...\Windows\CurrentVersion\Run`). It is per-user on purpose: Kiosk mode is for fun and must work without administrator rights;
  - the desktop shell is closed, and kept closed while Kiosk mode lasts.
- A low-level keyboard hook blocks all keys while the menu is showing. It is removed while a game runs and restored when the menu comes back.
- Quit undoes every setting above, removes the automatic start (from this user's Run key, and from the machine Run key and the wrong `Windows NT` key used by older versions), restarts the desktop shell and exits. It does not log off.
- A second command-line switch undoes Kiosk mode without showing any UI, for recovery.
- The double-click scripts stay as thin wrappers:
  - the kiosk script runs the launcher in Kiosk mode and starts it again whenever it exits with anything other than the Quit exit code;
  - the cleanup script runs the recovery switch.
  Both use only what Windows XP ships with.
- Kiosk mode writes what it does (settings applied or not, Quit, recovery) to the troubleshooting log `cyber.log` (see Troubleshooting log), which replaces the old `kiosko.log`.
- The PowerShell kiosk and cleanup scripts and the separate keyboard blocker are removed. The build stops producing and copying them.
- In Kiosk mode the menu shows a Quit button and an Organizer button next to the theme button. Outside Kiosk mode, Quit simply exits.

### Troubleshooting log

- One plain-text log, `cyber.log` next to the launcher, is written in both normal and Kiosk mode. It holds the Kiosk lines too.
- Every scan (start-up, after a game, manual rescan) writes a block: each Available Entry with its resolved Launch target as a full path and the resolution rule that found it (per-laptop override, absolute `exe=`, relative `exe=` in Install folder N, Entry folder, name search, shortcut, automatic choice), then each hidden Entry with its reason and the `exe=` (and per-laptop override) it looked for, if any.
- Game start, end and failure lines give the exact path launched, its arguments and working folder, and `wait=` and the mapper helper (`con=`) when used.
- The log is appended to across runs, so a crash and relaunch keeps the history. Once it reaches about 1 MB it is renamed to `cyber.old.log` (replacing any older one) and a new `cyber.log` is started.
- The scan result exposes which rule found each Launch target, so the rules stay testable through Seam 1. The log text is built by a portable formatter with its own tests; only the file writing is Windows code.

### Discovery

- Discovery is a new mode of the existing scraper tool, which stays PowerShell for a modern PC. It reuses that tool's database matching, cover/details/description writing and optional preview download.
- Its input is the Install folders (given as an argument or read from the laptop configuration) and the Master Library. A candidate is a top-level folder of an Install folder that no Entry's Launch target resolves into.
- Deliberately, a folder name covered by an Entry in one Install folder counts as covered in every Install folder. Two copies of a game are fine: the first listed Install folder wins at launch, so no second Entry is written for the other copy.
- Executable choice reuses the launcher's scoring rules: prefer names like the folder, avoid installers, uninstallers, setup, servers and similar.
- It asks on the console only when the match or the executable choice is ambiguous.
- It writes a new Entry folder with options holding a relative `exe=` (plus `args=` if given), and the fetched cover, details, description and optional preview.
- A dry run prints the planned Entries without writing. Existing Entries are only rewritten when asked to.

### Gamepad

- Uses SDL's game controller support for modern pads, with a plain joystick fallback for older DirectInput pads. Pads can be connected and disconnected while the launcher runs.
- Pad mapping in the menu: D-pad or left stick moves the selection, A starts the game (JUGAR), B closes the Info panel. Pads are only read while the menu is showing.
- No configuration screen.

### Attract mode

- Starts after the configured idle time with no input (3 minutes by default). It cycles through Available Entries in Library order, full screen.
- Each Entry shows for 20 seconds or until its preview ends, whichever comes first. Entries without a preview show their cover for 8 seconds.
- Any input leaves Attract mode and is consumed, so it never starts a game. Attract mode is suspended while a game runs.

### Documentation

- The README describes the new options, Install folders, Kiosk mode and its recovery, discovery, gamepads and Attract mode.
- The glossary already holds the vocabulary used here.

## Testing Decisions

- No automated tests are written now; the Organizer asked to postpone them. The seams below are recorded so tests can be added without restructuring later.
- A good test drives a seam from the outside and checks observable results: which Entries are Available, what they would launch, what hidden Entries report. Internal helpers aren't tested directly.
- **Seam 1, the library scan (the only automated seam):** give it a Library folder plus a laptop configuration over a prepared folder tree of fake Installations. Check the Available Entries, their resolved Launch target, arguments and working folder, and the hidden Entries with their reasons. This covers every resolution rule, shortcut handling and the listing rule. Because the scan uses Windows file APIs, tests would run on Windows or under Wine.
- **Seam 2, a Kiosk session (manual):** a checklist run on one XP laptop and one modern laptop. It covers: enter Kiosk mode; the lockdown is visible; keys are blocked in the menu; launch a game and the keyboard works in it; CPU is idle while it runs; the menu returns; Quit restores the desktop; a launcher crash is restarted; the recovery script works; the Organizer view and rescan work; the gamepad navigates; the mapper helper starts and stops with the game; Attract mode starts and exits.
- **Seam 3, the discovery dry run (manual):** run it over a folder of fake Installations and read the planned Entries.
- Prior art: the repo has a small manual test program for the video player, and no automated test harness yet.

## Out of Scope

- Filtering, searching or sorting the menu; play counts and "most played" or "new" badges; remembering the chosen theme.
- Session timers, billing or anything else for a commercial café; a central server or syncing the Library over the network.
- A built-in gamepad-to-keyboard mapper with per-game mappings in the Entry. The external mapper helper is used instead.
- A gamepad configuration screen.
- A PIN or other protection on Quit.
- Porting to Linux or moving to SDL3 or another language.
- Automated tests for now.
- Normalising the mixed line endings in the repository.

## Further Notes

- Windows XP SP3 32-bit is a hard target. Anything used on the laptops must exist on XP: no PowerShell, no APIs newer than XP, SDL2 2.32.x. 64-bit Windows is the second target.
- Suggested order for tickets, each building on the previous:
  1. one Library for every laptop and the new listing rule;
  2. launching and waiting, idle rendering, Kiosk mode, Quit, the Organizer view and rescanning;
  3. discovery;
  4. gamepad menu navigation;
  5. the mapper helper;
  6. Attract mode.
- The current repo Library's shortcuts all point at one development machine, so they are placeholders. On real laptops, Entries will mostly move to relative `exe=` Launch targets.
