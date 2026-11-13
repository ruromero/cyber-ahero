# Cyber Launcher

Lanzador de juegos estilo **cibercafé**, hecho en C con **SDL2**.
Lee los juegos desde una carpeta: a la izquierda una **rejilla de iconos** y a
la derecha una **columna con la ficha técnica** del juego apuntado (preview,
nombre y datos) y un botón **JUGAR**. El juego no se abre al pasar el ratón: hay
que pulsar **JUGAR**. Arranca a pantalla completa (la interfaz se escala según
la resolución); con **F11** o **Alt+Intro** se alterna ventana/pantalla completa.

Pensado para funcionar en **Windows XP de 32 bits** y también en Windows de
**64 bits**.

## Cómo funciona

1. Al arrancar, escanea la ruta de biblioteca. Cada **subcarpeta** es un juego.
2. Del contenido de cada carpeta toma automáticamente:
   - el **acceso directo** (`*.lnk`) que se abrirá al hacer clic,
   - la **imagen del icono** (`.png`, `.jpg` o `.bmp`),
   - el **vídeo de preview** (`preview.mpg` / `preview.mpeg`, MPEG-1).
3. También lee, si existen, la **descripción** (`descripcion.txt` u otro `.txt`)
   y los **datos de la ficha** (`details.txt`, líneas `Clave: valor`).
4. Al pasar el ratón por encima de un icono, la columna muestra su **ficha
   técnica**: el preview (vídeo o imagen), el nombre y los datos. Al
   **salir del icono** la ficha desaparece; si **haces clic** en el icono, la
   ficha **se queda fija** en la columna aunque apartes el ratón (Esc o clic en
   una zona vacía para cerrarla).
5. A los ~0,4 s empieza a reproducirse el vídeo de preview en bucle (con
   sonido). Si no hay vídeo, se muestra la imagen o un aviso.
6. El juego **no** se abre al pasar el ratón: pulsa **JUGAR** (o Intro/Espacio,
   o doble clic sobre el icono).

### Controles

| Acción | Control |
| --- | --- |
| Ver la ficha de un juego | Pasar el ratón por encima |
| Fijar la ficha en la columna | Clic en el icono |
| Cerrar la ficha fijada | Esc o clic en zona vacía |
| Abrir el juego | Botón JUGAR, Intro o Espacio |
| Abrir directo (doble clic) | Doble clic sobre el icono |
| Moverse con teclado | Flechas |
| Desplazar la rejilla | Rueda del ratón |
| Pantalla completa | F11 o Alt+Intro |
| Salir | Esc (sin ficha fijada) |

## Estructura de `games`

Cada **subcarpeta** de `games` es un juego, y todo lo suyo (acceso directo,
carátula, ficha, preview, descripción) va dentro de esa misma carpeta:

```
games/
  Quake 3 Arena/
    Quake 3 Arena.lnk     <- acceso directo que abre el juego (cualquier nombre)
    cover.png             <- carátula (también .ico, .jpg o .bmp)
    preview.mpg           <- vídeo de preview en MPEG-1, 4:3 (opcional)
    details.txt           <- datos de la ficha técnica (opcional)
    descripcion.txt       <- texto libre (opcional)
    juego.cfg             <- opciones del juego (opcional, ver más abajo)
  Doom/
    juego.cfg             <- exe=D:\Juegos\Doom\doom.exe (sin acceso directo)
    cover.png
    details.txt
    preview.mpg
```

Para lanzar el juego se elige, dentro de `games/<Nombre>/`, el primero que
funcione de estos:

1. el `exe=` de su `juego.cfg`, si ese archivo existe;
2. cualquier **acceso directo** (`*.lnk`, con el nombre que sea) cuyo destino
   existe en este portátil;
3. un **`.exe`** de la carpeta (se elige el que más se parece al nombre de la
   carpeta y se descartan instaladores/servidores dedicados).

Si el juego vive en una **subcarpeta** (por ejemplo
`games/<Nombre>/<Nombre>/Juego.exe`), también se buscan ahí el `.lnk` o el
`.exe` cuando no hay ninguno en el nivel superior.

**Solo se muestran los juegos que pueden arrancar.** Se ocultan:

