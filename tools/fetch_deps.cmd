@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0fetch_deps.ps1" %*
