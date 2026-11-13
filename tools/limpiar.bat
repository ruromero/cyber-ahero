@echo off
rem =========================================================================
rem  limpiar.bat - SALE DEL MODO KIOSCO (doble clic)
rem  Quita todas las restricciones, para el bloqueador de teclado, quita el
rem  arranque automatico y reinicia la sesion de Windows.
rem =========================================================================
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\limpiador.ps1"