- las carpetas sin forma de lanzar el juego (solo carátula, ficha, etc.):
  *sin información de lanzamiento*;
- las que la tienen pero no existe en este portátil (un `exe=` que no está, o
  accesos directos cuyo destino ya no existe): *instalación no encontrada*.

Un acceso directo cuyo destino no se puede leer (por ejemplo, uno que apunta a
algo que no es un archivo) se da por bueno y se deja abrir a Windows.

`details.txt` (también se aceptan `ficha.txt`, `info.txt` o `datos.txt`) tiene
una línea por dato, `Clave: valor`:

```
Género: Aventura
Año: 2004
Jugadores: 1
```

Si no hay `details.txt`, la columna muestra cinco líneas con **`?`**
(`Género: ?`, `Año: ?`, `Jugadores: ?`, `Desarrollador: ?`, `Distribuidora: ?`).
La línea `Plataforma` nunca se muestra en la ficha, aunque esté en
`details.txt` (el scraper la escribe igualmente).

En la **rejilla**, si hay varias imágenes se elige por puntuación: suma puntos
que el nombre coincida con el de la carpeta o contenga `icon`, `cover` o `logo`,
y que sea `.ico` (más que `.png` o `.jpg`). Así `cover.ico` gana a `cover.png`.
Si no hay ninguna imagen se muestra un **icono por defecto**.

El **vídeo de preview** solo se usa si el archivo se llama **`preview`**
(`preview.mpg` / `preview.mpeg`). La preview se muestra siempre en **4:3**
(proporción de la época). Al pasar el cursor sobre un juego aparece de inmediato
su **primer fotograma** (sin icono de por medio) y, un instante después, el vídeo
empieza a reproducirse. Si no hay vídeo `preview`, en la preview se muestra
siempre **`cover.png`** (aunque exista `cover.ico`); si tampoco hay `cover.png`,
se usa el icono por defecto.

## Indicar el ejecutable de un juego (`juego.cfg`)

Si el lanzador no encuentra el `.exe` correcto (o el juego necesita otro), pon
un archivo **`juego.cfg`** dentro de la carpeta del juego. Las rutas se
resuelven respecto a la carpeta del juego (se aceptan rutas absolutas) y las líneas con `#` o `;` son comentarios:

```
# Opciones del juego (ejemplo)
exe=ActionPuzzle100\ActionPuzzle.exe   <- ejecutable a lanzar
# shortcut=mi_acceso.lnk               <- alternativo al .exe
# nombre=Action Puzzle 100             <- nombre mostrado
# icon=cover.ico                       <- carátula/icono
# video=preview.mpg                    <- vídeo de preview
# descripcion=sinopsis.txt             <- descripción
```

Si el archivo de `exe` existe, gana siempre a los accesos directos y a la
detección automática. Con esto no hace falta ni `.lnk` ni que el `.exe` se llame
parecido a la carpeta. Si no existe, se prueban los accesos directos y la
detección automática; si nada funciona, el juego se oculta.

## Elegir la ruta de la biblioteca

La carpeta de juegos, por orden de prioridad:

1. Primer argumento: `cyber_ahero.exe "D:\Juegos"`
2. Variable de entorno: `CYBER_GAMES=D:\Juegos`
3. Archivo `cyber.cfg` junto al `.exe` con una línea `library=D:\Juegos`
4. Por defecto: carpeta `games` junto al `.exe`

En `cyber.cfg`, una ruta relativa se resuelve respecto a la carpeta del `.exe`.
`cyber.cfg` se lee una sola vez, al arrancar.

## Compilar (desde `cmd` o PowerShell, sin MSYS2)

Todo se hace con los scripts `.cmd`, que llaman a PowerShell internamente.

### 1. Descargar dependencias (SDL2, SDL2_image, SDL2_ttf)

```
tools\fetch_deps.cmd
```

