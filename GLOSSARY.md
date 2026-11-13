# Glossary

**Library** (_biblioteca_)
The set of all Entries a launcher shows. One Library is copied to every laptop.

**Entry** (_juego_, folder under `games/`)
What the launcher knows about one game: its name, cover, details, description, preview and how to start it. An Entry is not the game itself.
_Avoid_: "game" when you mean the folder; say Entry.

**Installation**
The real game files on a laptop's disk. They live outside the Library and can differ from one laptop to the next.

**Launch target**
What an Entry tells the launcher to start in order to run its Installation.

**Install folder**
A folder on one laptop where Installations live. Each laptop has its own list of Install folders.

**Available** (of an Entry)
Its Installation is present on this laptop. Only Available Entries are shown to players; the rest are hidden.

**Attract mode**
What the launcher shows when nobody has touched it for a while: Entries cycling one by one with their previews, until any input returns to the menu.

**Kiosk mode** (_modo kiosko_)
The laptop is locked to the launcher: no desktop, and no keyboard while the menu is showing. Leaving it is done with the Quit button. Without Kiosk mode, the launcher is an ordinary window.

**Master Library**
The Organizer's reference copy of the Library, which is copied to every laptop.

**Organizer**
The person who prepares the Library and the laptops before a LAN party. Distinct from a **Player**, who only picks and plays games.

**Info panel** (_ficha técnica_)
The right-hand column showing the selected Entry's preview, name, details and the JUGAR button.

## Relationships

- An Entry points to exactly one Installation through its Launch target.
- One Library is shared by many laptops; each laptop may have only some of the Installations.
- An Entry's Launch target is found by looking in this laptop's Install folders, so the same Entry works on every laptop.
