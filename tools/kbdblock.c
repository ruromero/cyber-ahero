/* =========================================================================
 * kbdblock.exe - Traga TODAS las teclas mientras el launcher esta en pantalla
 * -------------------------------------------------------------------------
 *  Se usa en modo kiosko: se arranca junto a la app del ciber y se para en
 *  cuanto se detecta un juego (para que el jugador pueda usar el teclado).
 *
 *  Usa un hook global de bajo nivel (WH_KEYBOARD_LL), asi que ninguna tecla
 *  llega a ninguna ventana: ni al shell, ni al launcher, ni a nada.
 *  (Ctrl+Alt+Del es una excepcion: no se puede capturar asi, para eso esta
 *   la polica DisableCAD que aplica kiosko.ps1)
 * ========================================================================= */

#include <windows.h>

static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    (void)wParam;
    (void)lParam;
    if (nCode == HC_ACTION || nCode == HC_NOREMOVE)
        return 1; /* tragamos la tecla */
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrev;
    (void)lpCmdLine;
    (void)nCmdShow;

    HHOOK hhk = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, hInst, 0);
    if (!hhk)
        return 1;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hhk);
    return 0;
}
