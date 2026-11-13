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
   o doble clic sobre el icono). Mientras juegas, el lanzador se oculta y
   espera; al cerrar el juego vuelve solo.

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
| Salir | Botón X (arriba a la derecha) o Esc sin ficha fijada (Esc no sale en modo kiosko) |
| Moverse con mando | Cruceta o stick izquierdo |
| Abrir el juego con mando | Botón A (en mandos antiguos, botón 1) |
| Cerrar la ficha con mando | Botón B (en mandos antiguos, botón 2) |
| Abrir el Organizador | Botón de lista junto al de tema (arriba a la derecha) |
| Reescanear la biblioteca | F5 (fuera del modo kiosko) o botón Reescanear del Organizador |

Sirven los mandos tipo Xbox y los mandos DirectInput antiguos; se pueden
conectar o desconectar con el menú abierto. El mando solo controla el menú:
mientras hay un juego en primer plano el launcher lo ignora.

## Estructura de `games`

El repositorio solo trae `games/Ejemplo`, una Entry de muestra. Tus juegos,
carátulas y vídeos se quedan en tu copia local: `.gitignore` excluye el resto de
`games/`, porque ese contenido tiene copyright de sus dueños (ver
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)).

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
    juego.cfg             <- exe=Doom\doom.exe (relativo a las carpetas de instalación)
    cover.png
    details.txt
    preview.mpg
```

Para lanzar el juego se elige el primero que funcione de estos (ver
[Una biblioteca para todos los portátiles](#una-biblioteca-para-todos-los-portátiles)):

1. el **override** de este portátil, `exe.<carpeta>=` en `cyber.cfg`;
2. el `exe=` de su `juego.cfg` si es una ruta absoluta y existe;
3. el `exe=` relativo, probado en cada carpeta de `installs=` en el orden
   escrito (y después respecto a la carpeta del juego);
4. el nombre de ese ejecutable buscado hasta dos niveles de carpetas dentro de
   cada carpeta de `installs=`, en el orden escrito;
5. cualquier **acceso directo** (`*.lnk`, con el nombre que sea) de
   `games/<Nombre>/` cuyo destino existe en este portátil;
6. un **`.exe`** de la carpeta del juego (se elige el que más se parece al
   nombre de la carpeta y se descartan instaladores/servidores dedicados).

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
un archivo **`juego.cfg`** dentro de la carpeta del juego. Un `exe=` relativo
se busca primero en las carpetas de instalación (`installs=`, ver la sección
siguiente) y luego en la carpeta del juego; las demás rutas se resuelven
respecto a la carpeta del juego (se aceptan rutas absolutas). Las líneas con
`#` o `;` son comentarios:

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

### Argumentos, carpeta de trabajo y proceso a esperar

```
exe=DOSBox\dosbox.exe
args=-conf doom.conf -noconsole    <- argumentos para el ejecutable
dir=..\Doom                         <- carpeta de trabajo
wait=empires2.exe                   <- proceso real del juego (ver abajo)
```

- **`args=`**: se pasan tal cual al ejecutable, detrás de su ruta entre
  comillas (`"D:\Games\DOSBox\dosbox.exe" -conf doom.conf -noconsole`). Si un
  argumento lleva espacios, ponle tú las comillas.
- **`dir=`**: carpeta en la que arranca el juego. Relativa a la carpeta del
  ejecutable ya resuelto (`..\Doom` con el ejemplo de arriba es
  `D:\Games\DOSBox\..\Doom`) o absoluta. Sin `dir=`, el juego arranca en la
  carpeta de su ejecutable. Los accesos directos usan la carpeta que diga el
  propio `.lnk`.
- **`wait=`**: para juegos que arrancan con un lanzador/ayudante que se cierra
  enseguida. Es el nombre del ejecutable del juego de verdad (`empires2.exe`,
  `empires2` o `AoE2\empires2.exe` valen igual). El lanzador busca ese proceso
  durante un máximo de **60 segundos** y espera a que termine; si no aparece,
  vuelve el menú.

Al pulsar JUGAR el lanzador arranca el juego él mismo, oculta su ventana, deja
de dibujar y espera (sin gastar CPU) a que el juego termine. Entonces vuelve el
menú, se **reescanea la biblioteca** (aparecen los juegos instalados mientras
tanto) y se redibuja. Con un acceso directo se espera al proceso que devuelva
Windows; si no devuelve ninguno (p. ej. un `.lnk` a algo que no es un
programa), el menú vuelve enseguida, salvo que se indique `wait=`. Si el juego
no se puede arrancar, el menú muestra *No se pudo iniciar <juego>*.

