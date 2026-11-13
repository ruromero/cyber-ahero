param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"
$deps = Join-Path $Root "deps"
New-Item -ItemType Directory -Force -Path $deps | Out-Null

$packages = @(
    @{ Repo = "SDL";       Asset = "SDL2-devel-2.32.10-mingw.zip";        Folder = "SDL2-2.32.10";       Version = "2.32.10" },
    @{ Repo = "SDL_image"; Asset = "SDL2_image-devel-2.8.12-mingw.zip";   Folder = "SDL2_image-2.8.12";  Version = "2.8.12" },
    @{ Repo = "SDL_ttf";   Asset = "SDL2_ttf-devel-2.24.0-mingw.zip";     Folder = "SDL2_ttf-2.24.0";    Version = "2.24.0" }
)

foreach ($p in $packages) {
    $dest = Join-Path $deps ($p.Asset.Split("-")[0])
    if (Test-Path -LiteralPath $dest) {
        Write-Host "Ya existe: $dest"
        continue
    }

    $url = "https://github.com/libsdl-org/$($p.Repo)/releases/download/release-$($p.Version)/$($p.Asset)"
    $zip = Join-Path $env:TEMP $p.Asset
    Write-Host "Descargando $($p.Asset) ..."
    Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $zip

    $tmp = Join-Path $env:TEMP ("sdlx_" + [System.IO.Path]::GetRandomFileName())
    New-Item -ItemType Directory -Force -Path $tmp | Out-Null
    Expand-Archive -LiteralPath $zip -DestinationPath $tmp -Force

    $inner = Join-Path $tmp $p.Folder
    Move-Item -LiteralPath $inner -Destination $dest
    Remove-Item -Recurse -Force -LiteralPath $tmp
    Remove-Item -Force -LiteralPath $zip
    Write-Host "Listo: $dest"
}

Write-Host "Dependencias en: $deps"
