param(
    [string]$Root = '',
    [string]$Config = '',
    [switch]$Force,
    [switch]$CoverOnly,
    [switch]$Video,
    [switch]$DryRun,
    # Discovery mode: new Entries for Installations no Entry
    # points to yet.
    [switch]$Discover,
    [string]$Installs = '',
    [string]$LaptopConfig = '',
    # Seams for the tests: online database search, cover download and the
    # console question. Defaults: IGDB, Invoke-WebRequest, Read-Host.
    [scriptblock]$Lookup = $null,
    [scriptblock]$Download = $null,
    [scriptblock]$Ask = $null
)

$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$repoRoot = Split-Path -Parent $PSScriptRoot

# The key=value files (cyber.cfg, scrape.cfg, an Entry's juego.cfg), read
# like the launcher does: '#' and ';' start comment lines, keys and values
# are trimmed, keys are lower-cased. Returns every pair in file order (each
# caller decides whether the first or the last value of a key wins).
function Read-KeyValueFile($path) {
    $pairs = New-Object System.Collections.Generic.List[object]
    if (-not $path -or -not (Test-Path -LiteralPath $path)) { return , $pairs }
    foreach ($line in Get-Content -LiteralPath $path) {
        $t = $line.Trim()
        if (-not $t -or $t.StartsWith('#') -or $t.StartsWith(';')) { continue }
        $eq = $t.IndexOf('=')
        if ($eq -lt 1) { continue }
        $k = $t.Substring(0, $eq).Trim().ToLower()
        if (-not $k) { continue }
        $pairs.Add([pscustomobject]@{ Key = $k; Value = $t.Substring($eq + 1).Trim() })
    }
    return , $pairs
}

# The laptop configuration (cyber.cfg): first value of each key wins.
if (-not $LaptopConfig) { $LaptopConfig = Join-Path $repoRoot 'cyber.cfg' }
$laptop = @{}
foreach ($p in (Read-KeyValueFile $LaptopConfig)) {
    if (-not $laptop.ContainsKey($p.Key)) { $laptop[$p.Key] = $p.Value }
}

if (-not $Root) {
    if ($laptop['library']) { $Root = $laptop['library'].Trim('"') }
    if (-not $Root) { $Root = Join-Path $repoRoot 'games' }
}
$Root = (Resolve-Path -LiteralPath $Root -ErrorAction SilentlyContinue).Path
if (-not $Root) {
    Write-Host "No existe la carpeta de juegos. Uso: scrape.ps1 -Root <carpeta>" -ForegroundColor Red
    exit 1
}

$cfg = @{}
if (-not $Config) { $Config = Join-Path $repoRoot 'scrape.cfg' }
foreach ($p in (Read-KeyValueFile $Config)) { $cfg[$p.Key] = $p.Value }
function Cfg($name, $envName) {
    if ($cfg.ContainsKey($name) -and $cfg[$name]) { return $cfg[$name] }
    $v = [Environment]::GetEnvironmentVariable($envName)
    if ($v) { return $v }
    return ''
}


$igdbId = Cfg 'igdb_client_id' 'IGDB_CLIENT_ID'
$igdbSecret = Cfg 'igdb_client_secret' 'IGDB_CLIENT_SECRET'
$sgdbKey = Cfg 'steamgriddb_key' 'STEAMGRIDDB_KEY'

