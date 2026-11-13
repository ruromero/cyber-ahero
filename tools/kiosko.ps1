# =========================================================================
# kiosko.ps1 - Modo kiosko + juego sin dejar la app abierta en segundo plano
# -------------------------------------------------------------------------
#  Que hace:
#   1. Aplica las restricciones de kiosko (tecla Win, Win+R, TaskMgr, Inicio,
#      Ctrl+Alt+Supr).
#   2. Arranca el bloqueador de teclado (kbdblock.exe) junto a la app: se
#      traga TODAS las teclas mientras el menu del ciber esta en pantalla.
#      Cuando se lanza un juego, el bloqueador se apaga para que el jugador
#      pueda usar el teclado con normalidad.
#   3. Resuelve los destinos de los accesos directos de los juegos (game.lnk)
#      para saber que ejecutables "son juego".
#   4. Arranca cyber_ahero.exe a pantalla completa (lo primero que hace).
#   5. Mantiene explorer.exe muerto (sin barra de tareas, sin menu Inicio, y
#      sin nadie que atienda la tecla Win) de forma continua.
#   6. Cuando el usuario lanza un juego desde la app, detecta el proceso del
#      juego y CIERRA la app del ciber para que no siga consumiendo CPU.
#   7. Espera a que el juego (y sus procesos) termine.
#   8. Al cerrarse el juego, vuelve a abrir la app del ciber (modo kiosko).
#
#  NOTA: ShellExecute lanza el juego a traves de un proceso intermediario, por
#  eso NO se usa la relacion padre-hijo sino la ruta del ejecutable del juego.
#
#  Para revertir todo: ejecutar limpiador.ps1
# =========================================================================

param()

$ErrorActionPreference = "SilentlyContinue"

# ---- Sin auto-elevacion: el script se ejecuta tal cual y abre la app ----

# ---- Rutas --------------------------------------------------------------
# La app del kiosko se busca primero en la instalacion fija (C:\kiosko) y si
# no, junto al script (para poder probarlo fuera del kiosko). El build genera
# cyber_ahero.exe; ciber_ahero.exe se acepta por compatibilidad con
# instalaciones antiguas.
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$appCandidates = @()
foreach ($d in @("C:\kiosko\cyber_ahero\cyber_ahero\x86", (Split-Path -Parent $scriptDir), $scriptDir)) {
    foreach ($n in @("cyber_ahero.exe", "ciber_ahero.exe")) {
        $appCandidates += (Join-Path $d $n)
    }
}
$exePath = $appCandidates[0]
foreach ($c in $appCandidates) {
    if ($c -and (Test-Path -LiteralPath $c)) { $exePath = $c; break }
}
$appDir         = Split-Path -Parent $exePath
$gamesDir       = Join-Path $appDir "games"
$gamesDirLower  = $gamesDir.ToLower()
$appDirLower    = $appDir.ToLower()
$logPath        = Join-Path $appDir "kiosko.log"

# Bloqueador de teclado (kbdblock.exe): junto a la app o junto al script
$blockerExe = ""
foreach ($c in @((Join-Path $appDir "kbdblock.exe"), (Join-Path $scriptDir "kbdblock.exe"))) {
    if (Test-Path -LiteralPath $c) { $blockerExe = $c; break }
}

# ---- Registro: restricciones de kiosko ----------------------------------
# secKey/expKey = policies del usuario actual (HKCU)
# mSecKey/mExpKey = policies de la maquina (HKLM, solo si hay permisos)
$secKey  = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Policies\System"
$expKey  = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer"
$mSecKey = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Policies\System"
$mExpKey = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer"
$advKey  = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced"

foreach ($k in @($secKey, $expKey)) {
    if (-not (Test-Path $k)) { New-Item -Path $k -Force | Out-Null }
}
if (-not (Test-Path $advKey)) { New-Item -Path $advKey -Force | Out-Null }

# DisableTaskMgr = bloquea el Administrador de tareas
# NoRun         = bloquea Ejecutar (Win+R)
# NoWinKeys     = desactiva la tecla Windows y sus atajos
Set-ItemProperty -Path $secKey -Name "DisableTaskMgr" -Value 1 -Type DWord
Set-ItemProperty -Path $expKey -Name "NoRun"          -Value 1 -Type DWord
Set-ItemProperty -Path $expKey -Name "NoWinKeys"      -Value 1 -Type DWord

# Quita el boton Inicio del shell (value "Start" = "NoStartMenu")
Set-ItemProperty -Path $advKey -Name "Start" -Value "NoStartMenu" -Type String

# Las mismas politicas a nivel maquina, si el script corre como administrador
try {
    foreach ($k in @($mSecKey, $mExpKey)) {
        if (-not (Test-Path $k)) { New-Item -Path $k -Force | Out-Null }
    }
    Set-ItemProperty -Path $mSecKey -Name "DisableTaskMgr" -Value 1 -Type DWord
    Set-ItemProperty -Path $mExpKey -Name "NoRun"          -Value 1 -Type DWord
    Set-ItemProperty -Path $mExpKey -Name "NoWinKeys"      -Value 1 -Type DWord
} catch { }

