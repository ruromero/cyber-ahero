@echo off
rem =========================================================================
rem  limpiar.bat - DESHACE EL MODO KIOSKO (doble clic), para recuperar un
rem  portatil bloqueado aunque el menu no este abierto.
rem  Ejecuta cyber_ahero.exe --limpiar: cierra el launcher si esta en marcha,
rem  quita todas las restricciones y el arranque automatico y vuelve a abrir
rem  el escritorio. No cierra la sesion. Detalles en cyber.log.
rem =========================================================================
cd /d "%~dp0"
"%~dp0cyber_ahero.exe" --limpiar
echo Modo kiosko deshecho. Detalles en "%~dp0cyber.log"
ping -n 4 127.0.0.1 >nul
