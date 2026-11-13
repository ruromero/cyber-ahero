# Discovery mode of tools/scrape.ps1, tested from the outside: the script is
# run over a temp tree of fake Installations and a fake Master Library, with
# the online database, the cover download and the console prompt injected.

BeforeAll {
    $script:Scrape = Join-Path (Split-Path -Parent $PSScriptRoot) 'scrape.ps1'

    # Creates the given files (relative to $base), each with dummy content.
    function New-Tree($base, [string[]]$files) {
        foreach ($f in $files) {
            $p = Join-Path $base ($f -replace '[\\/]', [IO.Path]::DirectorySeparatorChar)
            $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $p)
            Set-Content -LiteralPath $p -Value 'x' -NoNewline
        }
    }

    function New-Entry($library, $name, $cfg) {
        $dir = Join-Path $library $name
        $null = New-Item -ItemType Directory -Force -Path $dir
        Set-Content -LiteralPath (Join-Path $dir 'juego.cfg') -Value $cfg -NoNewline
        return $dir
    }

    # An online database that knows the given games by exact name.
    function New-FakeDb([string[]]$names) {
        $games = foreach ($n in $names) {
            [pscustomobject]@{
                name = $n
                summary = "Sinopsis de $n"
                first_release_date = 946684800   # 2000-01-01
                genres = @([pscustomobject]@{ name = 'Shooter' })
                game_modes = @([pscustomobject]@{ name = 'Multiplayer' })
                involved_companies = @()
                platforms = @([pscustomobject]@{ id = 6 })
                cover = [pscustomobject]@{ url = "//img/$n.jpg" }
            }
        }
        $games = @($games)
        return { param($query) $games | Where-Object { $_.name -like "*$query*" } }.GetNewClosure()
    }

    $script:FakeDownload = { param($url, $dest) Set-Content -LiteralPath $dest -Value $url -NoNewline; $true }
    # A console that answers by question kind and records what was asked.
    function New-Asker($answers) {
        $log = @{ Kinds = New-Object System.Collections.Generic.List[string]; Options = @{} }
        $log.Ask = {
            param($kind, $prompt, $options)
            $log.Kinds.Add($kind)
            $log.Options[$kind] = @($options)
            return $answers[$kind]
        }.GetNewClosure()
        return $log
    }

    $script:NoQuestions = { param($kind, $prompt, $options) throw "unexpected question: $kind" }

    function Invoke-Discovery {
        param($Library, $Installs, $Lookup, $Ask = $script:NoQuestions, [switch]$DryRun, [switch]$Force,
              $LaptopConfig = (Join-Path $TestDrive 'none.cfg'))
        $a = @{
            Discover = $true; Root = $Library; Installs = ($Installs -join ';')
            Lookup = $Lookup; Download = $script:FakeDownload; Ask = $Ask
            LaptopConfig = $LaptopConfig; DryRun = $DryRun; Force = $Force
        }
        return @(& $script:Scrape @a 6>$null)
    }

    function Get-TreeSnapshot($dir) {
        # Folders are compared by existence only: NTFS can report a folder's
        # LastWriteTime late, after files were created inside it earlier.
        return @(Get-ChildItem -LiteralPath $dir -Recurse -Force | ForEach-Object {
            if ($_.PSIsContainer) { "$($_.FullName)|<dir>"; return }
            $c = Get-Content -LiteralPath $_.FullName -Raw
            "$($_.FullName)|$c|$($_.LastWriteTimeUtc.Ticks)"
        } | Sort-Object)
    }
}

Describe 'Discovery candidates' {
    BeforeEach {
        $lib = Join-Path $TestDrive 'games'
        $c = Join-Path $TestDrive 'C_Juegos'
        $d = Join-Path $TestDrive 'D_Games'
        $null = New-Item -ItemType Directory -Force -Path $lib
        New-Tree $c @('Quake3\quake3.exe', 'Unreal\System\Unreal.exe', 'Worms\worms.exe')
        New-Tree $d @('Half-Life\hl.exe', 'Age of Empires\empires.exe')
        # Covered by a relative exe= in the first Install folder.
        $null = New-Entry $lib 'Quake III' "exe=Quake3\quake3.exe`r`n"
        # Covered through the file-name search (folder named differently).
        $null = New-Entry $lib 'Half-Life' "exe=HL\hl.exe`r`n"
    }

    It 'proposes exactly the top-level Install folders no Entry resolves into' {
        $plan = Invoke-Discovery -Library $lib -Installs $c, $d -Lookup (New-FakeDb @()) -DryRun
        @($plan | ForEach-Object { $_.Folder } | Sort-Object) | Should -Be @('Age of Empires', 'Unreal', 'Worms')
    }
}