### Mando en juegos que solo entienden teclado (`con=`, `con_args=`)

Para juegos antiguos sin soporte de mando, el juego puede ir acompañado de un
**mapeador externo de mando a teclado** (por ejemplo
[JoyToKey](https://joytokey.net/)) y sus argumentos, normalmente el perfil:

```
exe=Doom\doom.exe
con=JoyToKey\JoyToKey.exe          <- mapeador de mando a teclado
con_args=Doom                       <- sus argumentos (aquí, el perfil)
```

- **`con=`**: ejecutable del mapeador. Se resuelve igual que un `exe=`: una
  ruta absoluta se usa tal cual; una relativa se busca en cada carpeta de
  instalación (`installs=`) en orden, luego en la carpeta del juego (o la del
  `juego.cfg`) y por último por nombre de archivo hasta dos niveles dentro de
  las carpetas de instalación. Si no se encuentra, el juego **no** se oculta.
- **`con_args=`**: se pasan tal cual al mapeador, detrás de su ruta entre
  comillas. Con JoyToKey, el argumento es el nombre del perfil
  (`Doom` carga `Doom.cfg` de la carpeta de JoyToKey).

El mapeador arranca en su propia carpeta **justo antes** que el juego y se
**cierra cuando el juego termina**, antes de que vuelva el menú: primero se le
pide que cierre sus ventanas y, si a los 2 segundos sigue abierto, se termina.
Así nunca se queda funcionando en el menú ni en el siguiente juego. Si el
mapeador no se puede arrancar, el juego arranca igual y al volver el menú
muestra *No se pudo iniciar el mapeador del mando de <juego>* (también queda en
el log). Sin `con=`, todo funciona como siempre.

Consejo: instala JoyToKey en una carpeta de instalación (p. ej.
`C:\Juegos\JoyToKey`) con un perfil por juego, y no lo dejes en el arranque de
Windows: si ya hubiera otra copia abierta, JoyToKey podría pasarle el perfil y
cerrarse, y el lanzador no podría cerrar la que ya estaba.

## Una biblioteca para todos los portátiles

La misma carpeta `games` (la biblioteca maestra) se copia tal cual a todos los
portátiles. Lo que cambia en cada uno es su `cyber.cfg`, que dice dónde están
instalados los juegos **en ese portátil**:

- `installs=` lista las carpetas de instalación, separadas por `;`, en orden de
  preferencia;
- en `juego.cfg`, `exe=` se escribe **relativo** a esas carpetas
  (`exe=Quake3\quake3.exe`). Se prueba en cada carpeta de `installs=` en el
  orden escrito: si el juego está en dos, gana la primera;
- si no está con esa ruta (por ejemplo, la carpeta se llama distinto en este
  portátil, `Quake 3 Arena\quake3.exe`), se busca el **nombre del ejecutable**
  (`quake3.exe`) hasta **dos niveles** de carpetas dentro de cada carpeta de
  instalación, en el orden escrito. Más abajo no se busca, para que el arranque
  siga siendo rápido en los portátiles antiguos;
- si no aparece en ninguna, el juego se oculta (*instalación no encontrada*);
- las rutas absolutas en `exe=` y los accesos directos siguen funcionando;
- **override** por portátil: `exe.<nombre de la carpeta del juego>=<ruta
  absoluta>` en `cyber.cfg` gana a todo lo demás, para un juego instalado en un
  sitio raro sin tocar la biblioteca compartida.

Ejemplo: `games/Quake III/juego.cfg` contiene `exe=Quake3\quake3.exe`.

```
# cyber.cfg del portátil moderno (juegos en dos discos)
library=games
installs=C:\Juegos;D:\Games
```

Aquí se lanza `C:\Juegos\Quake3\quake3.exe`, o `D:\Games\Quake3\quake3.exe`
si no está en `C:`.

```
# cyber.cfg del portátil con XP (otra unidad, otra carpeta)
library=games
installs=E:\JUEGOS
exe.Quake III=C:\Archivos de programa\Quake III Arena\quake3.exe
```

Aquí el override manda: se lanza el Quake instalado con su instalador. Sin esa
línea se encontraría también `E:\JUEGOS\Quake 3 Arena\quake3.exe` gracias a
la búsqueda por nombre.

## Vista Organizador

Para preparar un portátil sin salir del menú: el botón con forma de lista, junto
al botón de tema (arriba a la derecha), abre la vista **Organizador** de la
biblioteca de este portátil. Muestra:

- arriba, cuántos juegos hay **disponibles**, **ocultos** e **incompletos**;
- los juegos **ocultos** y por qué: *sin información para lanzarlo* (la carpeta
  no tiene acceso directo, ni `exe=`, ni ejecutable) o *instalación no
  encontrada* (la hay, pero el juego no está instalado en este portátil);
- los juegos disponibles a los que les **falta** portada (`cover.png`),
  vista previa (`preview.mpg`) o ficha (`details.txt`), con lo que falta.

Los juegos completos no aparecen. Si la lista no cabe, se desplaza con la rueda
del ratón o con los botones `^` y `v`. Todo se maneja solo con el ratón.

**Reescanear** vuelve a leer la biblioteca y actualiza el menú y la vista: tras
instalar un juego o arreglar su `juego.cfg`, pulsarlo basta para verlo, sin
reiniciar el launcher. Fuera del modo kiosko, **F5** hace lo mismo desde
cualquier pantalla. **Cerrar**, Esc o un clic fuera del recuadro vuelven al menú.
El aviso «Biblioteca reescaneada: N juegos» solo sale al reescanear a mano; al
volver de un juego la biblioteca se reescanea sin avisar.

### Registro para diagnosticar (`cyber.log`)

El launcher escribe un registro de texto plano, **`cyber.log`**, junto a
`cyber_ahero.exe`, tanto en ventana normal como en modo kiosko. Se va añadiendo
entre ejecuciones (si el launcher se cuelga y vuelve a arrancar, lo anterior
sigue ahí). Cuando pasa de 1 MB aproximadamente se renombra a
**`cyber.old.log`** (sustituyendo al anterior) y se empieza uno nuevo. Cada
línea lleva fecha y hora. Contiene:

- **cada escaneo** (al arrancar, al volver de un juego y al reescanear a mano):
  cada juego disponible con la ruta completa de lo que va a lanzar y qué regla
  lo encontró (`exe.<carpeta>=` de `cyber.cfg`, `exe=` absoluto, `exe=`
  relativo en la carpeta de instalación N, `exe=` relativo en la carpeta del
  juego, búsqueda por nombre en las carpetas de instalación, acceso directo o
  elección automática); y cada juego oculto con el motivo y el `exe=` (y el
  `exe.<carpeta>=`) que buscaba;
- **cada partida**: juego iniciado, terminado o que no se pudo iniciar, con la
  ruta exacta lanzada, sus argumentos, la carpeta de trabajo y, si se usan,
  `wait=` y el mapeador `con=`;
- en modo kiosko, además, lo que hace el kiosko (ver [Modo kiosko](#modo-kiosko)).

Ejemplo:

```
2026-11-13 21:04:10  Escaneo de la biblioteca (inicio): C:\Cyber\games
2026-11-13 21:04:10    Disponible: Quake III -> D:\Juegos\Quake3\quake3.exe (exe= relativo, carpeta de instalacion 2)
2026-11-13 21:04:10    Oculto: Diablo II: instalacion no encontrada (buscaba exe=Diablo II\Game.exe)
2026-11-13 21:04:10  Escaneo terminado: 1 disponibles, 1 ocultos
2026-11-13 21:05:32  Juego iniciado: Quake III -> D:\Juegos\Quake3\quake3.exe +set fs_game baseq3 (carpeta D:\Juegos\Quake3)
```

## Elegir la ruta de la biblioteca

La carpeta de juegos, por orden de prioridad:

1. Primer argumento: `cyber_ahero.exe "D:\Juegos"`
2. Variable de entorno: `CYBER_GAMES=D:\Juegos`
3. Archivo `cyber.cfg` junto al `.exe` con una línea `library=D:\Juegos`
4. Por defecto: carpeta `games` junto al `.exe`

En `cyber.cfg`, una ruta relativa se resuelve respecto a la carpeta del `.exe`.
`cyber.cfg` se lee una sola vez, al arrancar.

## Modo atracción (`attract=`)

Si nadie toca el launcher durante un rato, pasa los juegos a pantalla
completa, como una recreativa: cada juego disponible, en el orden de la
biblioteca, con su preview durante 20 segundos (o hasta que acabe el vídeo,
lo que ocurra antes); los juegos sin preview muestran su carátula 8 segundos.

Cualquier movimiento del ratón, tecla o botón del mando vuelve al menú, y esa
pulsación no hace nada más (nunca abre un juego). Mientras hay un juego
abierto (el launcher no tiene el foco), el modo atracción no arranca.

Los minutos sin uso se fijan en `cyber.cfg` con una línea `attract=`:
`attract=3` es el valor por defecto, `attract=1` arranca tras un minuto y
`attract=0` lo desactiva.

Un valor que no sea un número entero (`attract=pronto`, `attract=-1`) deja los
3 minutos por defecto.

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
- el modo kiosko: `kiosko.bat` y `limpiar.bat` en la raíz, junto al `.exe`
  (ver [Modo kiosko](#modo-kiosko)).

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

Qué instalar (y cómo desinstalarlo) en macOS y Windows para compilar y pasar
los tests: [docs/dev-setup.md](docs/dev-setup.md).

## Distribuir

### Descargar una versión compilada

Cada versión publicada (etiqueta `v*`) tiene en
[Releases](../../releases) tres `.zip`, generados por el pipeline de GitHub
Actions (`.github/workflows/build.yml`):

| Archivo | Para qué |
| --- | --- |
| `cyber-ahero-<versión>-x86.zip` | Portátiles con Windows XP (32 bits); también funciona en Windows de 64 bits |
| `cyber-ahero-<versión>-x64.zip` | Portátiles con Windows de 64 bits |
| `cyber-ahero-<versión>-organizador.zip` | Herramientas del Organizador para su PC moderno: scraping y descubrimiento (`tools\scrape.cmd`) |

Cada zip del lanzador trae el `.exe`, las DLLs de SDL, el logo, los `.bat` del
modo kiosko, `games/Ejemplo`, los `.example` y las licencias. Copia tu propia
carpeta `games` al lado, o apunta a ella con `cyber.cfg`.

Cada push a `main` también compila, pasa los tests en Windows y deja los zips
como artefacto de la ejecución. Para publicar una versión:

```
git tag v1.0.0
git push origin v1.0.0
```

Para generar los mismos zips en local, tras compilar:

```
powershell -File tools\package.ps1 -Arch x86 -Version v1.0.0
powershell -File tools\package.ps1 -Organizer -Version v1.0.0
```

### A mano

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

### Descubrir juegos nuevos (`-Discover`)

En el PC moderno que tiene la copia maestra de las instalaciones, el modo
descubrimiento crea las entradas que faltan en la biblioteca maestra:

```
tools\scrape.cmd -Discover -DryRun                        (solo muestra lo que crearía)
tools\scrape.cmd -Discover                                (usa installs= de cyber.cfg)
tools\scrape.cmd -Discover -Installs "C:\Juegos;D:\Games" -Root D:\games
tools\scrape.cmd -Discover -Video                         (además, vídeo de preview)
tools\scrape.cmd -Discover -Force                         (rehace entradas con el mismo nombre)
```

- **Candidatos**: cada carpeta de primer nivel de una carpeta de instalación
  (`-Installs`, o `installs=` de `cyber.cfg`) a la que no llega ninguna
  entrada. Para decidirlo aplica las mismas reglas que el lanzador (override
  `exe.<carpeta>=`, `exe=` absoluto, `exe=` relativo en cada carpeta de
  instalación, búsqueda por nombre del ejecutable a dos niveles, accesos
  directos). Una carpeta que ya tiene entrada en una carpeta de instalación no
  se vuelve a proponer aunque haya otra copia con el mismo nombre en otra.
- **Ejecutable**: busca `.exe` en la carpeta del juego (si no hay, un nivel más
  abajo, y luego otro) y los puntúa como el lanzador: gana el que se llama como
  la carpeta; instaladores, desinstaladores, `setup`, servidores, `config`,
  `update` y `crash` puntúan muy bajo.
- **Entrada escrita**: `games\<carpeta>\juego.cfg` con `exe=<carpeta>\...`
  relativo a la carpeta de instalación (y `args=` si se dieron, `nombre=` si
  el nombre de IGDB es distinto del de la carpeta), más carátula, `details.txt`
  y `descripcion.txt` de IGDB, y `preview.mpg` con `-Video`. Así la entrada
  funciona en cualquier portátil que tenga el juego con el mismo nombre de
  carpeta en alguna de sus carpetas de instalación.
- **Preguntas**: solo cuando no está seguro. Pregunta **qué juego es** si IGDB
  no devuelve exactamente un resultado con el nombre de la carpeta (número de
  la lista, otro nombre para buscar, o Enter para dejarla sin datos), y **qué
  ejecutable y con qué argumentos** si hay varios candidatos sin un claro
  ganador (número, ruta relativa, o Enter para el propuesto; Enter en los
  argumentos = ninguno).
- `-DryRun` no escribe nada ni pregunta nada: lista las entradas que crearía,
  con el ejecutable propuesto y las preguntas que haría.
- No toca entradas existentes: si ya existe `games\<carpeta>` (aunque no
  apunte a nada) la omite; `-Force` la rehace. Ejecutarlo dos veces no crea
  duplicados, porque las entradas escritas ya apuntan a sus instalaciones.
- `-LaptopConfig <ruta>` usa otro `cyber.cfg` (por defecto el de la raíz del
  repositorio).

Los tests de este modo (Pester) están en `tools\tests\`:
`Invoke-Pester tools\tests` (ver [docs/dev-setup.md](docs/dev-setup.md)).

## Modo kiosko

El propio `cyber_ahero.exe` deja el portátil bloqueado en el menú de juegos.
No hace falta PowerShell ni nada más instalado: funciona igual en Windows XP
y en un portátil moderno de 64 bits.

- **`kiosko.bat`** (doble clic) ejecuta `cyber_ahero.exe --kiosko`, que:
  - aplica restricciones en el registro: sin Administrador de tareas, sin
    Ejecutar (Win+R) y sin tecla Windows (para el usuario y, con permisos de
    administrador, también para la máquina), y sin botón Inicio,
  - con administrador, evita que Windows reinicie `explorer.exe`
    (`AutoRestartShell=0`) y desactiva Ctrl+Alt+Supr (`DisableCAD=1`),
  - se registra para **arrancar solo** al iniciar sesión
    (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, valor
    `CiberAhKiosko`, que apunta a `kiosko.bat`),
  - cierra `explorer.exe` (sin escritorio, barra de tareas ni menú Inicio) y
    lo mantiene cerrado mientras dure el modo kiosko,
  - **bloquea el teclado** mientras el menú está en pantalla (el menú se
    maneja con el ratón o el mando); al abrir un juego suelta el teclado y lo
    vuelve a bloquear cuando el juego termina y vuelve el menú.
- `kiosko.bat` vuelve a abrir el launcher a los pocos segundos si se cierra
  por cualquier motivo (un fallo, o alguien lo mata) que no sea el botón Salir.
- **Botón Salir** (la X arriba a la derecha, junto al Organizador y al tema):
  deshace todas las restricciones, quita el arranque automático (también de
  las claves que usaban versiones antiguas, `HKLM\...\Windows\CurrentVersion\Run`
  y la errónea `HKLM\...\Windows NT\CurrentVersion\Run`), vuelve a abrir el
  escritorio y sale. **No cierra la sesión**; algunas políticas (tecla
  Windows, Ejecutar) solo vuelven del todo en el siguiente inicio de sesión.
- **`limpiar.bat`** (doble clic) es la recuperación: ejecuta
  `cyber_ahero.exe --limpiar`, que cierra el launcher si está en marcha (y
  con él el bucle de `kiosko.bat`), deshace todo lo anterior y vuelve a abrir
  el escritorio, sin mostrar nada. Sirve aunque el menú no esté abierto (por
  ejemplo, si el menú no llega a arrancar). Deshace los ajustes de la cuenta
  con la que se ejecuta: usa la misma cuenta que el kiosko.
- Sin administrador, el modo kiosko arranca igual con lo que se pueda
  aplicar; el registro dice qué ajustes no se aplicaron.
- Registro: el modo kiosko escribe en **`cyber.log`** junto al `.exe` (ver
  [Registro para diagnosticar](#registro-para-diagnosticar-cyberlog)), que
  sustituye al antiguo `kiosko.log`: ajustes aplicados y deshechos (y cuáles
  no se pudieron aplicar sin administrador), Salir, la recuperación, además
  de los escaneos y las partidas como en ventana normal.

Sin `--kiosko`, el launcher es una ventana normal: el botón Salir y Esc
simplemente cierran, y F5 reescanea.

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
tools/tests/   tests Pester de scrape.ps1 (descubrimiento)
games/         la biblioteca (en el repo solo games/Ejemplo; el resto es local)
assets/        logo
deps/          dependencias descargadas (SDL)
_toolchain/    compilador portable descargado (GCC/CMake/Ninja)
build/         salida de compilación
dist/          zips de tools/package.ps1 y RARs de pack_rar
.github/       pipeline de GitHub Actions (build win32/win64, tests, releases)
```

## Licencia

Código bajo licencia [MIT](LICENSE). Las licencias de pl_mpeg, SDL2,
SDL2_image, SDL2_ttf, FreeType y HarfBuzz, y qué contenido **no** se incluye
(juegos, carátulas, vídeos), están en
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
