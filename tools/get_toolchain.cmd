@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0get_toolchain.ps1" %*