Describe 'Writing an Entry' {
    BeforeEach {
        $lib = Join-Path $TestDrive 'games'
        $inst = Join-Path $TestDrive 'Juegos'
        $null = New-Item -ItemType Directory -Force -Path $lib
    }

    It 'writes a complete Entry for an unambiguous game without asking anything' {
        New-Tree $inst @('Worms Armageddon\WA.exe', 'Worms Armageddon\Setup.exe', 'Worms Armageddon\unins000.exe',
                         'Worms Armageddon\Tools\WormsArmageddonConfig.exe')
        $plan = Invoke-Discovery -Library $lib -Installs $inst -Lookup (New-FakeDb @('Worms Armageddon', 'Worms 2'))

        $entry = Join-Path $lib 'Worms Armageddon'
        Get-Content -LiteralPath (Join-Path $entry 'juego.cfg') | Should -Be @('exe=Worms Armageddon\WA.exe')
        Get-Content -LiteralPath (Join-Path $entry 'cover.png') -Raw | Should -Be 'https://img/Worms Armageddon.jpg'
        Get-Content -LiteralPath (Join-Path $entry 'details.txt') | Should -Contain 'Jugadores: Multiplayer'
        Get-Content -LiteralPath (Join-Path $entry 'descripcion.txt') | Should -Be @('Sinopsis de Worms Armageddon')
        $plan[0].Status | Should -Be 'written'
    }

    It 'asks only which executable and its arguments when the executable is ambiguous' {
        New-Tree $inst @('Unreal\System\UT.exe', 'Unreal\System\Editor.exe', 'Unreal\System\UCC.exe')
        $log = New-Asker @{ exe = '2'; args = '-nohomedir' }
        $null = Invoke-Discovery -Library $lib -Installs $inst -Lookup (New-FakeDb @('Unreal')) -Ask $log.Ask

        $log.Kinds | Should -Be @('exe', 'args')
        $log.Options['exe'] | Should -Contain 'Unreal\System\UCC.exe'
        $chosen = $log.Options['exe'][1]
        Get-Content -LiteralPath (Join-Path $lib 'Unreal\juego.cfg') | Should -Be @("exe=$chosen", 'args=-nohomedir')
    }

    It 'asks only for the game when the database match is ambiguous' {
        New-Tree $inst @('Doom\doom.exe')
        $two = {
            param($query)
            foreach ($y in 725846400, 1463097600) {   # 1993, 2016
                [pscustomobject]@{ name = 'Doom'; summary = "Doom $y"; first_release_date = $y
                                   platforms = @([pscustomobject]@{ id = 6 }); cover = $null }
            }
        }
        $log = New-Asker @{ name = '2' }
        $null = Invoke-Discovery -Library $lib -Installs $inst -Lookup $two -Ask $log.Ask

        $log.Kinds | Should -Be @('name')
        $log.Options['name'] | Should -Be @('Doom (1993)', 'Doom (2016)')
        Get-Content -LiteralPath (Join-Path $lib 'Doom\juego.cfg') | Should -Be @('exe=Doom\doom.exe')
        Get-Content -LiteralPath (Join-Path $lib 'Doom\descripcion.txt') | Should -Be @('Doom 1463097600')
    }

    It 'searches again with a name typed by the Organizer' {
        New-Tree $inst @('Q3\quake3.exe')
        $ask = { param($kind, $prompt, $options) 'Quake III Arena' }
        $null = Invoke-Discovery -Library $lib -Installs $inst -Lookup (New-FakeDb @('Quake III Arena')) -Ask $ask

        Get-Content -LiteralPath (Join-Path $lib 'Q3\juego.cfg') | Should -Be @('exe=Q3\quake3.exe', 'nombre=Quake III Arena')
    }
}