if (-not $Lookup -and (-not $igdbId -or -not $igdbSecret)) {
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

$igdbHeaders = $null
if (-not $Lookup) {
    $token = Get-IgdbToken
    $igdbHeaders = @{ 'Client-ID' = $igdbId; 'Authorization' = "Bearer $token" }
}

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

function Find-IgdbGames($clean) {
    $q = $clean.Replace('"', ' ')
    $body = "search `"$q`"; fields name,summary,first_release_date,genres.name,game_modes.name,involved_companies.company.name,involved_companies.developer,involved_companies.publisher,cover.url,platforms.id; limit 12;"
    return @(Invoke-Igdb 'games' $body)
}

$useIgdb = -not $Lookup
if ($useIgdb) { $Lookup = { param($query) Find-IgdbGames $query } }

# The database results for a name; none when the search fails.
function Find-Games($clean) {
    try { return @(& $Lookup $clean | Where-Object { $_ }) } catch { return @() }
}

function Get-GameScore($g, $nq) {
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
    return $score
}

function Select-BestGame($results, $clean) {
    $nq = Get-Norm $clean
    $best = $null
    $bestScore = -1
    foreach ($g in @($results)) {
        $score = Get-GameScore $g $nq
        if ($score -gt $bestScore) { $bestScore = $score; $best = $g }
    }
    return $best
}

function Select-IgdbGame($clean) {
    $res = Find-Games $clean
    if ($res.Count -eq 0) { return $null }
    return Select-BestGame $res $clean
}

function Download-Cover($url, $dest) {
    if (-not $url) { return $false }
    if ($url.StartsWith('//')) { $url = 'https:' + $url }
    $url = $url -replace '/t_[a-z0-9_]+/', '/t_cover_big/'
    if ($Download) { return [bool](& $Download $url $dest) }
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

# details.txt and descripcion.txt from a database match; returns the year.
function Write-GameInfo($game, $outDir) {
    $year = $null
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
    if ($rows.Count -gt 0) { Write-Text (Join-Path $outDir 'details.txt') (($rows -join "`r`n") + "`r`n") }
    if ($game.summary) { Write-Text (Join-Path $outDir 'descripcion.txt') ($game.summary + "`r`n") }
    return $year
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

function Save-Preview($name, $year, $outDir) {
    $q = $name
    if ($year) { $q = "$q $year" }
    $q = "$q PC"
    Write-Host ("    video: {0}" -f $q) -ForegroundColor DarkGray
    $null = Get-Video $q $outDir
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

# ---------------------------------------------------------------------------
# Discovery. Mirrors the launcher's resolution rules
# (src/library.c, resolve_launch_target) to find which top-level folders of
# the Install folders no Entry resolves into, and its executable scoring
# (score_exe) to pick each candidate's executable.
# ---------------------------------------------------------------------------

$sep = [IO.Path]::DirectorySeparatorChar

# Entries and cyber.cfg use Windows paths; on another OS (the tests) '\' is
# turned into the local separator.
function ConvertTo-LocalPath($p) {
    if ($sep -ne '\') { return $p.Replace('\', [string]$sep) }
    return $p
}

# Like path_is_absolute in src/util.c.
function Test-AbsolutePath($p) { return ($p -match '^[\\/]' -or $p -match '^[A-Za-z]:') }

function Join-Rel($base, $rel) { return Join-Path $base (ConvertTo-LocalPath $rel) }

function Test-Exists($p) { return [bool]($p -and (Test-Path -LiteralPath (ConvertTo-LocalPath $p))) }

function Get-LeafName($p) { return ($p -split '[\\/]')[-1] }

# Folders and files of one directory, in name order; hidden folders skipped.
function Get-VisibleDirs($dir) {
    return @(Get-ChildItem -LiteralPath $dir -Directory -Force -ErrorAction SilentlyContinue |
        Where-Object { -not ($_.Attributes -band [IO.FileAttributes]::Hidden) } | Sort-Object Name)
}
function Get-Files($dir) {
    return @(Get-ChildItem -LiteralPath $dir -File -Force -ErrorAction SilentlyContinue | Sort-Object Name)
}

# Install folders: -Installs, else installs= of the laptop configuration.
function Get-InstallFolders {
    $v = $Installs
    if (-not $v) { $v = $laptop['installs'] }
    $list = @()
    if (-not $v) { return $list }
    foreach ($part in $v.Split(';')) {
        $t = $part.Trim().Trim('"')
        if (-not $t) { continue }
        $list += $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath((ConvertTo-LocalPath $t))
    }
    return $list
}

# Mirrors score_exe in src/library.c (change both together): words (3+
# letters or digits) of the folder name found in the file name +20 each;
# installers, uninstallers, dedicated servers, crash reporters, updaters and
# config tools -60; renderer variants -30; minus one per 8 characters of the
# name.
function Get-ExeScore($fileName, $gameName) {
    $base = $fileName
    $dot = $base.LastIndexOf('.')
    if ($dot -ge 0) { $base = $base.Substring(0, $dot) }
    $base = $base.ToLowerInvariant()
    $s = 0
    foreach ($m in [regex]::Matches($gameName.ToLowerInvariant(), '[a-z0-9]+')) {
        if ($m.Value.Length -ge 3 -and $base.Contains($m.Value)) { $s += 20 }
    }
    if ($base -match 'unins|ded|setup|crash|update|config|server') { $s -= 60 }
    if ($base -match 'vulkan|glx|sdl') { $s -= 30 }
    $s -= [math]::Floor($base.Length / 8)
    return [int]$s
}

# Target of a .lnk, or $null when it can't be read (always so off Windows).
function Get-ShortcutTarget($lnk) {
    try {
        $sh = New-Object -ComObject WScript.Shell
        $t = $sh.CreateShortcut($lnk).TargetPath
        if ($t) { return $t }
    } catch { }
    return $null
}

# Files up to two folder levels inside the Install folders, Install folders
# in listed order, shallower files first (laptop_build_index).
$script:installIndex = $null
function Get-InstallIndex($installs) {
    if ($null -eq $script:installIndex) {
        $files = New-Object System.Collections.Generic.List[string]
        foreach ($inst in $installs) {
            $level = @($inst)
            for ($depth = 0; $depth -le 2; $depth++) {
                $next = @()
                foreach ($d in $level) {
                    foreach ($f in Get-Files $d) { $files.Add($f.FullName) }
                    if ($depth -lt 2) { foreach ($s in Get-VisibleDirs $d) { $next += $s.FullName } }
                }
                $level = $next
            }
        }
        $script:installIndex = $files
    }
    return , $script:installIndex
}

# resolve_cfg_value: absolute as-is; else relative to the Entry folder, then
# to the folder of the options file; the Entry-relative path if neither exists.
function Resolve-CfgValue($entryDir, $cfgDir, $val) {
    if (Test-AbsolutePath $val) { return $val }
    $p1 = Join-Rel $entryDir $val
    if (Test-Exists $p1) { return $p1 }
    if ($cfgDir -and $cfgDir -ne $entryDir) {
        $p2 = Join-Rel $cfgDir $val
        if (Test-Exists $p2) { return $p2 }
    }
    return $p1
}

# The Launch target the launcher would pick for one Entry on this PC, or
# $null. '' means a shortcut whose target can't be read: Available, but
# pointing nowhere we can tell. Mirrors resolve_launch_target in
# src/library.c rule for rule: change both together.
function Resolve-EntryTarget($entryDir, $installs) {
    $name = Split-Path -Leaf $entryDir
    $links = New-Object System.Collections.Generic.List[string]
    $cfgPath = $null
    $exes = New-Object System.Collections.Generic.List[string]
    foreach ($f in Get-Files $entryDir) {
        $e = $f.Extension.ToLowerInvariant()
        if ($e -eq '.lnk') { $links.Add($f.FullName) }
        elseif ($e -eq '.exe') { $exes.Add($f.FullName) }
        elseif ($e -eq '.cfg') { $cfgPath = $f.FullName }
    }
    if ($links.Count -eq 0 -and $exes.Count -eq 0) {
        foreach ($sub in Get-VisibleDirs $entryDir) {
            foreach ($f in Get-Files $sub.FullName) {
                $e = $f.Extension.ToLowerInvariant()
                if ($e -eq '.lnk') { $links.Add($f.FullName) }
                elseif ($e -eq '.exe') { $exes.Add($f.FullName) }
            }
        }
    }
    # The automatic executable: best score, first one on a tie.
    $autoExe = $null; $autoScore = 0
    foreach ($x in $exes) {
        $sc = Get-ExeScore (Split-Path -Leaf $x) $name
        if (-not $autoExe -or $sc -gt $autoScore) { $autoExe = $x; $autoScore = $sc }
    }

    $cfgExe = $null; $cfgExeRel = $null
    if ($cfgPath) {
        $cfgDir = Split-Path -Parent $cfgPath
        foreach ($p in (Read-KeyValueFile $cfgPath)) {
            $k = $p.Key
            $v = $p.Value
            if (-not $v) { continue }
            if ($k -ieq 'exe') {
                $cfgExe = Resolve-CfgValue $entryDir $cfgDir $v
                $cfgExeRel = $null
                if (-not (Test-AbsolutePath $v)) { $cfgExeRel = $v }
            } elseif ($k -ieq 'shortcut') {
                $links.Insert(0, (Resolve-CfgValue $entryDir $cfgDir $v))
            }
        }
    }

    $over = $laptop["exe.$name"]
    if ($over -and (Test-Exists $over)) { return $over }
    if ($cfgExeRel) {
        foreach ($i in $installs) {
            $p = Join-Rel $i $cfgExeRel
            if (Test-Exists $p) { return $p }
        }
    }
    if ($cfgExe -and (Test-Exists $cfgExe)) { return $cfgExe }
    if ($cfgExeRel) {
        $leaf = Get-LeafName $cfgExeRel
        foreach ($f in (Get-InstallIndex $installs)) {
            if ((Split-Path -Leaf $f) -ieq $leaf) { return $f }
        }
    }
    foreach ($l in $links) {
        if (-not (Test-Exists $l)) { continue }
        $t = Get-ShortcutTarget $l
        if (-not $t) { return '' }
        if (Test-Exists $t) { return $t }
    }
    if ($autoExe) { return $autoExe }
    return $null
}

# The top-level folder of an Install folder that a path lies in, or $null.
function Get-InstallTop($path, $installs) {
    if (-not $path) { return $null }
    $full = [IO.Path]::GetFullPath((ConvertTo-LocalPath $path))
    foreach ($i in $installs) {
        $base = [IO.Path]::GetFullPath($i).TrimEnd('\', '/') + $sep
        if ($full.StartsWith($base, [StringComparison]::OrdinalIgnoreCase)) {
            $rest = $full.Substring($base.Length)
            $top = ($rest -split '[\\/]')[0]
            if ($top -and $rest.Length -gt $top.Length) { return $top }
        }
    }
    return $null
}

# Top-level Install folders no Entry resolves into, first Install folder
# first. A folder name counts as covered in every Install folder: an Entry's
# relative exe= would find either copy, so a second copy is no new game.
function Get-DiscoveryCandidates($installs) {
    $covered = @{}
    foreach ($e in Get-VisibleDirs $Root) {
        $top = Get-InstallTop (Resolve-EntryTarget $e.FullName $installs) $installs
        if ($top) { $covered[$top] = $e.Name }
    }
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $seen = @{}
    $out = @()
    foreach ($inst in $installs) {
        foreach ($d in Get-VisibleDirs $inst) {
            if ($covered.ContainsKey($d.Name) -or $seen.ContainsKey($d.Name)) { continue }
            if ([string]::Equals($d.FullName.TrimEnd('\', '/'), $rootFull, [StringComparison]::OrdinalIgnoreCase)) { continue }
            $seen[$d.Name] = $true
            $out += [pscustomobject]@{ Folder = $d.Name; Dir = $d.FullName; Install = $inst }
        }
    }
    return $out
}

function Invoke-Discovery {
    $installs = @(Get-InstallFolders)
    if ($installs.Count -eq 0) {
        Write-Host "Faltan las carpetas de instalacion: usa -Installs 'C:\Juegos;D:\Games' o installs= en cyber.cfg" -ForegroundColor Red
        exit 1
    }
    Write-Host "Biblioteca: $Root"
    Write-Host "Carpetas de instalacion: $($installs -join '; ')"
    foreach ($c in Get-DiscoveryCandidates $installs) {
        $plan = New-DiscoveryPlan $c
        if ($plan.Status -eq 'planned' -and -not $DryRun) { Write-DiscoveredEntry $plan $c }
        Show-DiscoveryPlan $plan
        $plan
    }
}

# Executables of a candidate, best first: its own files, else one folder
# level below, else two. Rel is the exe= value (relative to the Install folder).
function Get-CandidateExes($c) {
    $level = @($c.Dir)
    for ($depth = 0; $depth -le 2; $depth++) {
        $found = @()
        $next = @()
        foreach ($d in $level) {
            foreach ($f in Get-Files $d) {
                if ($f.Extension -ine '.exe') { continue }
                $rel = $c.Folder + $f.FullName.Substring($c.Dir.Length).Replace([string]$sep, '\')
                $found += [pscustomobject]@{ Rel = $rel; Score = (Get-ExeScore $f.Name $c.Folder); Order = $found.Count }
            }
            foreach ($s in Get-VisibleDirs $d) { $next += $s.FullName }
        }
        if ($found.Count -gt 0) {
            return @($found | Sort-Object @{ Expression = 'Score'; Descending = $true }, @{ Expression = 'Order' })
        }
        $level = $next
    }
    return @()
}

# The launcher's choice is sure when there is one executable, when the best
# one is named like the folder and beats the rest, or when it is the only one
# without a penalty (installer, uninstaller, server...).
function Test-ExeChoiceSure($exes) {
    if ($exes.Count -le 1) { return $true }
    if ($exes[0].Score -ge 20 -and $exes[0].Score -gt $exes[1].Score) { return $true }
    return (@($exes | Where-Object { $_.Score -gt -30 }).Count -eq 1)
}

# The database match is sure when exactly one result has the folder's name.
function Get-SureMatch($results, $clean) {
    $nq = Get-Norm $clean
    $exact = @($results | Where-Object { (Get-Norm $_.name) -eq $nq })
    if ($exact.Count -eq 1) { return $exact[0] }
    return $null
}

function New-DiscoveryPlan($c) {
    $plan = [pscustomobject]@{
        Folder = $c.Folder; Install = $c.Install; Entry = (Join-Path $Root $c.Folder)
        Exe = $null; Args = $null; Name = $c.Folder; Game = $null
        Questions = @(); Status = 'planned'
    }
    if ((Test-Path -LiteralPath $plan.Entry) -and -not $Force) { $plan.Status = 'exists'; return $plan }

    $exes = Get-CandidateExes $c
    if ($exes.Count -eq 0) { $plan.Status = 'noexe'; return $plan }
    $plan.Exe = $exes[0].Rel

    $clean = Get-CleanName $c.Folder
    $results = Find-Games $clean
    if ($useIgdb) { Start-Sleep -Milliseconds 280 }
    $plan.Game = Get-SureMatch $results $clean
    if (-not $plan.Game) {
        $plan.Questions += 'name'
        if ($DryRun) {
            if ($results.Count -gt 0) { $plan.Game = Select-BestGame $results $clean }
        } else {
            Resolve-GameQuestion $plan $results
        }
    }
    if ($plan.Game) { $plan.Name = $plan.Game.name }

    if (-not (Test-ExeChoiceSure $exes)) {
        $plan.Questions += 'exe', 'args'
        if (-not $DryRun) { Resolve-ExeQuestions $plan $exes }
    }
    return $plan
}

function Get-GameLabel($g) {
    $l = $g.name
    if ($g.first_release_date) { $l += " (" + [DateTimeOffset]::FromUnixTimeSeconds($g.first_release_date).Year + ")" }
    return $l
}

# Which game a candidate is: a number picks a result, a typed name searches
# again, Enter leaves the Entry without database details.
function Resolve-GameQuestion($plan, $results) {
    $nq = Get-Norm (Get-CleanName $plan.Folder)
    # Sort-Object is not stable in Windows PowerShell 5.1: break ties by the
    # database's own order so the options don't shuffle between runs.
    $i = 0
    $ranked = @($results | ForEach-Object { [pscustomobject]@{ R = $_; Score = (Get-GameScore $_ $nq); Order = $i }; $i++ })
    $opts = @($ranked | Sort-Object @{ Expression = 'Score'; Descending = $true }, @{ Expression = 'Order' } |
              Select-Object -First 9 | ForEach-Object { $_.R })
    $labels = @($opts | ForEach-Object { Get-GameLabel $_ })
    $a = [string](& $Ask 'name' ("Que juego es '{0}'? Numero, otro nombre para buscar o Enter para dejarlo sin datos" -f $plan.Folder) $labels)
    $a = $a.Trim()
    $n = 0
    if (-not $a) { return }
    if ([int]::TryParse($a, [ref]$n) -and $n -ge 1 -and $n -le $opts.Count) { $plan.Game = $opts[$n - 1]; return }
    $again = Find-Games $a
    $plan.Game = Get-SureMatch $again $a
    if (-not $plan.Game -and $again.Count -gt 0) { $plan.Game = Select-BestGame $again $a }
    if (-not $plan.Game) { $plan.Name = $a }
}

# Which executable (number, a path relative to the Install folder, or Enter
# for the best guess) and its launch arguments (Enter for none).
function Resolve-ExeQuestions($plan, $exes) {
    $labels = @($exes | ForEach-Object { $_.Rel })
    $a = ([string](& $Ask 'exe' ("Que ejecutable lanza '{0}'? Numero o ruta; Enter = {1}" -f $plan.Name, $plan.Exe) $labels)).Trim()
    $n = 0
    if ([int]::TryParse($a, [ref]$n) -and $n -ge 1 -and $n -le $exes.Count) { $plan.Exe = $exes[$n - 1].Rel }
    elseif ($a) { $plan.Exe = $a }
    $a = ([string](& $Ask 'args' ("Argumentos para {0}? (Enter = ninguno)" -f $plan.Exe) @())).Trim()
    if ($a) { $plan.Args = $a }
}

if (-not $Ask) {
    $Ask = {
        param($kind, $prompt, $options)
        Write-Host $prompt -ForegroundColor Cyan
        for ($i = 0; $i -lt $options.Count; $i++) { Write-Host ("  {0}) {1}" -f ($i + 1), $options[$i]) }
        return Read-Host '>'
    }
}

function Write-DiscoveredEntry($plan, $c) {
    $dir = $plan.Entry
    $null = New-Item -ItemType Directory -Force -Path $dir
    $lines = @("exe=$($plan.Exe)")
    if ($plan.Args) { $lines += "args=$($plan.Args)" }
    if ($plan.Name -cne $plan.Folder) { $lines += "nombre=$($plan.Name)" }
    Write-Text (Join-Path $dir 'juego.cfg') (($lines -join "`r`n") + "`r`n")

    $game = $plan.Game
    $ico = Find-GameIco $c.Dir
    $gotCover = $false
    if ($ico) { Copy-GameIco $ico (Join-Path $dir 'cover.ico'); $gotCover = $true }
    if (-not $gotCover -and $game -and $game.cover) { $gotCover = Download-Cover $game.cover.url (Join-Path $dir 'cover.png') }
    if (-not $gotCover -and -not $Download) { $gotCover = Download-SgdbCover $plan.Name (Join-Path $dir 'cover.png') }

    $year = $null
    if ($game) { $year = Write-GameInfo $game $dir }
    if ($Video) { Save-Preview $plan.Name $year $dir }
    $plan.Status = 'written'
}

function Show-DiscoveryPlan($p) {
    $line = "  {0}" -f $p.Folder
    switch ($p.Status) {
        'exists' { Write-Host "$line  -> ya hay una entrada con ese nombre (usa -Force para rehacerla)" -ForegroundColor Yellow; return }
        'noexe'  { Write-Host "$line  -> sin ejecutable, se omite" -ForegroundColor Yellow; return }
    }
    $what = "exe=$($p.Exe)"
    if ($p.Args) { $what += " args=$($p.Args)" }
    $db = if ($p.Game) { $p.Game.name } else { 'sin datos' }
    $verb = if ($p.Status -eq 'written') { 'creada' } else { 'se crearia' }
    $color = if ($p.Status -eq 'written') { 'Green' } else { 'Cyan' }
    Write-Host ("{0}  -> {1}: games\{2}  {3}  [{4}]" -f $line, $verb, $p.Folder, $what, $db) -ForegroundColor $color
    if ($DryRun -and $p.Questions.Count -gt 0) {
        Write-Host ("      preguntaria: {0}" -f ($p.Questions -join ', ')) -ForegroundColor DarkGray
    }
}

if ($Discover) {
    Invoke-Discovery
    return
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
    if (-not $CoverOnly) { $year = Write-GameInfo $game $outDir }

    if ($Video -and -not $CoverOnly) {
        $mpg = Join-Path $outDir 'preview.mpg'
        if ($Force -or -not (Test-Path -LiteralPath $mpg)) { Save-Preview $game.name $year $outDir }
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