Descarga los paquetes oficiales *mingw* de SDL en `deps\`. El decodificador de
vídeo (`pl_mpeg`) ya viene incluido en `third_party\`.

### 2. Descargar el compilador portable (una sola vez)

```
tools\get_toolchain.cmd
```

Descarga **w64devkit** (GCC + CMake + Ninja, portable, no instala nada en el
sistema) en `_toolchain\`. Es **multilib**: sirve para 32 y 64 bits desde el
mismo kit, y las builds de 32 bits apuntan a Windows XP.

### 3. Compilar

```
tools\build_x64.cmd        (64 bits -> build\x64\cyber_ahero.exe)
tools\build_x86.cmd        (32 bits, XP -> build\x86\cyber_ahero.exe)
```

Cada script detecta el compilador en `_toolchain\`, compila y deja en
`build\<arch>\` todo lo necesario para ejecutar:

- `cyber_ahero.exe` y las **DLLs** de SDL,
- una **copia** de la carpeta `games` del repositorio y un `cyber.cfg` con
  `library=games` (ojo: si `games` es muy grande, la copia también),
- `assets\` (logo),
- el modo kiosko: `kiosko.bat` y `limpiar.bat` en la raíz, `tools\kiosko.ps1`,
  `tools\limpiador.ps1` y `kbdblock.exe` (ver [Modo kiosko](#modo-kiosko)).

Si ya tienes tu propio `gcc` + `cmake` en el `PATH`, se usan automáticamente
(no hace falta `get_toolchain`), o pásalos con `-CC` y `-CMake`.

El proyecto compila con `_WIN32_WINNT=0x0501` (Windows XP) y sin APIs posteriores
a XP, por lo que el `.exe` de 32 bits corre en XP SP3 y en Windows de 64 bits.

### Con CMake directamente

```
cmake -S . -B build\x64 -G "MinGW Makefiles" ^
  -DSDL2_DIR=deps\SDL2\x86_64-w64-mingw32\lib\cmake\SDL2 ^
  -DSDL2_image_DIR=deps\SDL2_image\x86_64-w64-mingw32\lib\cmake\SDL2_image ^
  -DSDL2_ttf_DIR=deps\SDL2_ttf\x86_64-w64-mingw32\lib\cmake\SDL2_ttf
cmake --build build\x64
```

### Tests

La lógica portable (sin `windows.h` ni SDL) tiene tests en `tests\`. En un
host que no es Windows (macOS, Linux), CMake solo compila los tests:

```
cmake -S . -B build/host-tests && cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

En una build de Windows se activan con `-DCYBER_TESTS=ON`.

## Distribuir

Copia junto al `.exe`:

- `SDL2.dll`, `SDL2_image.dll`, `SDL2_ttf.dll` (el script de build las copia).
- La carpeta `games`, o un `cyber.cfg` que apunte a ella.

Lo más sencillo es copiar la carpeta `build\<arch>\` entera, que ya lo trae
todo (incluido el modo kiosko).

### Empaquetar la última build en un RAR

```
tools\pack_rar.cmd                 (empaqueta la build más reciente)
tools\pack_rar.cmd -Build x64      (o -Build x86)
```

Genera `dist\cyber-ahero-AAAAMMDD-HHMMSS.rar` con el `.exe`, las DLLs de SDL,
la carpeta `games` del repositorio, el README y los `.example`. No incluye los
archivos del modo kiosko. Requiere `rar.exe`
(viene con WinRAR). Ojo: si tu `games` es muy grande, el RAR también lo será.

## Convertir vídeos de preview a MPEG-1

El reproductor interno solo admite **MPEG-1** (formato muy ligero y portable,
ideal para XP). Convierte cualquier vídeo con `ffmpeg`:

```
tools\convert_preview.cmd mi_video.mp4
```

o manualmente:

```
ffmpeg -i entrada.mp4 -vf "scale=480:360:force_original_aspect_ratio=increase,crop=480:360,fps=25" ^
  -c:v mpeg1video -b:v 1200k -bf 0 ^
  -c:a mp2 -ar 44100 -b:a 128k -f mpeg preview.mpg
