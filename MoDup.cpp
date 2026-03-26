#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <fstream>
#include <vector>
#include <dwmapi.h>
#include <algorithm>

#include "Resource.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Msimg32.lib")

#include "json.hpp"
using json = nlohmann::json;

#define BTN_SWAP    1001
#define IDC_MONITOR_LIST 2000

// Colors
const COLORREF COL_BG       = RGB(250, 250, 252);
const COLORREF COL_BTN_PRI  = RGB(0, 120, 215);
const COLORREF COL_BTN_TXT_P= RGB(255, 255, 255);

struct DisplayInfo {
    int displayNum;
    std::wstring gdiName;
    std::wstring monitorName;
    LUID adapterId;
    UINT32 sourceId;
    UINT32 targetId;
    DISPLAYCONFIG_ROTATION rotation;

    HWND hCheckbox;
    bool selected;
};

// ---- Global ----
HFONT g_hFontBtn  = NULL;
HWND g_hBtnHover = NULL;
std::vector<DisplayInfo> g_displays;
HWND g_hwndMain = NULL;

// ---- Forward ----
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializeUI(HWND, HINSTANCE);
void ApplyFontAndTheme(HWND hwnd);
void DrawButton(LPDRAWITEMSTRUCT pDIS);
void EnumDisplays();
void SwapDisplays(HWND hwnd);

// Config file path
std::wstring GetConfigPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath(path);
    size_t pos = exePath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) exePath = exePath.substr(0, pos);
    return exePath + L"\\modup_settings.json";
}

void SaveLastSelection(const std::vector<int>& selectedIndices) {
    try {
        json j;
        j["last_selection"] = selectedIndices;
        std::ofstream out(GetConfigPath());
        out << j.dump();
    } catch (...) {}
}

std::vector<int> LoadLastSelection() {
    std::vector<int> result;
    try {
        std::ifstream in(GetConfigPath());
        if (!in.is_open()) return result;
        json j; in >> j;
        if (j.contains("last_selection")) {
            for (auto& item : j["last_selection"]) result.push_back(item.get<int>());
        }
    } catch (...) {}
    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const wchar_t CLASS_NAME[] = L"MoDupCCD_v33";

    WNDCLASS wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COL_BG);
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));

    RegisterClass(&wc);

    int w = 600, h = 580;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_hwndMain = CreateWindowEx(WS_EX_APPWINDOW, CLASS_NAME, L"Monitor Swap Tool (v3.3)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        sx, sy, w, h, NULL, NULL, hInstance, NULL);

    if (!g_hwndMain) return 0;

    EnumDisplays();
    InitializeUI(g_hwndMain, hInstance);
    ApplyFontAndTheme(g_hwndMain);

    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);

    MSG msg{};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}

void EnumDisplays() {
    g_displays.clear();

    UINT32 pCount, mCount;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pCount, &mCount) != ERROR_SUCCESS) return;
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(pCount);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(mCount);
    if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pCount, paths.data(), &mCount, modes.data(), NULL) != ERROR_SUCCESS) return;

    for (UINT32 i = 0; i < pCount; ++i) {
        DisplayInfo info{};
        info.adapterId = paths[i].sourceInfo.adapterId;
        info.sourceId = paths[i].sourceInfo.id;
        info.targetId = paths[i].targetInfo.id;
        info.rotation = paths[i].targetInfo.rotation;

        info.displayNum = paths[i].sourceInfo.id + 1;

        // Get GDI Name
        DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName{};
        sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        sourceName.header.size = sizeof(sourceName);
        sourceName.header.adapterId = paths[i].sourceInfo.adapterId;
        sourceName.header.id = paths[i].sourceInfo.id;
        if (DisplayConfigGetDeviceInfo(&sourceName.header) == ERROR_SUCCESS) {
            info.gdiName = sourceName.viewGdiDeviceName;
        }

        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName{};
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);
        targetName.header.adapterId = paths[i].targetInfo.adapterId;
        targetName.header.id = paths[i].targetInfo.id;
        if (DisplayConfigGetDeviceInfo(&targetName.header) == ERROR_SUCCESS) {
            info.monitorName = (wcslen(targetName.monitorFriendlyDeviceName) > 0) ? targetName.monitorFriendlyDeviceName : L"Internal Display";
        } else {
            info.monitorName = L"Internal Display";
        }

        info.selected = false;
        info.hCheckbox = NULL;
        g_displays.push_back(info);
    }

    std::sort(g_displays.begin(), g_displays.end(), [](const DisplayInfo& a, const DisplayInfo& b) {
        return a.displayNum < b.displayNum;
    });
}

