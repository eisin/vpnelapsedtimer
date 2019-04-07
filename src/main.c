#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include <string.h>

HINSTANCE g_hInst;
wchar_t g_szAppName[] = TEXT("VPN Elapsed Timer");
wchar_t g_szClassName[] = TEXT("VpnElapsedTimerWindowClass");
signed long g_lngCalculatedElapsedTimeInSecond;
wchar_t g_szConnectState[100];

#define g_intNotifyIconId 1
#define WM_USER_TRAYICONMESSAGE (WM_APP + 1)
#define g_intTimerId 1

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void TimerProc(HWND hwnd);
void ChangeConnectState(HWND hwnd, wchar_t *szNewState);
void CheckVpnConnection(HWND hwnd);
void ChangeElapsedTimeInSecond(HWND hwnd, unsigned long lngElapsedTimeInSecond);
void SecondsToHHMMSS(unsigned long lngSeconds, wchar_t *result);
void MakeTooltipText(wchar_t *result);
void SetCalculatedElapsedTimeInSecond(HWND hwnd, unsigned long lngCalculatedElapsedTimeInSecond);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG Msg;
    HWND hwnd;
    WNDCLASSEX wc;
    NOTIFYICONDATAW niData;

    g_hInst = hInstance;
    g_lngCalculatedElapsedTimeInSecond = -1;
    lstrcpyW(g_szConnectState, TEXT(""));

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
        MessageBox(NULL, TEXT("RegisterClassEx Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, g_szClassName, TEXT(""), 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, TEXT("CreateWindowEx Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    if (0 == SetTimer(hwnd, g_intTimerId, 10, NULL)) {
        MessageBox(NULL, TEXT("SetTimer Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
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
    MakeTooltipText(niData.szTip);


    if (!Shell_NotifyIcon(NIM_ADD, &niData)) {
        MessageBox(NULL, TEXT("Shell_NotifyIcon[NIM_ADD] Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    if (!Shell_NotifyIcon(NIM_SETVERSION, &niData)) {
        MessageBox(NULL, TEXT("Shell_NotifyIcon[NIM_SETVERSION] Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
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

unsigned long ExecCommand(wchar_t* cmdline, unsigned char* output, unsigned long length) {
    wchar_t cmdline_variable[10240];
    lstrcpynW(cmdline_variable, cmdline, sizeof(cmdline_variable));
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    HANDLE hReadPipe, hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, cmdline_variable, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return 1;
    }
    unsigned long isProcessSignaled = 0;
    unsigned long waitcount = 0;
    unsigned long lngReadBytes = 0;
    while (!isProcessSignaled) {
        DWORD lpTotalBytesAvail = 0;
        if (WaitForSingleObject(pi.hProcess, 1000) == WAIT_OBJECT_0) {
            DWORD lngExitCode = 0;
            GetExitCodeProcess(pi.hProcess, &lngExitCode);
            if (lngExitCode != 0) {
                printf("exit code error =%d\n", lngExitCode);
                return 4;
            }
            isProcessSignaled = 1;
        }
        PeekNamedPipe(hReadPipe, NULL, 0, NULL, &lpTotalBytesAvail, NULL);
        if (lpTotalBytesAvail == 0) {
            waitcount += 1;
            if (waitcount > 30) {
                return 2;
            }
            continue;
        }
        unsigned long lngBytesToRead = lpTotalBytesAvail;
        if (lpTotalBytesAvail > length - 1) {
            lngBytesToRead = length - 1;
        }
        ReadFile(hReadPipe, output, lngBytesToRead, &lngReadBytes, NULL);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
        return 0;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);
    return 3;
}

void CheckVpnConnection(HWND hwnd) {
    unsigned char output[10240];
    unsigned long result;
    signed long lngElapsedTimeInSecond = 0;
    wchar_t szState[1024];

    ZeroMemory(output, sizeof(output));
    result = ExecCommand(TEXT("..\\test\\vpnui-test-1.bat stats"), output, sizeof(output));
    char *line = strtok(output, "\r\n");
    while(line) {
        char* pos;
        const char szTimeConnectedMatch[] = "Time Connected: ";
        pos = strstr(line, szTimeConnectedMatch);
        if (pos) {
            pos += strlen(szTimeConnectedMatch);
            if (*pos == '\0') {
                line = strtok(NULL, "\r\n");
                continue;
            }
            if (strlen(pos) == 8 && pos[2] == ':' && pos[5] == ':') {
                pos[2] = '\0';
                pos[5] = '\0';
                lngElapsedTimeInSecond = atoi(pos) * 60 * 60 + atoi(&pos[3]) * 60 + atoi(&pos[6]);
            }
            ChangeElapsedTimeInSecond(hwnd, lngElapsedTimeInSecond);
        }

        const char szStateMatch[] = "state: ";
        pos = strstr(line, szStateMatch);
        if (pos) {
            pos += strlen(szTimeConnectedMatch);
            if (*pos == '\0') {
                line = strtok(NULL, "\r\n");
                continue;
            }
            printf("STATE=%s\n", pos);
            mbstowcs(szState, pos, sizeof(szState));
            if (lstrcmpW(szState, g_szConnectState) != 0) {
                ChangeConnectState(hwnd, szState);
            }
        }
        line = strtok(NULL, "\r\n");
    }
}

void ChangeConnectState(HWND hwnd, wchar_t* szNewState) {
    NOTIFYICONDATAW niData;
    if (lstrcmpW(szNewState, TEXT("Disconnected")) == 0) {
        printf("State Change: Disconnected\n");

        ZeroMemory(&niData, sizeof(niData));
        niData.cbSize = sizeof(niData);
        niData.uVersion = NOTIFYICON_VERSION_4;
        niData.hWnd = hwnd;
        niData.uID = g_intNotifyIconId;
        niData.uFlags = NIF_INFO | NIF_SHOWTIP;
        lstrcpynW(niData.szInfo, TEXT("VPN Connected"), sizeof(niData.szInfo));
        lstrcpynW(niData.szInfoTitle, TEXT("VPN Elapsed Timer"), sizeof(niData.szInfoTitle));
        Shell_NotifyIcon(NIM_MODIFY, &niData);
    }
    if (lstrcmpW(szNewState, TEXT("Connected")) == 0) {
        printf("State Change: Connected\n");
        ZeroMemory(&niData, sizeof(niData));
        niData.cbSize = sizeof(niData);
        niData.uVersion = NOTIFYICON_VERSION_4;
        niData.hWnd = hwnd;
        niData.uID = g_intNotifyIconId;
        niData.uFlags = NIF_INFO | NIF_SHOWTIP;
        lstrcpynW(niData.szInfo, TEXT("VPN Disconnected"), sizeof(niData.szInfo));
        lstrcpynW(niData.szInfoTitle, TEXT("VPN Elapsed Timer"), sizeof(niData.szInfoTitle));
        Shell_NotifyIcon(NIM_MODIFY, &niData);
    }
    lstrcpyW(g_szConnectState, szNewState);
    if (lstrcmpW(szNewState, TEXT("Connected")) != 0) {
        g_lngCalculatedElapsedTimeInSecond = -1;
    }
}

void ChangeElapsedTimeInSecond(HWND hwnd, unsigned long lngElapsedTimeInSecond) {
    g_lngCalculatedElapsedTimeInSecond = lngElapsedTimeInSecond;
}

void TimerProc(HWND hwnd) {
    static signed int intCallCount = -1;
    if (g_lngCalculatedElapsedTimeInSecond >= 0) {
        SetCalculatedElapsedTimeInSecond(hwnd, g_lngCalculatedElapsedTimeInSecond + 1);
    }
    intCallCount += 1;
    if (intCallCount == 0 || intCallCount >= 5) {
        intCallCount = 0;
        CheckVpnConnection(hwnd);
    }
    wchar_t time[100];
    SecondsToHHMMSS(g_lngCalculatedElapsedTimeInSecond, time);
    wprintf(TEXT("state: [%s] elapsed: [%d sec] [%s]\n"), g_szConnectState, g_lngCalculatedElapsedTimeInSecond, time);
    fflush(stdout);
}

void SetCalculatedElapsedTimeInSecond(HWND hwnd, unsigned long lngCalculatedElapsedTimeInSecond) {
    static DWORD lngLastTick = 0;
    g_lngCalculatedElapsedTimeInSecond = lngCalculatedElapsedTimeInSecond;
    //if (g_lngCalculatedElapsedTimeInSecond == 1 * 60 * 60 + 55 * 60) {
    if (g_lngCalculatedElapsedTimeInSecond == 1 * 60 * 60 + 17 * 60 + 36) {
        DWORD lngCurrentTick = GetTickCount();
        if (lngLastTick == 0 || lngCurrentTick - lngLastTick > 10 * 1000) {
            wchar_t time[100];
            SecondsToHHMMSS(g_lngCalculatedElapsedTimeInSecond, time);

            NOTIFYICONDATA niData;
            ZeroMemory(&niData, sizeof(niData));
            niData.cbSize = sizeof(niData);
            niData.uVersion = NOTIFYICON_VERSION_4;
            niData.hWnd = hwnd;
            niData.uID = g_intNotifyIconId;
            niData.uFlags = NIF_INFO | NIF_SHOWTIP;
            wsprintfW(niData.szInfo, TEXT("%s %s"), g_szConnectState, time);
            wcsncpy_s(niData.szInfoTitle, sizeof(niData.szInfoTitle), TEXT("VPN Elapsed Timer"), _TRUNCATE);
            Shell_NotifyIcon(NIM_MODIFY, &niData);

            lngLastTick = lngCurrentTick;
        }
    }
}

void SecondsToHHMMSS(unsigned long lngSeconds, wchar_t *result) {
    unsigned int hour = lngSeconds / 60 / 60;
    unsigned int minute = lngSeconds / 60 % 60;
    unsigned int second = lngSeconds % 60;
    wsprintfW(result, TEXT("%02d:%02d:%02d"), hour, minute, second);
}

void MakeTooltipText(wchar_t *result) {
    if (lstrcmpW(g_szConnectState, TEXT("")) == 0) {
        wsprintfW(result, TEXT("VPN Elapsed Timer"));
    } else if (lstrcmpW(g_szConnectState, TEXT("Connected")) == 0) {
        wchar_t time[100];
        SecondsToHHMMSS(g_lngCalculatedElapsedTimeInSecond, time);
        wsprintfW(result, TEXT("%s - VPN Elapsed Timer"), time);
    } else {
        wsprintfW(result, TEXT("%s - VPN Elapsed Timer"), g_szConnectState);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    RECT rect;
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
            Shell_NotifyIcon(NIM_DELETE, &niData);
            PostQuitMessage(0);
            break;

        case WM_LBUTTONDBLCLK:
            SetForegroundWindow(hwnd);
            menu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TRAYMENU));
            if (menu) {
                submenu = GetSubMenu(menu, 0);
                if (submenu) {
                    GetCursorPos(&pt);
                    TrackPopupMenuEx(submenu, TPM_RIGHTBUTTON, pt.x, pt.y, hwnd, NULL);
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
                    wcsncpy_s(niData.szInfo, sizeof(niData.szInfo), TEXT("This is a test"), _TRUNCATE);
                    wcsncpy_s(niData.szInfoTitle, sizeof(niData.szInfoTitle), TEXT("Title"), _TRUNCATE);
                    Shell_NotifyIcon(NIM_MODIFY, &niData);
                    break;

                case ID_TRAYMENU_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }
            break;

        case WM_TIMER:
            TimerProc(hwnd);
            break;


        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

