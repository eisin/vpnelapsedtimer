#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include <string.h>

HINSTANCE g_hInst;
const char g_szAppName[] = "VPN Elapsed Timer";
const char g_szClassName[] = "VpnElapsedTimerWindowClass";

#define g_intNotifyIconId 1
#define WM_USER_TRAYICONMESSAGE (WM_APP + 1)

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG Msg;
    HWND hwnd;
    WNDCLASSEX wc;
    NOTIFYICONDATA niData;

    g_hInst = hInstance;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpszMenuName = NULL;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szClassName;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wc.lpfnWndProc = (WNDPROC) &WndProc;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "RegisterClassEx Failed", g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, g_szClassName, "", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "CreateWindowEx Failed", g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    ZeroMemory(&niData, sizeof(niData));
    niData.cbSize = sizeof(niData);
    niData.uVersion = NOTIFYICON_VERSION_4;
    niData.hWnd = hwnd;
    niData.uID = g_intNotifyIconId;
    niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    niData.dwInfoFlags = NIIF_NONE;
    niData.hIcon = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_SYSTRAY_NORMAL), IMAGE_ICON,
                               GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    niData.uCallbackMessage = WM_USER_TRAYICONMESSAGE;
    strncpy_s(niData.szTip, sizeof(niData.szTip), TEXT("Test message"), _TRUNCATE);


    if (!Shell_NotifyIcon(NIM_ADD, &niData)) {
        MessageBox(NULL, "Shell_NotifyIcon[NIM_ADD] Failed", g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    if (!Shell_NotifyIcon(NIM_SETVERSION, &niData)) {
        MessageBox(NULL, "Shell_NotifyIcon[NIM_SETVERSION] Failed", g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }    if (niData.hIcon) {
        DestroyIcon(niData.hIcon);
    }


    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    RECT rect;
    TEXTMETRIC tm;
    PAINTSTRUCT ps;
    HMENU menu, submenu;
    POINT pt;
    UINT wmId, wmEvent, nIconId;
    NOTIFYICONDATAW niData;

    switch (msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            ZeroMemory(&niData, sizeof(niData));
            niData.cbSize = sizeof(niData);
            niData.uVersion = NOTIFYICON_VERSION_4;
            niData.hWnd = hwnd;
            niData.uID = g_intNotifyIconId;
            Shell_NotifyIconW(NIM_DELETE, &niData);
            PostQuitMessage(0);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            DrawText(hdc, TEXT ("Native Windows Development with CygWin and CLion."),
                     -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
            return 0;

        case WM_LBUTTONDBLCLK:
            SetForegroundWindow(hwnd);
            menu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TRAYMENU));
            if (menu) {
                submenu = GetSubMenu(menu, 0);
                if (submenu) {
                    GetCursorPos(&pt);
                    TrackPopupMenuEx(submenu, TPM_RIGHTBUTTON, pt.x, pt.y, hwnd, NULL);
                    //ShowContextMenu(hwnd);
                }
                DestroyMenu(menu);
            }
            return 0;

        case WM_USER_TRAYICONMESSAGE:

            wmEvent = LOWORD(lParam);
            nIconId = HIWORD(lParam);
            if (nIconId != g_intNotifyIconId) {
                break;
            }
            switch (wmEvent) {
                case WM_LBUTTONDBLCLK:
                    ShowWindow(hwnd, SW_RESTORE);
                    break;
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    SetForegroundWindow(hwnd);
                    menu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TRAYMENU));
                    if (menu) {
                        submenu = GetSubMenu(menu, 0);
                        if (submenu) {
                            GetCursorPos(&pt);
                            TrackPopupMenuEx(submenu, TPM_RIGHTBUTTON, pt.x, pt.y, hwnd, NULL);
                            //ShowContextMenu(hwnd);
                        }
                        DestroyMenu(menu);
                    }
                    return 0;
            }
            break;

        case WM_COMMAND:
            wmId = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            switch (wmId) {
                case ID_TRAYMENU_ABOUT:
                    ZeroMemory(&niData, sizeof(niData));
                    niData.cbSize = sizeof(niData);
                    niData.uVersion = NOTIFYICON_VERSION_4;
                    niData.hWnd = hwnd;
                    niData.uID = g_intNotifyIconId;
                    niData.uFlags = NIF_INFO | NIF_SHOWTIP;
                    wcsncpy_s(niData.szInfo, sizeof(niData.szInfo), L"This is a test", _TRUNCATE);
                    wcsncpy_s(niData.szInfoTitle, sizeof(niData.szInfoTitle), L"Title", _TRUNCATE);
                    Shell_NotifyIconW(NIM_MODIFY, &niData);
                    //
                    // MessageBox(NULL, TEXT("test"), TEXT("hello"), 0);
                    break;

                case ID_TRAYMENU_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

