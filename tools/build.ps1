param(
    [ValidateSet("x86", "x64")]
    [string]$Arch = "x64",
    [string]$CC = "",
    [string]$CMake = "cmake",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

$toolchain = Join-Path $root "_toolchain\w64devkit"
$haveToolchain = Test-Path -LiteralPath (Join-Path $toolchain "bin\gcc.exe")
if ($haveToolchain) {
    $tcBin = Join-Path $toolchain "bin"
    if (($env:PATH -split ";") -notcontains $tcBin) { $env:PATH = "$tcBin;$env:PATH" }
}

if ($Arch -eq "x86") {
    $triplet = "i686-w64-mingw32"
    $buildName = "build\x86"
    if ($haveToolchain -and -not $CC) { $CC = Join-Path $toolchain "bin\i686-w64-mingw32-gcc.exe" }
} else {
    $triplet = "x86_64-w64-mingw32"
    $buildName = "build\x64"
    if ($haveToolchain -and -not $CC) { $CC = Join-Path $toolchain "bin\gcc.exe" }
}

if (-not $PSBoundParameters.ContainsKey("CMake")) {
    $candidates = @()
    if ($haveToolchain) { $candidates += (Join-Path $toolchain "bin\cmake.exe") }
    if ($CC) { $candidates += (Join-Path (Split-Path $CC -Parent) "cmake.exe") }
    $candidates += (Join-Path $env:ProgramFiles "CMake\bin\cmake.exe")
    foreach ($c in $candidates) {
        if ($c -and (Test-Path -LiteralPath $c)) { $CMake = $c; break }
    }
}

$deps = Join-Path $root "deps"
if (-not (Test-Path -LiteralPath (Join-Path $deps "SDL2"))) {
    throw "Faltan las dependencias. Ejecuta primero: tools\fetch_deps.ps1"
}

$bdir = Join-Path $root $buildName
if ($Clean -and (Test-Path -LiteralPath $bdir)) {
    Remove-Item -Recurse -Force -LiteralPath $bdir
}
New-Item -ItemType Directory -Force -Path $bdir | Out-Null

$gen = "MinGW Makefiles"
if (Get-Command ninja -ErrorAction SilentlyContinue) { $gen = "Ninja" }

$sdl2    = Join-Path $deps "SDL2\$triplet\lib\cmake\SDL2"
$sdl2img = Join-Path $deps "SDL2_image\$triplet\lib\cmake\SDL2_image"
$sdl2ttf = Join-Path $deps "SDL2_ttf\$triplet\lib\cmake\SDL2_ttf"

$cmakeArgs = @(
    "-S", $root,
    "-B", $bdir,
    "-G", $gen,
    "-DCMAKE_BUILD_TYPE=Release",
    "-DSDL2_DIR=$sdl2",
    "-DSDL2_image_DIR=$sdl2img",
    "-DSDL2_ttf_DIR=$sdl2ttf"
)
if ($CC) { $cmakeArgs += "-DCMAKE_C_COMPILER=$CC" }

& $CMake @cmakeArgs
if ($LASTEXITCODE -ne 0) { throw "Fallo la configuracion de CMake" }

& $CMake --build $bdir --config Release
if ($LASTEXITCODE -ne 0) { throw "Fallo la compilacion" }

foreach ($dep in "SDL2", "SDL2_image", "SDL2_ttf") {
    $bin = Join-Path $deps "$dep\$triplet\bin"
    if (Test-Path -LiteralPath $bin) {
        Copy-Item -Path (Join-Path $bin "*.dll") -Destination $bdir -Force
    }
}

$logo = Join-Path $root "assets\ahero-acronyms4267x4267.png"
if (Test-Path -LiteralPath $logo) {
    $assetsBin = Join-Path $bdir "assets"
    New-Item -ItemType Directory -Force -Path $assetsBin | Out-Null
    Copy-Item -LiteralPath $logo -Destination $assetsBin -Force
}

$games = Join-Path $root "games"
$gamesBin = Join-Path $bdir "games"
if (Test-Path -LiteralPath $games) {
    if (Test-Path -LiteralPath $gamesBin) { Remove-Item -Recurse -Force -LiteralPath $gamesBin }
    Copy-Item -Recurse -Force -LiteralPath $games -Destination $gamesBin
    Write-Host "Juegos copiados -> $gamesBin"
}

$cfg = Join-Path $bdir "cyber.cfg"
$lines = @()
if (Test-Path -LiteralPath $games) { $lines += "library=games" }
Set-Content -LiteralPath $cfg -Value $lines -Encoding ASCII

# Los scripts de kiosco van en tools\ dentro del build; los .bat, en la raiz
# (para lanzarlos con doble clic).
$toolsBin = Join-Path $bdir "tools"
New-Item -ItemType Directory -Force -Path $toolsBin | Out-Null
foreach ($s in @("kiosko.ps1", "limpiador.ps1")) {
    $src = Join-Path $root "tools\$s"
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $toolsBin -Force
    }
}
foreach ($s in @("kiosko.bat", "limpiar.bat")) {
    $src = Join-Path $root "tools\$s"
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination (Join-Path $bdir $s) -Force
    }
}
# Limpia copias antiguas de los .ps1 en la raiz (ahora viven en tools\)
foreach ($s in @("kiosko.ps1", "limpiador.ps1", "autologon.ps1", "autologon.bat")) {
    $stale = Join-Path $bdir $s
    if (Test-Path -LiteralPath $stale) { Remove-Item -LiteralPath $stale -Force }
}
Write-Host "Scripts de kiosko copiados -> $toolsBin"

# Bloqueador de teclado del modo kiosko (hook global WH_KEYBOARD_LL)
$kbdSrc = Join-Path $root "tools\kbdblock.c"
if ($CC -and (Test-Path -LiteralPath $kbdSrc)) {
    $kbdExe = Join-Path $bdir "kbdblock.exe"
    & $CC $kbdSrc -o $kbdExe -mwindows -O2 -s
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Bloqueador de teclado -> $kbdExe"
    } else {
        Write-Warning "No se pudo compilar kbdblock.exe"
    }
}

Write-Host "Compilado -> $bdir"