void InitializeUI(HWND hwnd, HINSTANCE hInst) {
    for (int i = 0; i < 50; ++i) { HWND h = GetDlgItem(hwnd, IDC_MONITOR_LIST + i); if (h) DestroyWindow(h); }
    int startY = 30, chkH = 45, chkW = 520, gap = 12;
    for (size_t i = 0; i < g_displays.size(); ++i) {
        std::wstring gdiShort = g_displays[i].gdiName;
        if (gdiShort.find(L"\\\\.\\") == 0) gdiShort = gdiShort.substr(4);
        std::wstring rotStr = (g_displays[i].rotation == DISPLAYCONFIG_ROTATION_ROTATE90 || g_displays[i].rotation == DISPLAYCONFIG_ROTATION_ROTATE270) ? L"Portrait" : L"Landscape";
        std::wstring text = L"Display " + std::to_wstring(g_displays[i].displayNum) + L": " + g_displays[i].monitorName + L" (" + gdiShort + L") - " + rotStr;
        g_displays[i].hCheckbox = CreateWindowW(L"BUTTON", text.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,
            30, startY + (int)i * (chkH+gap), chkW, chkH, hwnd, (HMENU)(IDC_MONITOR_LIST + i), hInst, NULL);
    }

    // Load last selection and apply
    std::vector<int> lastSel = LoadLastSelection();
    for (int selIdx : lastSel) {
        for (size_t i = 0; i < g_displays.size(); ++i) {
            if (g_displays[i].displayNum == selIdx) {
                CheckDlgButton(hwnd, IDC_MONITOR_LIST + i, BST_CHECKED);
                break;
            }
        }
    }

    // Create Swap button with WS_TABSTOP for keyboard focus (Enter/Space)
    if (!GetDlgItem(hwnd, BTN_SWAP)) {
        CreateWindowW(L"BUTTON", L"Swap Settings",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            (600-240)/2, 450, 240, 60, hwnd, (HMENU)BTN_SWAP, hInst, NULL);
    }
}

void ApplyFontAndTheme(HWND hwnd) {
    int dpi = GetDpiForWindow(hwnd);
    int hText = MulDiv(-13, dpi, 96);
    if (g_hFontBtn) DeleteObject(g_hFontBtn);
    LOGFONTW lf{}; wcscpy_s(lf.lfFaceName, L"Segoe UI"); lf.lfHeight = hText; lf.lfWeight = FW_SEMIBOLD;
    g_hFontBtn = CreateFontIndirectW(&lf);
    for (auto& d : g_displays) SendMessage(d.hCheckbox, WM_SETFONT, (WPARAM)g_hFontBtn, TRUE);
    SendMessage(GetDlgItem(hwnd, BTN_SWAP), WM_SETFONT, (WPARAM)g_hFontBtn, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == BTN_SWAP) SwapDisplays(hwnd);
        else if (LOWORD(wParam) >= IDC_MONITOR_LIST && LOWORD(wParam) < IDC_MONITOR_LIST + 50) {
            int checked = 0;
            for (int i = 0; i < (int)g_displays.size(); ++i) if (IsDlgButtonChecked(hwnd, IDC_MONITOR_LIST + i) == BST_CHECKED) checked++;
            if (IsDlgButtonChecked(hwnd, LOWORD(wParam)) == BST_CHECKED && checked > 2) CheckDlgButton(hwnd, LOWORD(wParam), BST_UNCHECKED);
        }
        break;
    case WM_DRAWITEM: DrawButton((LPDRAWITEMSTRUCT)lParam); return TRUE;
    case WM_ACTIVATE: {
        HWND hBtn = GetDlgItem(hwnd, BTN_SWAP);
        if (hBtn) SetFocus(hBtn);
    } break;
    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; HWND hc = ChildWindowFromPoint(hwnd, pt);
        if (hc && hc != hwnd && hc != g_hBtnHover) {
            if (g_hBtnHover) InvalidateRect(g_hBtnHover, NULL, TRUE);
            g_hBtnHover = hc; InvalidateRect(g_hBtnHover, NULL, TRUE);
            TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0}; TrackMouseEvent(&tme);
        }
    } break;
    case WM_MOUSELEAVE: if (g_hBtnHover) { HWND hOld = g_hBtnHover; g_hBtnHover = NULL; InvalidateRect(hOld, NULL, TRUE); } break;
    case WM_DESTROY: if (g_hFontBtn) DeleteObject(g_hFontBtn); PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void DrawButton(LPDRAWITEMSTRUCT pDIS) {
    HDC hdc = pDIS->hDC; RECT rc = pDIS->rcItem;
    COLORREF bg = (pDIS->itemState & ODS_SELECTED) ? RGB(0, 100, 190) : (pDIS->hwndItem == g_hBtnHover ? RGB(20, 140, 235) : COL_BTN_PRI);
    HBRUSH hbr = CreateSolidBrush(bg); FillRect(hdc, &rc, hbr); DeleteObject(hbr);
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, COL_BTN_TXT_P);
    HGDIOBJ hOf = SelectObject(hdc, g_hFontBtn);
    wchar_t buf[256]; GetWindowTextW(pDIS->hwndItem, buf, 256);
    DrawTextW(hdc, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOf);
}

