# Entorno de desarrollo: instalar y desinstalar

Qué hay que instalar para compilar y pasar los tests en cada sistema, y cómo
quitarlo después. En los portátiles de la LAN party no se instala nada de esto:
solo necesitan el contenido de `build\<arch>\`.

## Resumen

| Requisito | Para qué | macOS | Windows |
| --- | --- | --- | --- |
| CMake ≥ 3.16 | configurar la build | Homebrew `cmake` | incluido en w64devkit |
| GCC mingw-w64 (i686 + x86_64) | compilar el `.exe` de XP (32 bits) y el de 64 bits | Homebrew `mingw-w64` | w64devkit (`tools\get_toolchain.cmd`) |
| Compilador nativo (clang) | compilar y ejecutar los tests en el Mac | Xcode Command Line Tools | no hace falta (los tests se compilan con w64devkit) |
| SDL2, SDL2_image, SDL2_ttf (mingw) | enlazar el lanzador | copiar `deps\` desde Windows (ver abajo) | `tools\fetch_deps.cmd` |
| PowerShell 7 | ejecutar `tools/scrape.ps1` y sus tests | Homebrew `powershell` | ya viene PowerShell 5.1; opcional PowerShell 7 |
| Pester ≥ 5 | tests de `scrape.ps1` (descubrimiento) | módulo de usuario | módulo de usuario |
| yt-dlp + ffmpeg (opcionales) | `scrape.ps1 -Video`: bajar el tráiler y convertirlo a `preview.mpg` | Homebrew `yt-dlp`, `ffmpeg` | `winget` `yt-dlp.yt-dlp`, `Gyan.FFmpeg` |

No hay Wine en el Mac (la cask `wine-stable` de Homebrew está deshabilitada
desde 2026-09-01), así que los `.exe` de Windows, tests incluidos, se compilan
en el Mac pero solo se **ejecutan** en Windows.

## macOS

Versiones instaladas el 2026-11-13: mingw-w64 14.0.0, PowerShell 7.7.0,
Pester 6.2.0.

### Instalar

```sh
xcode-select --install            # clang nativo, si aún no está
brew install cmake
brew install mingw-w64            # i686-w64-mingw32-gcc y x86_64-w64-mingw32-gcc
brew install powershell           # pwsh (es una fórmula, no una cask)
pwsh -NoProfile -Command 'Install-Module Pester -Scope CurrentUser -Force -SkipPublisherCheck -MinimumVersion 5.0'
```

Pester queda en `~/.local/share/powershell/Modules/Pester/`.

Opcional, solo para que el Organizador baje vistas previas con
`scrape.ps1 -Video` (el script los busca en el `PATH`):

```sh
brew install yt-dlp ffmpeg
```

`deps/` no está en git. En el Mac se copia la carpeta `deps\` que genera
`tools\fetch_deps.cmd` en Windows (son los paquetes oficiales *mingw* de SDL;
también se pueden descargar a mano desde las releases de SDL en GitHub).

### Compilar y probar

```sh
# Tests nativos (en un host que no es Windows, CMake solo compila los tests)
cmake -S . -B build/host-tests && cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure

# Build cruzada para Windows (t = i686-w64-mingw32 o x86_64-w64-mingw32)
D="$PWD/deps"
cmake -S . -B build/cross-$t -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=$t-gcc -DCMAKE_RC_COMPILER=$t-windres \
  -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=NEVER \
  -DSDL2_DIR=$D/SDL2/$t/lib/cmake/SDL2 \
  -DSDL2_image_DIR=$D/SDL2_image/$t/lib/cmake/SDL2_image \
  -DSDL2_ttf_DIR=$D/SDL2_ttf/$t/lib/cmake/SDL2_ttf
cmake --build build/cross-$t

# Ojo: el libmsvcrt.a de Homebrew es en realidad la UCRT, que XP no tiene. Para
# un .exe de 32 bits que arranque en XP, añade a la configuración:
#   -DCMAKE_C_FLAGS=-mcrtdll=msvcrt-os -DCMAKE_EXE_LINKER_FLAGS=-mcrtdll=msvcrt-os
# y comprueba que importa msvcrt.dll y ningún api-ms-win-crt-*.dll:
#   i686-w64-mingw32-objdump -p build/cross-$t/cyber_ahero.exe | grep "DLL Name"
# (w64devkit en Windows ya enlaza msvcrt.dll; no hace falta nada.)

# Tests de PowerShell
pwsh -NoProfile -Command 'Invoke-Pester tools/tests'
```

### Desinstalar

```sh
brew uninstall yt-dlp ffmpeg      # si los instalaste para -Video
pwsh -NoProfile -Command 'Uninstall-Module Pester -AllVersions'   # antes de quitar pwsh
rm -rf ~/.local/share/powershell                                   # módulos y perfil de pwsh
brew uninstall powershell
brew uninstall mingw-w64
brew autoremove                   # dependencias que solo usaba mingw-w64 (gmp, mpfr, libmpc, isl...)
brew uninstall cmake              # si no lo usas para otra cosa
rm -rf deps build                 # dentro del repo
```

Las Xcode Command Line Tools se pueden dejar (las usan otras herramientas); para
quitarlas: `sudo rm -rf /Library/Developer/CommandLineTools`.

## Windows

### Instalar

```
tools\fetch_deps.cmd        (SDL2, SDL2_image, SDL2_ttf en deps\)
tools\get_toolchain.cmd     (w64devkit en _toolchain\: GCC, CMake, Ninja)
```

Los dos son portables: no tocan el registro ni el `PATH` del sistema.

Para los tests de `scrape.ps1`, Pester ≥ 5 (Windows trae Pester 3.4, que no
sirve). Desde PowerShell:

```powershell
Install-Module Pester -Scope CurrentUser -Force -SkipPublisherCheck -MinimumVersion 5.0
```

PowerShell 7 es opcional (`winget install Microsoft.PowerShell`); los scripts
funcionan con el PowerShell 5.1 que trae Windows.

Opcional, solo para `scrape.ps1 -Video` (vistas previas); abre una consola
nueva después para que el `PATH` los incluya:

```
winget install yt-dlp.yt-dlp
winget install Gyan.FFmpeg
```

### Compilar y probar

```
tools\build_x64.cmd
tools\build_x86.cmd
```

Para compilar y ejecutar también los tests, configura con `-DCYBER_TESTS=ON` y
lanza `ctest --test-dir <carpeta de build> --output-on-failure`. Los de
PowerShell: `Invoke-Pester tools\tests`.

### Desinstalar

```powershell
Uninstall-Module Pester -AllVersions        # la 3.4 que trae Windows se queda
winget uninstall Microsoft.PowerShell       # solo si instalaste PowerShell 7
winget uninstall yt-dlp.yt-dlp              # solo si los instalaste para -Video
winget uninstall Gyan.FFmpeg
```

y borra `_toolchain\`, `deps\` y `build\` dentro del repo.
