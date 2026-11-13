@echo off
rem =========================================================================
rem  kiosko.bat - ARRANCA EL MODO KIOSCO (doble clic)
rem  Bloquea el escritorio y el teclado, abre el menu del ciber a pantalla
rem  completa y, cuando lanzas un juego, cierra la app del ciber para no
rem  consumir CPU; al terminar el juego, vuelve a abrir el menu.
rem  Para salir del modo kiosco: doble clic en LIMPIAR.BAT
rem =========================================================================
cd /d "%~dp0"
start "" powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "%~dp0tools\kiosko.ps1"
