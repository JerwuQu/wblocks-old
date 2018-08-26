#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "../shared/shared.h"

static HWND wblocksWnd = 0;

extern __declspec(dllexport) LRESULT WINAPI wndProcHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wblocksWnd) {
        CWPSTRUCT *data = (CWPSTRUCT*)lParam;
        if (data->message == WM_SIZE) {
            SendMessage(wblocksWnd, WBLOCKS_WM_SIZE, data->wParam, data->lParam);
        } else if (data->message == WBLOCKS_WM_PING) {
            SendMessage(wblocksWnd, WBLOCKS_WM_PONG, 0, 0);
        }
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // Find taskbar
        HWND temp = FindWindow("Shell_TrayWnd", NULL);
        if (!temp) return FALSE;
        temp = FindWindowEx(temp, NULL, "ReBarWindow32", NULL);
        if (!temp) return FALSE;
        wblocksWnd = FindWindowEx(temp, NULL, WBLOCKS_BAR_CLASS, NULL);
        if (!wblocksWnd) return FALSE;
    }

    return TRUE;
}
