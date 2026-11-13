# The launcher owns Kiosk mode, not PowerShell scripts

Kiosk mode used to be run by `kiosko.ps1`, `limpiador.ps1` and `kbdblock.exe`. The PowerShell script applied the registry lockdown, kept `explorer.exe` closed, and guessed which new process was the game so it could kill the launcher and free CPU. We moved all of that into `cyber_ahero.exe --kiosko`. The launcher now starts the game itself and waits for it while idle. It applies and undoes the lockdown, and blocks the keyboard only while the menu is showing. The Quit button leaves Kiosk mode.

We did this because Windows XP laptops, a hard target, ship without PowerShell. The script's process guessing was also unreliable, since any new user process could be taken for the game. A launcher that starts the game knows exactly which process to wait for.

## Consequences

- A laptop needs nothing installed besides the launcher and its DLLs.
- Games that start a helper process and exit straight away need `wait=` in their `juego.cfg` to name the real process.
- Kiosk behaviour is now C code and has to be rebuilt to change. It can no longer be edited as a script on the laptop.