# ---- Winlogon: que Windows NO vuelva a arrancar explorer.exe -----------
# Sin esto, Windows reinicia el shell solo cada vez que lo matamos, y el
# escritorio (barra de tareas, menu Inicio, tecla Win) reaparece.
$wlKey   = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
$wlKeyCU = "HKCU:\Software\Microsoft\Windows NT\CurrentVersion\Winlogon"
$shellLocked = $false
$cadLocked   = $false
try {
    if (-not (Test-Path $wlKey)) { New-Item -Path $wlKey -Force | Out-Null }
    Set-ItemProperty -Path $wlKey -Name "AutoRestartShell" -Value 0 -Type DWord
    $shellLocked = ((Get-ItemProperty -Path $wlKey -Name "AutoRestartShell" -ErrorAction SilentlyContinue).AutoRestartShell -eq 0)
} catch { }
try {
    if (-not (Test-Path $wlKeyCU)) { New-Item -Path $wlKeyCU -Force | Out-Null }
    Set-ItemProperty -Path $wlKeyCU -Name "AutoRestartShell" -Value 0 -Type DWord
} catch { }
# DisableCAD = bloquea por completo Ctrl+Alt+Supr (la pantalla de seguridad de
# Windows es la unica via que queda para llegar al Administrador de tareas o
# cambiar de usuario). Es de nivel maquina: hace falta administrador.
try {
    Set-ItemProperty -Path $wlKey -Name "DisableCAD" -Value 1 -Type DWord
    $cadLocked = ((Get-ItemProperty -Path $wlKey -Name "DisableCAD" -ErrorAction SilentlyContinue).DisableCAD -eq 1)
} catch { }

