# Empaqueta una build en un .zip listo para publicar (lo usa el pipeline de CI).
#   tools\package.ps1 -Arch x86 -Version v1.0.0
#   tools\package.ps1 -Organizer -Version v1.0.0   (herramientas del Organizador)
# Compatible con Windows PowerShell 5.1.
param(
    [ValidateSet("x86", "x64")]
    [string]$Arch = "x64",
    [string]$Version = "dev",
    [string]$OutDir = "",
    [switch]$Organizer
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) { $OutDir = Join-Path $root "dist" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

function Copy-Into([string]$src, [string]$destDir) {
    if (-not (Test-Path -LiteralPath $src)) { throw "Falta: $src" }
    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    Copy-Item -LiteralPath $src -Destination $destDir -Recurse -Force
}

if ($Organizer) {
    $name = "cyber-ahero-$Version-organizador"
} else {
    $name = "cyber-ahero-$Version-$Arch"
}
$stage = Join-Path $env:TEMP ("pkg_" + [System.IO.Path]::GetRandomFileName())
$top = Join-Path $stage "cyber-ahero"
New-Item -ItemType Directory -Force -Path $top | Out-Null

foreach ($f in "README.md", "LICENSE", "THIRD_PARTY_NOTICES.md") {
    Copy-Into (Join-Path $root $f) $top
}

if ($Organizer) {
    # Descubrimiento y scraping: se ejecutan en un PC moderno, no en los portatiles.
    foreach ($f in "scrape.ps1", "scrape.cmd", "convert_preview.cmd") {
        Copy-Into (Join-Path $root "tools\$f") (Join-Path $top "tools")
    }
    Copy-Into (Join-Path $root "scrape.cfg.example") $top
    Copy-Into (Join-Path $root "cyber.cfg.example") $top
} else {
    $bdir = Join-Path $root "build\$Arch"
    if (-not (Test-Path -LiteralPath (Join-Path $bdir "cyber_ahero.exe"))) {
        throw "No hay build en $bdir. Ejecuta antes tools\build.ps1 -Arch $Arch"
    }
    foreach ($f in "cyber_ahero.exe", "SDL2.dll", "SDL2_image.dll", "SDL2_ttf.dll",
                   "kiosko.bat", "limpiar.bat", "cyber.cfg", "assets", "games") {
        Copy-Into (Join-Path $bdir $f) $top
    }
    Copy-Into (Join-Path $root "cyber.cfg.example") $top

    # Licencias de las DLLs de SDL que se redistribuyen.
    $lic = Join-Path $top "licenses"
    New-Item -ItemType Directory -Force -Path $lic | Out-Null
    foreach ($dep in "SDL2", "SDL2_image", "SDL2_ttf") {
        Copy-Item -LiteralPath (Join-Path $root "deps\$dep\LICENSE.txt") -Destination (Join-Path $lic "$dep.txt") -Force
    }
    Copy-Item -LiteralPath (Join-Path $root "deps\SDL2\README-SDL.txt") -Destination $lic -Force
}

$zip = Join-Path $OutDir "$name.zip"
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
# Compress-Archive in Windows PowerShell 5.1 writes entry names with '\',
# which other unzip tools turn into literal file names. Write '/' ourselves.
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($zip, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    # Same path form as Get-ChildItem below (TEMP may be an 8.3 short path).
    $base = (Get-Item -LiteralPath $stage).FullName.TrimEnd('\', '/')
    Get-ChildItem -LiteralPath $top -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($base.Length + 1).Replace('\', '/')
        [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $_.FullName, $rel, [System.IO.Compression.CompressionLevel]::Optimal)
    }
} finally {
    $archive.Dispose()
}
Remove-Item -Recurse -Force -LiteralPath $stage
Write-Host "Paquete -> $zip"