```

La salida es **480x360 (4:3)**, recortando el centro si el vídeo es 16:9, para
que se vea como los juegos de la época.

## Scraping de carátulas y datos (IGDB)

Rellena cada `games\<Nombre>\` con `cover.ico` (si el juego trae un `.ico`; si
no, `cover.png`), `details.txt` y `descripcion.txt` (y `preview.mpg` en 4:3 con
`-Video`), buscando por el nombre de la carpeta.

1. Copia `scrape.cfg.example` a `scrape.cfg` y pon tus claves:
   - `igdb_client_id` / `igdb_client_secret` — gratis en
     <https://dev.twitch.tv/console/apps> (crea una app y genera el secret).
   - `steamgriddb_key` (opcional) — respaldo de carátula si IGDB no tiene:
     <https://www.steamgriddb.com/profile/preferences/api>

2. Ejecuta:

```
tools\scrape.cmd                    (usa games\ o la ruta de cyber.cfg)
tools\scrape.cmd -Root C:\Juegos -Force
tools\scrape.cmd -DryRun            (solo muestra las coincidencias)
tools\scrape.cmd -CoverOnly         (solo carátulas)
tools\scrape.cmd -Video             (además, vídeos de preview)
```

Escribe directamente en la carpeta de cada juego dentro de `games`.

Genera la carátula y los campos `Género`, `Año`, `Jugadores`, `Desarrollador`,
`Distribuidora` y `Plataforma`, más la sinopsis en `descripcion.txt`. Omite los
juegos que ya tienen carátula y ficha (usa `-Force` para rehacerlos). Respeta un
pequeño retardo entre peticiones para no pasarse de los límites de la API.

Con `-Video` también descarga un vídeo por juego (`preview.mpg`, MPEG-1): busca
`"<Juego> <Año> PC"` y coge el mejor resultado ≤720p. Requiere **`yt-dlp`** en el
PATH (`winget install yt-dlp`) y **`ffmpeg`**. La búsqueda automática puede fallar
en títulos ambiguos; revísalos. Úsalo para tus copias personales.

## Modo kiosko

Para dejar el PC del ciber bloqueado en el menú de juegos:

- **`kiosko.bat`** (doble clic) arranca `tools\kiosko.ps1`, que:
  - aplica restricciones en el registro: sin Administrador de tareas, sin
    Ejecutar (Win+R), sin tecla Windows, sin botón Inicio y sin Ctrl+Alt+Supr
    (las de nivel máquina requieren administrador),
  - evita que Windows reinicie `explorer.exe` y lo mantiene cerrado (sin
    escritorio ni barra de tareas),
  - se registra para **arrancar solo** al iniciar sesión
    (`HKLM\...\Windows\CurrentVersion\Run`, valor `CiberAhKiosko`),
  - abre `cyber_ahero.exe` junto con **`kbdblock.exe`**, que se traga todas las
    teclas mientras el menú está en pantalla (el menú se maneja con el ratón),
  - cuando detecta el proceso de un juego, suelta el teclado y **cierra el
    menú** para liberar CPU; al cerrarse el juego, vuelve a abrir el menú.
  - Deja un registro en `kiosko.log` junto al `.exe`.
- **`limpiar.bat`** (doble clic) ejecuta `tools\limpiador.ps1`, que deshace
  todo lo anterior, para `kbdblock.exe` y **cierra la sesión** de Windows.

El script busca la app primero en `C:\kiosko\cyber_ahero\cyber_ahero\x86\` y,
si no está, junto a `kiosko.bat`.

## Notas sobre Windows XP

- En XP no suele haber aceleración 3D: la aplicación pide un *renderer*
  acelerado y, si falla, cae automáticamente al *renderer* **software**.
- Usa las DLLs de SDL2 **32 bits**. Las versiones oficiales de la rama SDL2
  (2.32.x) siguen soportando XP. Si en algún equipo XP diera problemas, se
  pueden usar versiones más antiguas (SDL2 2.0.22, SDL2_image 2.0.5,
  SDL2_ttf 2.0.18) editando las versiones en `tools\fetch_deps.ps1`.

## Estructura del proyecto

```
src/           código de la aplicación
third_party/   pl_mpeg.h (decodificador MPEG-1, cabecera única MIT)
tools/         scripts de dependencias, build, conversión de vídeo y scraping
games/         los juegos (cada subcarpeta es un juego, con su carátula y ficha)
assets/        logo
deps/          dependencias descargadas (SDL)
_toolchain/    compilador portable descargado (GCC/CMake/Ninja)
build/         salida de compilación
dist/          RARs generados por pack_rar
```