# ---- Autoarranque: que el kiosko arranque solo al iniciar sesion ---------
$runKey  = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
$runName = "CiberAhKiosko"
$autoStart = $false
try {
    if (-not (Test-Path $runKey)) { New-Item -Path $runKey -Force | Out-Null }
    $runCmd = "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"" + $MyInvocation.MyCommand.Path + "`""
    Set-ItemProperty -Path $runKey -Name $runName -Value $runCmd -Type String
    $autoStart = $true
} catch { }

# ---- Funciones auxiliares -----------------------------------------------
function Write-Log($msg) {
    "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  $msg" | Out-File -FilePath $logPath -Append -Encoding ASCII
}

# Mantiene explorer.exe muerto. Sin shell no hay barra de tareas, ni menu
# Inicio, ni nada que atienda la tecla Windows.
$script:lastExplorerKill = [DateTime]::MinValue
function Stop-Explorer {
    if (((Get-Date) - $script:lastExplorerKill).TotalSeconds -lt 3) { return }
    $script:lastExplorerKill = Get-Date
    Stop-Process -Name "explorer" -Force -ErrorAction SilentlyContinue
}

# --- Bloqueador de teclado -----------------------------------------------
# Mientras esta el menu del ciber, kbdblock.exe se traga todas las teclas
# (hook global WH_KEYBOARD_LL). Se para en cuanto hay un juego en marcha.
function Start-KeyboardBlock {
    if (-not (Test-Path -LiteralPath $blockerExe)) { return $false }
    Stop-KeyboardBlock
    Start-Process -FilePath $blockerExe -WorkingDirectory $appDir -WindowStyle Hidden -ErrorAction SilentlyContinue | Out-Null
    return $true
}
function Stop-KeyboardBlock {
    Stop-Process -Name "kbdblock" -Force -ErrorAction SilentlyContinue
}

# Destinos (.exe) de todos los game.lnk de la biblioteca
$targetSet   = @{}   # rutas completas (en minusculas)
$targetNames = @{}   # nombres de ejecutable (en minusculas)
try {
    $shell = New-Object -ComObject WScript.Shell
    if (Test-Path -LiteralPath $gamesDir) {
        Get-ChildItem -LiteralPath $gamesDir -Recurse -Filter *.lnk -ErrorAction SilentlyContinue | ForEach-Object {
            $tp = $shell.CreateShortcut($_.FullName).TargetPath
            if ($tp) {
                $targetSet[$tp.ToLower()] = $true
                $targetNames[[System.IO.Path]::GetFileName($tp).ToLower()] = $true
            }
        }
    }
} catch { }

function Test-IsGameProcess($exePath) {
    if (-not $exePath) { return $false }
    $lp = $exePath.ToLower()
    # Un juego puede estar dentro de la propia biblioteca (games\)
    if ($lp.StartsWith($gamesDirLower + "\")) { return $true }
    # Pero lo demas que vive en la carpeta de la app NO es un juego
    # (cyber_ahero.exe, kbdblock.exe, el powershell del kiosko...)
    if ($lp.StartsWith($appDirLower + "\")) { return $false }
    # Destinos de los accesos directos de la biblioteca
    if ($targetSet.ContainsKey($lp)) { return $true }
    if ($targetNames.ContainsKey([System.IO.Path]::GetFileName($lp))) { return $true }
    # Respaldo: cualquier ejecutable de usuario (fuera de Windows y de
    # Archivos de programa) se considera juego. Asi tambien se detectan los
    # juegos que lanzan otro proceso (emuladores, lanzadores, wrappers...).
    if (-not $lp.StartsWith("c:\windows\") -and
        -not $lp.StartsWith("c:\program files\") -and
        -not $lp.StartsWith("c:\program files (x86)\")) { return $true }
    return $false
}

# Busca un proceso de juego lanzado DESPUES de arrancar la app
function Find-GameProcess([int]$appPid, $baseline) {
    foreach ($pr in Get-WmiObject Win32_Process) {
        if ($pr.ProcessId -eq $appPid) { continue }
        if ($baseline -contains $pr.ProcessId) { continue }
        if (Test-IsGameProcess $pr.ExecutablePath) { return $pr }
    }
    return $null
}

# Espera a que el juego (por PID y por ruta del ejecutable) termine
function Wait-GameEnd($game) {
    $pidRoot = $game.ProcessId
    $path = ""
    if ($game.ExecutablePath) { $path = $game.ExecutablePath.ToLower() }
    while ($true) {
        Stop-Explorer
        $still = $false
        foreach ($pr in Get-WmiObject Win32_Process) {
            if ($pr.ProcessId -eq $pidRoot) { $still = $true; break }
            if ($path -and $pr.ExecutablePath -and ($pr.ExecutablePath.ToLower() -eq $path)) { $still = $true; break }
        }
        if (-not $still) { return }
        Start-Sleep -Milliseconds 1000
    }
}

# ---- Inicio -------------------------------------------------------------
Remove-Item -LiteralPath $logPath -Force
Write-Log "Kiosko iniciado. App: $exePath"
Write-Log ("Destinos de juegos resueltos: " + $targetSet.Count)
if ($shellLocked) {
    Write-Log "AutoRestartShell=0: explorer.exe NO se reiniciara solo"
} else {
    Write-Log "AVISO: no se pudo aplicar AutoRestartShell (hace falta administrador); Windows reiniciara explorer.exe"
}
if ($cadLocked) {
    Write-Log "DisableCAD=1: Ctrl+Alt+Supr bloqueado (no habra pantalla de seguridad)"
} else {
    Write-Log "AVISO: no se pudo aplicar DisableCAD (hace falta administrador); Ctrl+Alt+Supr seguira mostrando la pantalla de seguridad"
}
if ($autoStart) {
    Write-Log "Autoarranque activado: el kiosko arrancara solo en cada inicio de sesion"
} else {
    Write-Log "AVISO: no se pudo activar el autoarranque (hace falta administrador)"
}
if (Test-Path -LiteralPath $blockerExe) {
    Write-Log "Bloqueador de teclado disponible: $blockerExe"
} else {
    Write-Log "AVISO: no se encuentra kbdblock.exe; el teclado NO quedara bloqueado en el menu"
}

while ($true) {
    # Sin shell: ni barra de tareas, ni menu Inicio, ni tecla Win
    Stop-Explorer

    # Foto de procesos actuales: lo que ya estaba no se considera "juego nuevo"
    $baseline = @(Get-WmiObject Win32_Process | ForEach-Object { [int]$_.ProcessId })

    $app = Start-Process -FilePath $exePath -WorkingDirectory $appDir -PassThru
    Write-Log "App del ciber abierta (PID $($app.Id))"

    # Teclado bloqueado mientras el menu esta en pantalla
    if (Start-KeyboardBlock) {
        Write-Log "Teclado BLOQUEADO (ninguna tecla llega a ninguna ventana)"
    }

    # Espera a que se lance un juego o a que la app se cierre
    $game = $null
    while (-not $app.HasExited) {
        Stop-Explorer
        $game = Find-GameProcess $app.Id $baseline
        if ($game) { break }
        Start-Sleep -Milliseconds 200
    }

    if ($app.HasExited -and -not $game) {
        Write-Log "La app del ciber se cerro por si sola; se vuelve a abrir"
        Stop-KeyboardBlock
        Start-Sleep -Milliseconds 1000
        continue
    }

    # Se lanzo un juego: devolver el teclado al jugador y cerrar la app del
    # ciber para liberar CPU (evita el ~50% de carga)
    Stop-KeyboardBlock
    Write-Log ("Juego detectado: " + $game.Name + " (PID " + $game.ProcessId + "). Teclado devuelto al jugador. Cerrando la app del ciber (PID " + $app.Id + ")")
    Stop-Process -Id $app.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 300

    # Esperar a que termine el juego
    Wait-GameEnd $game
    Write-Log "Juego finalizado. Volviendo al menu del ciber"

    Start-Sleep -Milliseconds 1500
}