Describe 'Dry run and re-runs' {
    BeforeEach {
        $lib = Join-Path $TestDrive 'games'
        $inst = Join-Path $TestDrive 'Juegos'
        $null = New-Item -ItemType Directory -Force -Path $lib
        New-Tree $inst @('Worms\worms.exe', 'Doom\doom.exe', 'Unreal\System\UT.exe', 'Unreal\System\UCC.exe')
        $null = New-Entry $lib 'Mi Doom' "exe=Doom\doom.exe`r`n"
        $db = New-FakeDb @('Worms', 'Doom')
    }

    It 'writes nothing and asks nothing in a dry run, but prints the planned Entries' {
        $before = Get-TreeSnapshot $TestDrive
        $plan = Invoke-Discovery -Library $lib -Installs $inst -Lookup $db -DryRun
        Get-TreeSnapshot $TestDrive | Should -Be $before
        @($plan | ForEach-Object { $_.Folder }) | Should -Be @('Unreal', 'Worms')
        ($plan | Where-Object Folder -eq 'Worms').Exe | Should -Be 'Worms\worms.exe'
        ($plan | Where-Object Folder -eq 'Unreal').Questions | Should -Be @('name', 'exe', 'args')
    }

    It 'creates no duplicates and touches no Entry when run twice' {
        $ask = (New-Asker @{ name = ''; exe = ''; args = '' }).Ask
        $null = Invoke-Discovery -Library $lib -Installs $inst -Lookup $db -Ask $ask
        $after = Get-TreeSnapshot $lib
        $second = Invoke-Discovery -Library $lib -Installs $inst -Lookup $db
        $second.Count | Should -Be 0
        Get-TreeSnapshot $lib | Should -Be $after
        @(Get-ChildItem -LiteralPath $lib -Directory | ForEach-Object Name | Sort-Object) | Should -Be @('Mi Doom', 'Unreal', 'Worms')
    }

    It 'leaves an Entry folder with the same name alone unless -Force is given' {
        $old = New-Entry $lib 'Worms' "exe=Otro\worms-viejo.exe`r`n"   # resolves nowhere
        $plan = Invoke-Discovery -Library $lib -Installs $inst -Lookup $db -Ask (New-Asker @{}).Ask
        ($plan | Where-Object Folder -eq 'Worms').Status | Should -Be 'exists'
        Get-Content -LiteralPath (Join-Path $old 'juego.cfg') | Should -Be @('exe=Otro\worms-viejo.exe')

        $null = Invoke-Discovery -Library $lib -Installs $inst -Lookup $db -Ask (New-Asker @{}).Ask -Force
        Get-Content -LiteralPath (Join-Path $old 'juego.cfg') | Should -Be @('exe=Worms\worms.exe')
    }

    It 'writes Entries that resolve on another laptop with the game under the same folder name' {
        $null = Invoke-Discovery -Library $lib -Installs $inst -Lookup $db -Ask (New-Asker @{}).Ask
        $other = Join-Path $TestDrive 'OtroPortatil'
        New-Tree $other @('Worms\worms.exe', 'Unreal\System\UT.exe', 'Unreal\System\UCC.exe')
        # Under the launcher's rules every Installation there is reached by an Entry.
        $plan = Invoke-Discovery -Library $lib -Installs $other -Lookup $db -DryRun
        $plan.Count | Should -Be 0
    }
}

Describe 'Install folders and the laptop configuration' {
    BeforeEach {
        $lib = Join-Path $TestDrive 'games'
        $inst = Join-Path $TestDrive 'Juegos'
        $null = New-Item -ItemType Directory -Force -Path $lib
        New-Tree $inst @('Worms\worms.exe', 'Quake3\quake3.exe')
        $db = New-FakeDb @('Worms', 'Quake3')
    }

    It 'reads installs= from the laptop configuration and honours its per-laptop override' {
        $null = New-Entry $lib 'Quake III' "exe=C:\no\existe\quake3.exe`r`n"
        $cfg = Join-Path $TestDrive 'cyber.cfg'
        Set-Content -LiteralPath $cfg -Value "installs=$inst`r`nexe.Quake III=$(Join-Path $inst 'Quake3/quake3.exe')`r`n"
        $plan = Invoke-Discovery -Library $lib -Installs @() -Lookup $db -DryRun -LaptopConfig $cfg
        @($plan | ForEach-Object Folder) | Should -Be @('Worms')
    }

    It 'chooses the executable named like the folder over installers and servers' {
        New-Tree $inst @('Half-Life\hl.exe', 'Half-Life\hlds.exe', 'Half-Life\Half-Life Setup.exe', 'Half-Life\half-life.exe')
        $plan = Invoke-Discovery -Library $lib -Installs $inst -Lookup (New-FakeDb @('Half-Life')) -DryRun
        ($plan | Where-Object Folder -eq 'Half-Life').Exe | Should -Be 'Half-Life\half-life.exe'
        ($plan | Where-Object Folder -eq 'Half-Life').Questions.Count | Should -Be 0
    }
}
