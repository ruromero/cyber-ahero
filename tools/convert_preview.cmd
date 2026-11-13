@echo off
setlocal
if "%~1"=="" (
    echo Uso: convert_preview.cmd entrada.mp4 [salida.mpg]
    exit /b 1
)
set "IN=%~1"
if "%~2"=="" (set "OUT=%~dpn1.mpg") else (set "OUT=%~2")
ffmpeg -y -i "%IN%" -vf "scale=480:360:force_original_aspect_ratio=increase,crop=480:360,fps=25" -c:v mpeg1video -b:v 1200k -bf 0 -c:a mp2 -ar 44100 -b:a 128k -f mpeg "%OUT%"
if errorlevel 1 (
    echo Error al convertir. Comprueba que ffmpeg esta en el PATH.
    exit /b 1
)
echo Listo: %OUT%
