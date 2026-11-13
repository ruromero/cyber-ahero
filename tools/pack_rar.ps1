param(
    [ValidateSet('x64', 'x86')]
    [string]$Build = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Find-Rar {
    $c = Get-Command rar.exe -ErrorAction SilentlyContinue
    if ($c) { return $c.Source }
    foreach ($p in @(
            "$env:ProgramFiles\WinRAR\Rar.exe",
            "${env:ProgramFiles(x86)}\WinRAR\Rar.exe")) {
        if (Test-Path -LiteralPath $p) { return $p }
    }
    return $null
}

$rar = Find-Rar
if (-not $rar) {
    Write-Host "No se encontro rar.exe. Instala WinRAR o anade rar.exe al PATH." -ForegroundColor Red
    exit 1
}

if ($Build) {
    $dir = Join-Path $root "build\$Build"
    if (-not (Test-Path -LiteralPath (Join-Path $dir 'cyber_ahero.exe'))) {
        Write-Host "No hay build en $dir. Compila primero." -ForegroundColor Red
        exit 1
    }
} else {
    $candidates = @('x64', 'x86') | ForEach-Object {
        $exe = Join-Path $root "build\$_\cyber_ahero.exe"
        if (Test-Path -LiteralPath $exe) {
            [pscustomobject]@{ Arch = $_; Dir = (Split-Path $exe -Parent); Time = (Get-Item $exe).LastWriteTime }
        }
    }
    if (-not $candidates) {
        Write-Host "No hay ninguna build. Usa tools\build_x64.cmd o tools\build_x86.cmd primero." -ForegroundColor Red
        exit 1
    }
    $latest = $candidates | Sort-Object Time -Descending | Select-Object -First 1
    $Build = $latest.Arch
    $dir = $latest.Dir
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$dist = Join-Path $root 'dist'
New-Item -ItemType Directory -Force -Path $dist | Out-Null
$stage = Join-Path $dist "stage-$stamp"
New-Item -ItemType Directory -Force -Path $stage | Out-Null

Copy-Item -Path (Join-Path $dir 'cyber_ahero.exe') -Destination $stage -Force
Copy-Item -Path (Join-Path $dir '*.dll') -Destination $stage -Force
foreach ($extra in @('games')) {
    $p = Join-Path $root $extra
    if (Test-Path -LiteralPath $p) {
        Copy-Item -Recurse -Path $p -Destination (Join-Path $stage $extra) -Force
    }
}
foreach ($extra in 'README.md', 'cyber.cfg.example', 'scrape.cfg.example') {
    $p = Join-Path $root $extra
    if (Test-Path -LiteralPath $p) { Copy-Item -Path $p -Destination $stage -Force }
}

$out = Join-Path $dist "cyber-ahero-$stamp.rar"
Write-Host "Empaquetando build $Build -> $out"
Push-Location $stage
try {
    & $rar a -r -m5 -ep1 $out '*'
} finally {
    Pop-Location
}

$code = $LASTEXITCODE
Remove-Item -Recurse -Force -LiteralPath $stage
if ($code -ne 0) {
    Write-Host "Rar termino con codigo $code" -ForegroundColor Red
    exit $code
}

$size = [math]::Round((Get-Item -LiteralPath $out).Length / 1MB, 2)
Write-Host "Listo: $out ($size MB)" -ForegroundColor Green
