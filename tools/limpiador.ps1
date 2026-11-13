# =========================================================================
# limpiador.ps1 - Reversión de restricciones y cierre de sesión
# -------------------------------------------------------------------------
#  Revierte TODO lo que aplica kiosko.ps1:
#   - DisableTaskMgr, NoRun, NoWinKeys (HKCU y HKLM)
#   - El boton Inicio eliminado (Explorer\Advanced -> "Start")
#   - Vuelve a arrancar explorer.exe y reinicia la sesion
# =========================================================================

$secKey  = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Policies\System"
$expKey  = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer"
$mSecKey = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Policies\System"
$mExpKey = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer"
$advKey  = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced"
$wlKey   = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
$wlKeyCU = "HKCU:\Software\Microsoft\Windows NT\CurrentVersion\Winlogon"

# 1. Eliminar restricción del Administrador de Tareas (System)
if (Test-Path $secKey) {
    Remove-ItemProperty -Path $secKey -Name "DisableTaskMgr" -ErrorAction SilentlyContinue
}

# 2. Eliminar restricciones de Ejecutar y Tecla Windows (Explorer)
if (Test-Path $expKey) {
    Remove-ItemProperty -Path $expKey -Name "NoRun"     -ErrorAction SilentlyContinue
    Remove-ItemProperty -Path $expKey -Name "NoWinKeys" -ErrorAction SilentlyContinue
}

# 3. Lo mismo en las politicas de la maquina (si el script tiene permisos)
try {
    if (Test-Path $mSecKey) {
        Remove-ItemProperty -Path $mSecKey -Name "DisableTaskMgr" -ErrorAction SilentlyContinue
    }
    if (Test-Path $mExpKey) {
        Remove-ItemProperty -Path $mExpKey -Name "NoRun"     -ErrorAction SilentlyContinue
        Remove-ItemProperty -Path $mExpKey -Name "NoWinKeys" -ErrorAction SilentlyContinue
    }
} catch { }

# 4. Restaurar el boton Inicio del shell
if (Test-Path $advKey) {
    Remove-ItemProperty -Path $advKey -Name "Start" -ErrorAction SilentlyContinue
}

# 5. Volver a permitir que Windows reinicie explorer.exe
try {
    if (Test-Path $wlKey) {
        Remove-ItemProperty -Path $wlKey -Name "AutoRestartShell" -ErrorAction SilentlyContinue
    }
} catch { }
try {
    if (Test-Path $wlKeyCU) {
        Remove-ItemProperty -Path $wlKeyCU -Name "AutoRestartShell" -ErrorAction SilentlyContinue
    }
} catch { }

# 5b. Volver a permitir Ctrl+Alt+Supr (DisableCAD)
try {
    if (Test-Path $wlKey) {
        Remove-ItemProperty -Path $wlKey -Name "DisableCAD" -ErrorAction SilentlyContinue
    }
} catch { }

# 6. Quitar el autoarranque del kiosko (si esta activado)
# (la ruta "Windows NT\...\Run" es la que usaban versiones antiguas por error;
#  se limpia tambien por si quedo ahi)
foreach ($runKey in @("HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run",
                      "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Run")) {
    if (Test-Path $runKey) {
        Remove-ItemProperty -Path $runKey -Name "CiberAhKiosko" -ErrorAction SilentlyContinue
    }
}

# 6b. Parar el bloqueador de teclado (kbdblock.exe)
Stop-Process -Name "kbdblock" -Force -ErrorAction SilentlyContinue

# 7. Restaurar explorer.exe por si acaso antes de salir
if (-not (Get-Process -Name "explorer" -ErrorAction SilentlyContinue)) {
    Start-Process "explorer.exe"
}

# 8. Forzar el Cierre de Sesión en Windows XP para aplicar los cambios en limpio
(Get-WmiObject -Class Win32_OperatingSystem).Win32Shutdown(0)
