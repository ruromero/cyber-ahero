@echo off
rem =========================================================================
rem  kiosko.bat - ARRANCA EL MODO KIOSKO (doble clic)
rem  Abre cyber_ahero.exe --kiosko, que bloquea el escritorio y el teclado
rem  del menu. Si el launcher se cierra por cualquier motivo que no sea el
rem  boton Salir (codigo 10), se vuelve a abrir a los pocos segundos.
rem  Solo usa lo que trae Windows XP (cmd.exe y ping).
rem  Para salir del modo kiosko: boton Salir del menu, o doble clic en
rem  LIMPIAR.BAT
rem =========================================================================
if not "%~1"=="/min" (
    start "Cyber AHERO - modo kiosko" /min cmd /c ""%~f0" /min"
    exit /b
)
cd /d "%~dp0"
:otra
"%~dp0cyber_ahero.exe" --kiosko
if "%errorlevel%"=="10" goto fin
rem Se cerro sin Salir (fallo, o lo mataron): esperar 2 s y volver a abrirlo
ping -n 3 127.0.0.1 >nul
goto otra
:fin
