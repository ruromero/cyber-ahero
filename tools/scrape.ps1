param(
    [string]$Root = '',
    [string]$Config = '',
    [switch]$Force,
    [switch]$CoverOnly,
    [switch]$Video,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$root = Split-Path -Parent $PSScriptRoot

if (-not $Root) {
    $cfgRoot = Join-Path $root 'cyber.cfg'
    if (Test-Path -LiteralPath $cfgRoot) {
        foreach ($line in Get-Content -LiteralPath $cfgRoot) {
            if ($line -match '^\s*library\s*=\s*(.+?)\s*$') {
                $Root = $Matches[1].Trim('"')
            }
        }
    }
    if (-not $Root) { $Root = Join-Path $root 'games' }
}
$Root = (Resolve-Path -LiteralPath $Root -ErrorAction SilentlyContinue).Path
if (-not $Root) {
    Write-Host "No existe la carpeta de juegos. Uso: scrape.ps1 -Root <carpeta>" -ForegroundColor Red
    exit 1
}

$cfg = @{}
if (-not $Config) { $Config = Join-Path $root 'scrape.cfg' }
if (Test-Path -LiteralPath $Config) {
    foreach ($line in Get-Content -LiteralPath $Config) {
        if ($line -match '^\s*([^#;][^=]*?)\s*=\s*(.*?)\s*$') {
            $cfg[$Matches[1].Trim().ToLower()] = $Matches[2].Trim()
        }
    }
}
function Cfg($name, $envName) {
    if ($cfg.ContainsKey($name) -and $cfg[$name]) { return $cfg[$name] }
    $v = [Environment]::GetEnvironmentVariable($envName)
    if ($v) { return $v }
    return ''
}


$igdbId = Cfg 'igdb_client_id' 'IGDB_CLIENT_ID'
$igdbSecret = Cfg 'igdb_client_secret' 'IGDB_CLIENT_SECRET'
$sgdbKey = Cfg 'steamgriddb_key' 'STEAMGRIDDB_KEY'

if (-not $igdbId -or -not $igdbSecret) {
    Write-Host "Faltan las credenciales de IGDB." -ForegroundColor Red
    Write-Host "Crea una app en https://dev.twitch.tv/console/apps y pon las claves en scrape.cfg:"
    Write-Host "  igdb_client_id=xxxx"
    Write-Host "  igdb_client_secret=xxxx"
    Write-Host "(opcional) steamgriddb_key=xxxx  -> https://www.steamgriddb.com/profile/preferences/api"
    exit 1
}

function Get-IgdbToken {
    $uri = "https://id.twitch.tv/oauth2/token?client_id=$igdbId&client_secret=$igdbSecret&grant_type=client_credentials"
    return (Invoke-RestMethod -Method Post -Uri $uri).access_token
}

$token = Get-IgdbToken
$igdbHeaders = @{ 'Client-ID' = $igdbId; 'Authorization' = "Bearer $token" }

function Invoke-Igdb($endpoint, $body) {
    return Invoke-RestMethod -Method Post -Uri "https://api.igdb.com/v4/$endpoint" -Headers $igdbHeaders -Body $body -ContentType 'text/plain'
}

function Get-CleanName($n) {
    $n = $n -replace '\[[^\]]*\]', ' '
    $n = $n -replace '\([^\)]*\)', ' '
    $n = $n -replace '\b(GOG|Repack|RIP|Portable|Multilenguaje|Multi\d*|Espa[nñ]ol|Spanish|English|Update|v\d[\d.]*)\b', ' '
    $n = $n -replace '[_]+', ' '
    $n = $n -replace '\s+', ' '
    return $n.Trim()
}

function Get-Norm($s) {
    if (-not $s) { return '' }
    $s = $s.ToLower()
    $s = $s -replace '[^a-z0-9]+', ''
    return $s
}

function Select-IgdbGame($clean) {
    $q = $clean.Replace('"', ' ')
    $body = "search `"$q`"; fields name,summary,first_release_date,genres.name,game_modes.name,involved_companies.company.name,involved_companies.developer,involved_companies.publisher,cover.url,platforms.id; limit 12;"
    $res = Invoke-Igdb 'games' $body
    if (-not $res) { return $null }

    $nq = Get-Norm $clean
    $best = $null
    $bestScore = -1
    foreach ($g in $res) {
        $score = 0
        $ng = Get-Norm $g.name
        if ($ng -eq $nq) { $score += 100 }
        elseif ($ng.StartsWith($nq) -or $nq.StartsWith($ng)) { $score += 30 }
        if ($g.platforms) {
            foreach ($p in $g.platforms) {
                if ($p.id -eq 6) { $score += 25 }
                elseif ($p.id -eq 13) { $score += 15 }
            }
        }
        if ($g.cover) { $score += 10 }
        if ($g.first_release_date) { $score += 3 }
        if ($score -gt $bestScore) { $bestScore = $score; $best = $g }
    }
    return $best
}

function Download-Cover($url, $dest) {
    if (-not $url) { return $false }
    if ($url.StartsWith('//')) { $url = 'https:' + $url }
    $url = $url -replace '/t_[a-z0-9_]+/', '/t_cover_big/'
    try {
        Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing
        return $true
    } catch {
        return $false
    }
}

function Download-SgdbCover($name, $dest) {
    if (-not $sgdbKey) { return $false }
    $h = @{ Authorization = "Bearer $sgdbKey" }
    try {
        $term = [uri]::EscapeDataString($name)
        $search = Invoke-RestMethod -Uri "https://www.steamgriddb.com/api/v2/search/autocomplete/$term" -Headers $h
        if (-not $search.data -or $search.data.Count -eq 0) { return $false }
        $id = $search.data[0].id
        $grids = Invoke-RestMethod -Uri "https://www.steamgriddb.com/api/v2/grids/game/$id?dimensions=600x900" -Headers $h
        if (-not $grids.data -or $grids.data.Count -eq 0) { return $false }
        Invoke-WebRequest -Uri $grids.data[0].url -OutFile $dest -UseBasicParsing
        return $true
    } catch {
        return $false
    }
}

function Write-Text($path, $content) {
    $enc = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($path, $content, $enc)
}

function Copy-GameIco($ico, $dest) {
    if ([string]::Equals($ico, $dest, [StringComparison]::OrdinalIgnoreCase)) { return }
    Copy-Item -LiteralPath $ico -Destination $dest -Force
}

function Find-GameIco($dir) {
    $icos = Get-ChildItem -LiteralPath $dir -File -Filter '*.ico' -ErrorAction SilentlyContinue
    if (-not $icos) { return $null }
    $best = $null; $bestScore = -1
    foreach ($f in $icos) {
        $n = $f.BaseName.ToLower()
        $s = 0
        if ($n -match 'icon|cover|logo') { $s += 10 }
        if ($s -gt $bestScore) { $bestScore = $s; $best = $f }
    }
    return $best.FullName
}

function Test-Tool($name) {
    return [bool](Get-Command $name -ErrorAction SilentlyContinue)
}

function Get-Video($query, $destDir) {
    $tmp = Join-Path $destDir 'preview.tmp.mp4'
    $mpg = Join-Path $destDir 'preview.mpg'
    $null = & yt-dlp --no-playlist --no-warnings --max-downloads 1 `
        -f "bv*[height<=720]+ba/b[height<=720]/b" --merge-output-format mp4 `
        -o $tmp "ytsearch1:$query" 2>&1
    if (-not (Test-Path -LiteralPath $tmp)) { return $false }
    $null = & ffmpeg -y -i $tmp -vf "scale=480:360:force_original_aspect_ratio=increase,crop=480:360,fps=25" `
        -c:v mpeg1video -b:v 1200k -bf 0 -c:a mp2 -ar 44100 -b:a 128k -f mpeg $mpg 2>&1
    Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue
    return (Test-Path -LiteralPath $mpg)
}

if ($Video) {
    if (-not (Test-Tool 'yt-dlp')) {
        Write-Host "yt-dlp no esta en el PATH; se omite el video. Instala: winget install yt-dlp" -ForegroundColor Yellow
        $Video = $false
    } elseif (-not (Test-Tool 'ffmpeg')) {
        Write-Host "ffmpeg no esta en el PATH; se omite el video." -ForegroundColor Yellow
        $Video = $false
    }
}

$folders = Get-ChildItem -LiteralPath $Root -Directory | Where-Object { -not ($_.Attributes -band [IO.FileAttributes]::Hidden) }
if (-not $folders) {
    Write-Host "No hay juegos en $Root" -ForegroundColor Red
    exit 1
}

Write-Host "Biblioteca: $Root ($($folders.Count) juegos)"
$ok = 0; $skip = 0; $fail = @()

foreach ($f in $folders) {
    $outDir = $f.FullName
    $coverPng = Join-Path $outDir 'cover.png'
    $coverIco = Join-Path $outDir 'cover.ico'
    $ficha = Join-Path $outDir 'details.txt'
    $hasCover = (Test-Path -LiteralPath $coverIco) -or (Test-Path -LiteralPath $coverPng)
    $hasFicha = Test-Path -LiteralPath $ficha
    if (-not $Force -and $hasCover -and ($CoverOnly -or $hasFicha)) {
        $skip++
        continue
    }

    $clean = Get-CleanName $f.Name
    Write-Host ("  {0}" -f $f.Name) -NoNewline

    $game = $null
    try { $game = Select-IgdbGame $clean } catch { $game = $null }

    if (-not $game) {
        $ico = Find-GameIco $f.FullName
        if ($ico -and -not $DryRun) {
            Copy-GameIco $ico $coverIco
            Write-Host "  -> icono del juego (sin ficha IGDB)" -ForegroundColor Yellow
            $ok++
        } else {
            Write-Host "  -> sin resultados" -ForegroundColor Yellow
            $fail += $f.Name
        }
        continue
    }

    if ($DryRun) {
        Write-Host ("  -> {0}" -f $game.name) -ForegroundColor Cyan
        continue
    }

    $gotCover = $false
    if ($hasCover -and -not $Force) {
        $gotCover = $true
    } else {
        $ico = Find-GameIco $f.FullName
        if ($ico) {
            Copy-GameIco $ico $coverIco
            $gotCover = $true
        }
    }
    if (-not $gotCover -and $game.cover) {
        $gotCover = Download-Cover $game.cover.url $coverPng
    }
    if (-not $gotCover) {
        $gotCover = Download-SgdbCover $clean $coverPng
    }

    $year = $null
    if (-not $CoverOnly) {
        $rows = New-Object System.Collections.Generic.List[string]
        $genero = "G" + [char]0x00E9 + "nero: "
        $ano = "A" + [char]0x00F1 + "o: "
        if ($game.genres) { $rows.Add($genero + (($game.genres | ForEach-Object { $_.name }) -join ', ')) }
        if ($game.first_release_date) {
            $year = [DateTimeOffset]::FromUnixTimeSeconds($game.first_release_date).Year
            $rows.Add("$ano$year")
        }
        if ($game.game_modes) { $rows.Add("Jugadores: " + (($game.game_modes | ForEach-Object { $_.name }) -join ', ')) }
        if ($game.involved_companies) {
            $devs = $game.involved_companies | Where-Object { $_.developer } | ForEach-Object { $_.company.name }
            $pubs = $game.involved_companies | Where-Object { $_.publisher } | ForEach-Object { $_.company.name }
            if ($devs) { $rows.Add("Desarrollador: " + ($devs -join ', ')) }
            if ($pubs) { $rows.Add("Distribuidora: " + ($pubs -join ', ')) }
        }
        if ($game.platforms) {
            $names = @(); foreach ($p in $game.platforms) { if ($p.id -eq 6) { $names += 'Windows' } elseif ($p.id -eq 13) { $names += 'DOS' } }
            if ($names) { $rows.Add("Plataforma: " + (($names | Select-Object -Unique) -join ', ')) }
        }
        if ($rows.Count -gt 0) { Write-Text $ficha (($rows -join "`r`n") + "`r`n") }
        if ($game.summary) { Write-Text (Join-Path $outDir 'descripcion.txt') ($game.summary + "`r`n") }
    }

    if ($Video -and -not $CoverOnly) {
        $mpg = Join-Path $outDir 'preview.mpg'
        if ($Force -or -not (Test-Path -LiteralPath $mpg)) {
            $q = $game.name
            if ($year) { $q = "$q $year" }
            $q = "$q PC"
            Write-Host ("    video: {0}" -f $q) -ForegroundColor DarkGray
            $null = Get-Video $q $outDir
        }
    }

    $flag = if ($gotCover) { '' } else { ' (sin caratula)' }
    Write-Host ("  -> {0}{1}" -f $game.name, $flag) -ForegroundColor Green
    $ok++
    Start-Sleep -Milliseconds 280
}

Write-Host ""
Write-Host "Hecho: $ok actualizados, $skip omitidos, $($fail.Count) sin resultados." -ForegroundColor Green
if ($fail.Count -gt 0) {
    Write-Host "Sin resultados: $($fail -join ', ')" -ForegroundColor Yellow
}
