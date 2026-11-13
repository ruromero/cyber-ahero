param(
    [string]$Version = "2.10.0",
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"
$dest = Join-Path $Root "_toolchain"
$bin = Join-Path $dest "w64devkit\bin\gcc.exe"

if (Test-Path -LiteralPath $bin) {
    Write-Host "El toolchain ya esta en $dest"
    exit 0
}

$url = "https://github.com/skeeto/w64devkit/releases/download/v$Version/w64devkit-x64-$Version.7z.exe"
$sfx = Join-Path $env:TEMP "w64devkit-x64-$Version.7z.exe"

Write-Host "Descargando w64devkit $Version (multilib x86+x64)..."
$wc = New-Object System.Net.WebClient
$wc.DownloadFile($url, $sfx)

if ((Get-Item $sfx).Length -lt (50MB)) {
    throw "Descarga incompleta del toolchain"
}

New-Item -ItemType Directory -Force -Path $dest | Out-Null
Write-Host "Extrayendo..."
$p = Start-Process -FilePath $sfx -ArgumentList "-y", "-o$dest" -PassThru -Wait
if ($p.ExitCode -ne 0) {
    throw "Fallo la extraccion (codigo $($p.ExitCode))"
}
Remove-Item -Force $sfx

Write-Host "Toolchain listo en: $dest"
