#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include <string.h>

#define UNUSED(x) (void)(x)

HINSTANCE g_hInst;
const wchar_t g_szAppName[] = TEXT("VPN Elapsed Timer");
const wchar_t g_szClassName[] = TEXT("VpnElapsedTimerWindowClass");
const wchar_t g_szAppVersion[] = TEXT("0.1.0");
const wchar_t g_szConfigFileName[] = TEXT("vpnelapsedtimer.ini");
const wchar_t g_szConfigSectionName[] = TEXT("vpnelapsedtimer");
wchar_t g_szConfigFullPath[10240];
signed long g_lngCalculatedElapsedTimeInSecond;
wchar_t g_szConnectState[100];
wchar_t g_szProfileVpnExecutable[10240];
unsigned int g_boolNotifyOnConnect;
unsigned int g_boolNotifyOnDisconnect;
long g_lngNotifyElapsedTime;
long g_lngCheckIntervalInSecond;

#define g_intNotifyIconId 1
#define WM_USER_TRAYICONMESSAGE (WM_APP + 1)
#define g_intTimerId 1

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void TimerProc(HWND hwnd);

void ChangeConnectState(HWND hwnd, wchar_t *szNewState);

void CheckVpnConnection(HWND hwnd);

void ChangeElapsedTimeInSecond(HWND hwnd, unsigned long lngElapsedTimeInSecond);

void SecondsToHHMMSS(signed long lngSeconds, wchar_t *result);

signed long HHMMSSToSeconds(wchar_t *szHHMMSS);

void MakeTooltipText(wchar_t *result);

void SetCalculatedElapsedTimeInSecond(HWND hwnd, long lngCalculatedElapsedTimeInSecond);

void ShowAboutWindow(HWND hwnd);

void ReadConfiguration();

void CloseApplication(HWND hwnd);