void SwapDisplays(HWND hwnd) {
    std::vector<int> sels;
    for (int i = 0; i < (int)g_displays.size(); ++i) if (IsDlgButtonChecked(hwnd, IDC_MONITOR_LIST + i) == BST_CHECKED) sels.push_back(i);
    if (sels.size() != 2) return;

    UINT32 pCount, mCount;
    GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pCount, &mCount);
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(pCount);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(mCount);
    QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pCount, paths.data(), &mCount, modes.data(), NULL);

    int pIdx1 = -1, pIdx2 = -1;
    for (UINT32 i = 0; i < pCount; ++i) {
        if (paths[i].sourceInfo.id == g_displays[sels[0]].sourceId && paths[i].sourceInfo.adapterId.LowPart == g_displays[sels[0]].adapterId.LowPart) pIdx1 = i;
        if (paths[i].sourceInfo.id == g_displays[sels[1]].sourceId && paths[i].sourceInfo.adapterId.LowPart == g_displays[sels[1]].adapterId.LowPart) pIdx2 = i;
    }

    if (pIdx1 != -1 && pIdx2 != -1) {
        DISPLAYCONFIG_PATH_TARGET_INFO info1 = paths[pIdx1].targetInfo;
        DISPLAYCONFIG_PATH_TARGET_INFO info2 = paths[pIdx2].targetInfo;

        paths[pIdx1].targetInfo.adapterId = info2.adapterId;
        paths[pIdx1].targetInfo.id = info2.id;
        paths[pIdx1].targetInfo.modeInfoIdx = info2.modeInfoIdx;

        paths[pIdx2].targetInfo.adapterId = info1.adapterId;
        paths[pIdx2].targetInfo.id = info1.id;
        paths[pIdx2].targetInfo.modeInfoIdx = info1.modeInfoIdx;

        LONG res = SetDisplayConfig(pCount, paths.data(), mCount, modes.data(), SDC_APPLY | SDC_USE_SUPPLIED_DISPLAY_CONFIG | SDC_ALLOW_CHANGES);
        if (res == ERROR_SUCCESS) {
            // Save last selection
            std::vector<int> toSave;
            toSave.push_back(g_displays[sels[0]].displayNum);
            toSave.push_back(g_displays[sels[1]].displayNum);
            SaveLastSelection(toSave);

            MessageBoxW(hwnd, L"Swapped successfully!", L"Success", MB_OK);
            EnumDisplays();
            InitializeUI(hwnd, GetModuleHandle(NULL));
            ApplyFontAndTheme(hwnd);
        } else {
            std::wstring msg = L"Failed. Code: " + std::to_wstring(res);
            MessageBoxW(hwnd, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }
}