void RefreshTrayIcon(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG Msg;
    HWND hwnd;
    WNDCLASSEX wc;
    NOTIFYICONDATAW niData;

    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    g_hInst = hInstance;
    g_lngCalculatedElapsedTimeInSecond = -1;
    lstrcpyW(g_szConnectState, TEXT(""));

    ReadConfiguration();

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
        return 2;
    }

    if (0 == SetTimer(hwnd, g_intTimerId, 1000, NULL)) {
        MessageBox(NULL, TEXT("SetTimer Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 3;
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
        return 4;
    }
    if (!Shell_NotifyIcon(NIM_SETVERSION, &niData)) {
        MessageBox(NULL, TEXT("Shell_NotifyIcon[NIM_SETVERSION] Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        return 5;
    }
    if (niData.hIcon) {
        DestroyIcon(niData.hIcon);
    }


    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

unsigned long ExecCommand(wchar_t *cmdline, char *output, unsigned long length) {
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
    unsigned long lngWaitCount = 0;
    unsigned long lngReadBytes = 0;
    while (!isProcessSignaled) {
        DWORD lpTotalBytesAvail = 0;
        if (WaitForSingleObject(pi.hProcess, 10000) == WAIT_OBJECT_0) {
            DWORD lngExitCode = 0;
            GetExitCodeProcess(pi.hProcess, &lngExitCode);
            if (lngExitCode != 0) {
#ifdef DEBUG
                printf("exit code error =%ld\n", lngExitCode);
                fflush(stdout);
#endif
                return 4;
            }
            isProcessSignaled = 1;
        }
        PeekNamedPipe(hReadPipe, NULL, 0, NULL, &lpTotalBytesAvail, NULL);
        if (lpTotalBytesAvail == 0) {
            lngWaitCount += 1;
            if (lngWaitCount > 3) {
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
    wchar_t szCommandLine[10240];
    char szOutput[10240];
    unsigned long result;
    wchar_t szState[1024];

    ZeroMemory(szOutput, sizeof(szOutput));
    wsprintfW(szCommandLine, TEXT("\"%s\" stats"), g_szProfileVpnExecutable);
    result = ExecCommand(szCommandLine, szOutput, sizeof(szOutput));
    if (result > 0) {
        wchar_t szMessage[10240];
        CloseApplication(hwnd);
        wsprintfW(szMessage, TEXT("Could not execute %s (error %ld)"), g_szProfileVpnExecutable, result);
        MessageBox(hwnd, szMessage, g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        exit(6);
    }
    char *line = strtok(szOutput, "\r\n");
    boolean hasCheckedTimeConnected = 0;
    boolean hasCheckedConnectState = 0;
    while (line) {
        char *pos;
        const char szTimeConnectedMatch[] = "Time Connected: ";
        if (!hasCheckedTimeConnected) {
            pos = strstr(line, szTimeConnectedMatch);
            if (pos) {
                pos += strlen(szTimeConnectedMatch);
                if (*pos == '\0') {
                    line = strtok(NULL, "\r\n");
                    continue;
                }
                while (*pos == ' ') {
                    pos += 1;
                }
                wchar_t szTime[10240];
                mbstowcs(szTime, pos, sizeof(szTime));
                signed long lngElapsedTimeInSecond = HHMMSSToSeconds(szTime);
                if (lngElapsedTimeInSecond >= 0) {
                    ChangeElapsedTimeInSecond(hwnd, lngElapsedTimeInSecond);
                }
                hasCheckedTimeConnected = 1;
            }
        }

        const char szStateMatch[] = "state: ";
        if (!hasCheckedConnectState) {
            pos = strstr(line, szStateMatch);
            if (pos) {
                pos += strlen(szStateMatch);
                if (*pos == '\0') {
                    line = strtok(NULL, "\r\n");
                    continue;
                }
#ifdef DEBUG
                printf("STATE=%s\n", pos);
                fflush(stdout);
#endif
                mbstowcs(szState, pos, sizeof(szState));
                if (lstrcmpW(szState, g_szConnectState) != 0) {
                    ChangeConnectState(hwnd, szState);
                }
                hasCheckedConnectState = 1;
            }
        }
        line = strtok(NULL, "\r\n");
    }
}

void ChangeConnectState(HWND hwnd, wchar_t *szNewState) {
    NOTIFYICONDATAW niData;
    if (lstrcmpW(g_szConnectState, TEXT("")) != 0 && lstrcmpW(szNewState, TEXT("Disconnected")) == 0 && g_boolNotifyOnDisconnect != 0) {
#ifdef DEBUG
        printf("State Change: Disconnected\n");
        fflush(stdout);
#endif

        ZeroMemory(&niData, sizeof(niData));
        niData.cbSize = sizeof(niData);
        niData.uVersion = NOTIFYICON_VERSION_4;
        niData.hWnd = hwnd;
        niData.uID = g_intNotifyIconId;
        niData.uFlags = NIF_INFO | NIF_SHOWTIP;
        lstrcpynW(niData.szInfo, TEXT("VPN Disconnected"), sizeof(niData.szInfo));
        lstrcpynW(niData.szInfoTitle, g_szAppName, sizeof(niData.szInfoTitle));
        Shell_NotifyIcon(NIM_MODIFY, &niData);
    }
    if (lstrcmpW(g_szConnectState, TEXT("")) != 0 && lstrcmpW(szNewState, TEXT("Connected")) == 0 && g_boolNotifyOnConnect != 0) {
#ifdef DEBUG
        printf("State Change: Connected\n");
        fflush(stdout);
#endif
        ZeroMemory(&niData, sizeof(niData));
        niData.cbSize = sizeof(niData);
        niData.uVersion = NOTIFYICON_VERSION_4;
        niData.hWnd = hwnd;
        niData.uID = g_intNotifyIconId;
        niData.uFlags = NIF_INFO | NIF_SHOWTIP;
        lstrcpynW(niData.szInfo, TEXT("VPN Connected"), sizeof(niData.szInfo));
        lstrcpynW(niData.szInfoTitle, g_szAppName, sizeof(niData.szInfoTitle));
        Shell_NotifyIcon(NIM_MODIFY, &niData);
    }
    lstrcpyW(g_szConnectState, szNewState);
    if (lstrcmpW(szNewState, TEXT("Connected")) != 0) {
        g_lngCalculatedElapsedTimeInSecond = -1;
    }
}

void ChangeElapsedTimeInSecond(HWND hwnd, unsigned long lngElapsedTimeInSecond) {
    UNUSED(hwnd);
    g_lngCalculatedElapsedTimeInSecond = lngElapsedTimeInSecond;
}

void TimerProc(HWND hwnd) {
    static signed long intCallCount = -1;
    int boolNeedRefresh = 0;
    if (g_lngCalculatedElapsedTimeInSecond >= 0) {
        SetCalculatedElapsedTimeInSecond(hwnd, g_lngCalculatedElapsedTimeInSecond + 1);
        boolNeedRefresh = 1;
    }
    intCallCount += 1;
    if (intCallCount == 0 || intCallCount >= g_lngCheckIntervalInSecond) {
        intCallCount = 0;
        CheckVpnConnection(hwnd);
        boolNeedRefresh = 1;
    }
    wchar_t time[100];
    SecondsToHHMMSS(g_lngCalculatedElapsedTimeInSecond, time);
#ifdef DEBUG
    wprintf(TEXT("state: [%s] elapsed: [%ld sec] [%s]\n"), g_szConnectState, g_lngCalculatedElapsedTimeInSecond, time);
    fflush(stdout);
#endif
    if (boolNeedRefresh) {
        RefreshTrayIcon(hwnd);
    }
}

void SetCalculatedElapsedTimeInSecond(HWND hwnd, long lngCalculatedElapsedTimeInSecond) {
    static DWORD lngLastTick = 0;
    g_lngCalculatedElapsedTimeInSecond = lngCalculatedElapsedTimeInSecond;
    if (g_lngCalculatedElapsedTimeInSecond == g_lngNotifyElapsedTime) {
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
            wsprintfW(niData.szInfoTitle, g_szAppName, sizeof(niData.szInfoTitle));
            Shell_NotifyIcon(NIM_MODIFY, &niData);

            lngLastTick = lngCurrentTick;
        }
    }
}

void RefreshTrayIcon(HWND hwnd) {
    static HICON hOldIcon = NULL;
    NOTIFYICONDATA niData;
    if (hOldIcon) {
        DestroyIcon(hOldIcon);
    }
    if (lstrcmpW(g_szConnectState, TEXT("Connected")) == 0) {
        wchar_t time[100];
        wchar_t text1[100];
        wchar_t text2[100];
        SecondsToHHMMSS(g_lngCalculatedElapsedTimeInSecond, time);
        lstrcpynW(text1, time + 1, 2 + 1);
        lstrcpynW(text2, time + 3, 2 + 1);

        RECT rect;
        SetRect(&rect, 0, 0, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
        HDC hDC = GetDC(hwnd);
        HDC hMemDC = CreateCompatibleDC(hDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(hDC, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
        HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);

        HBRUSH hBrush = CreateSolidBrush(RGB(80, 80, 80));
        FillRect(hMemDC, &rect, hBrush);
        DeleteObject(hBrush);

        LOGFONT lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = GetSystemMetrics(SM_CYSMICON) * 1.5 / 2;
        if (lf.lfHeight < 14) {
            lf.lfHeight = 14;
        }
        lf.lfWeight = FW_NORMAL;
        lf.lfOutPrecision = OUT_TT_PRECIS;
        lf.lfQuality = ANTIALIASED_QUALITY;
        lf.lfPitchAndFamily = FF_SWISS;
        lstrcpy(lf.lfFaceName, TEXT("Arial"));


        HFONT hFont = CreateFontIndirect(&lf);
        HGDIOBJ hOldFont = SelectObject(hMemDC, hFont);
        SetTextColor(hMemDC, RGB(255, 255, 255));
        SetTextAlign(hMemDC, TA_BASELINE | TA_LEFT);
        SetBkMode(hMemDC, TRANSPARENT);

        TextOut(hMemDC, 0, GetSystemMetrics(SM_CYSMICON) / 2, text1, lstrlen(text1));
        TextOut(hMemDC, 0, GetSystemMetrics(SM_CYSMICON) / 2 * 2, text2, lstrlen(text2));

        HDC hMaskDC = CreateCompatibleDC(hDC);
        HBITMAP hMaskBitmap = CreateCompatibleBitmap(hMaskDC, GetSystemMetrics(SM_CXSMICON),
                                                     GetSystemMetrics(SM_CYSMICON));
        HGDIOBJ hOldMaskBitmap = SelectObject(hMaskDC, hMaskBitmap);
        HBRUSH hMaskBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hMaskDC, &rect, hMaskBrush);
        DeleteObject(hMaskBrush);

        ICONINFO ii;
        ZeroMemory(&ii, sizeof(ii));
        ii.fIcon = TRUE;
        ii.hbmMask = hMaskBitmap;
        ii.hbmColor = hBitmap;
        HICON hNewIcon = CreateIconIndirect(&ii);
        hOldIcon = hNewIcon;

        ZeroMemory(&niData, sizeof(niData));
        niData.cbSize = sizeof(niData);
        niData.uVersion = NOTIFYICON_VERSION_4;
        niData.hWnd = hwnd;
        niData.uID = g_intNotifyIconId;
        niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
        niData.dwInfoFlags = NIIF_NONE;
        niData.hIcon = hNewIcon;
        niData.uCallbackMessage = WM_USER_TRAYICONMESSAGE;
        MakeTooltipText(niData.szTip);
        Shell_NotifyIcon(NIM_MODIFY, &niData);

        SelectObject(hMaskDC, hOldMaskBitmap);
        DeleteObject(hMaskBitmap);
        DeleteDC(hMaskDC);
        SelectObject(hMemDC, hOldFont);
        DeleteObject(hFont);
        SelectObject(hMemDC, hOldBitmap);
        DeleteObject(hBitmap);

        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
    } else {
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
        Shell_NotifyIcon(NIM_MODIFY, &niData);
    }
}

void SecondsToHHMMSS(signed long lngSeconds, wchar_t *result) {
    if (lngSeconds < 0) {
        lstrcpyW(result, TEXT("--:--:--"));
        return;
    }
    unsigned int hour = lngSeconds / 60 / 60;
    unsigned int minute = lngSeconds / 60 % 60;
    unsigned int second = lngSeconds % 60;
    wsprintfW(result, TEXT("%02d:%02d:%02d"), hour, minute, second);
}

signed long HHMMSSToSeconds(wchar_t *szHHMMSS) {
    if (lstrlenW(szHHMMSS) == 8 && szHHMMSS[2] == ':' && szHHMMSS[5] == ':') {
        szHHMMSS[2] = '\0';
        szHHMMSS[5] = '\0';
        return (_wtoi(szHHMMSS) * 60 * 60 + _wtoi(&szHHMMSS[3]) * 60 + _wtoi(&szHHMMSS[6]));
    }
    return -1;
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

    HMENU menu, submenu;
    POINT pt;
    UINT wmId, wmEvent, nIconId;

    switch (msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            CloseApplication(hwnd);
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
                    ShowAboutWindow(hwnd);
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

void ShowAboutWindow(HWND hwnd) {
    wchar_t szMessage[10240];
    wchar_t szNotifyElapsedTime[100];
    wchar_t szCalculatedElapsedTime[100];
    SecondsToHHMMSS(g_lngNotifyElapsedTime, szNotifyElapsedTime);
    SecondsToHHMMSS(g_lngCalculatedElapsedTimeInSecond, szCalculatedElapsedTime);
    wsprintfW(szMessage, TEXT("%s %s\n\n"
                              "* Configuration *\n"
                              "File: %s\n"
                              "VpnExecutable: %s\n"
                              "NotifyOnConnect: %ld (1:Yes 0:No)\n"
                              "NotifyOnDisconnect: %ld (1:Yes 0:No)\n"
                              "NotifyElapsedTime: %s (%ld seconds)\n"
                              "CheckIntervalInSecond: %ld\n"
                              "\n"
                              "* Status *\n"
                              "CalculatedElapsedTimeInSecond: %s (%ld seconds)\n"
                              "ConnectState: %s\n"),
              g_szAppName, g_szAppVersion, \
              g_szConfigFullPath,
              g_szProfileVpnExecutable, \
              g_boolNotifyOnConnect, \
              g_boolNotifyOnDisconnect, \
              szNotifyElapsedTime, g_lngNotifyElapsedTime,
              g_lngCheckIntervalInSecond,
              szCalculatedElapsedTime, g_lngCalculatedElapsedTimeInSecond,
              g_szConnectState);
    MessageBox(hwnd, szMessage, g_szAppName, MB_ICONINFORMATION);
}

void ReadConfiguration() {
    wchar_t szExecutableFullPath[MAX_PATH];
    wchar_t szDrive[MAX_PATH];
    wchar_t szDirectory[MAX_PATH];
    wchar_t szFileName[MAX_PATH];
    wchar_t szExtension[MAX_PATH];
    if (0 == GetModuleFileName(NULL, szExecutableFullPath, MAX_PATH)) {
        MessageBox(NULL, TEXT("GetModuleFileName Failed"), g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        exit(7);
    }
    _wsplitpath(szExecutableFullPath, szDrive, szDirectory, szFileName, szExtension);
    wsprintfW(g_szConfigFullPath, TEXT("%s%s%s"), szDrive, szDirectory, g_szConfigFileName);

    GetPrivateProfileString(g_szConfigSectionName,
                            TEXT("VpnExecutable"),
                            TEXT("C:\\Program Files (x86)\\Cisco\\Cisco AnyConnect Secure Mobility Client\\vpnui.exe"),
                            g_szProfileVpnExecutable,
                            sizeof(g_szProfileVpnExecutable),
                            g_szConfigFullPath);
    g_boolNotifyOnConnect = GetPrivateProfileInt(g_szConfigSectionName,
                                                 TEXT("NotifyOnConnect"), 1, g_szConfigFullPath);
    g_boolNotifyOnDisconnect = GetPrivateProfileInt(g_szConfigSectionName,
                                                    TEXT("NotifyOnDisconnect"), 1, g_szConfigFullPath);
    wchar_t szTime[10240];
    GetPrivateProfileString(g_szConfigSectionName,
                            TEXT("NotifyElapsedTime"),
                            TEXT("01:55:00"),
                            szTime,
                            sizeof(szTime),
                            g_szConfigFullPath);
    if (lstrcmpW(szTime, TEXT("")) == 0) {
        g_lngNotifyElapsedTime = 0;
    } else {
        signed long lngNotifyElapsedTime = HHMMSSToSeconds(szTime);
        if (lngNotifyElapsedTime < 0) {
            MessageBox(NULL, TEXT("Configuration 'NotifyElapsedTime' parsing failed. Allowed values: '' or HH:MM:SS"),
                       g_szAppName, MB_ICONEXCLAMATION | MB_OK);
            exit(8);
        }
        g_lngNotifyElapsedTime = lngNotifyElapsedTime;
    }
    g_lngCheckIntervalInSecond = GetPrivateProfileInt(g_szConfigSectionName,
                                                      TEXT("CheckIntervalInSecond"), 10, g_szConfigFullPath);
    if (g_lngCheckIntervalInSecond <= 0 || 10000 < g_lngCheckIntervalInSecond) {
        MessageBox(NULL, TEXT("Configuration 'CheckIntervalInSecond' has invalid value. Allowed values: 1-10000"),
                   g_szAppName, MB_ICONEXCLAMATION | MB_OK);
        exit(9);
    }
}

void CloseApplication(HWND hwnd) {
    NOTIFYICONDATA niData;
    ZeroMemory(&niData, sizeof(niData));
    niData.cbSize = sizeof(niData);
    niData.uVersion = NOTIFYICON_VERSION_4;
    niData.hWnd = hwnd;
    niData.uID = g_intNotifyIconId;
    Shell_NotifyIcon(NIM_DELETE, &niData);

    KillTimer(hwnd, g_intTimerId);
}