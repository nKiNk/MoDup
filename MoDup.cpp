#include <windows.h>
#include <windowsx.h>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
#include <dwmapi.h>

#include "Resource.h" // Added for IDI_APPICON

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Msimg32.lib")

#include "json.hpp"
using json = nlohmann::json;
namespace fs = std::filesystem;

#define BTN_SAVE    1001
#define BTN_RESTORE 1002
#define BTN_EXIT    1003

// ... (Globals remain same)
// ---- Global ----
HFONT g_hFontType = NULL;
HFONT g_hFontBtn  = NULL;

// Colors
const COLORREF COL_BG       = RGB(250, 250, 252);
const COLORREF COL_BTN_PRI  = RGB(0, 120, 215);
const COLORREF COL_BTN_SEC  = RGB(225, 225, 225);
const COLORREF COL_BTN_TXT_P= RGB(255, 255, 255);
const COLORREF COL_BTN_TXT_S= RGB(50, 50, 50);

HWND g_hBtnHover = NULL;

// ---- Forward ----
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializeUI(HWND, HINSTANCE);
void ApplyFontAndTheme(HWND hwnd);
void SaveCurrentDisplayConfig();
void RestoreDisplayConfig();
void DrawButton(LPDRAWITEMSTRUCT pDIS);

// ... (Path Helpers remain same)
static fs::path GetConfigPath()
{
    wchar_t path[MAX_PATH]{};
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0)
    {
        return fs::current_path() / "display_config.json";
    }
    return fs::path(path).parent_path() / "display_config.json";
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const wchar_t CLASS_NAME[] = L"DisplayConfigTool";

    WNDCLASS wc{};
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COL_BG);
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON)); // Restore Icon
    wc.lpszMenuName  = NULL;

    RegisterClass(&wc);

    int w = 400;
    int h = 320;
    
    int scW = GetSystemMetrics(SM_CXSCREEN);
    int scH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scW - w) / 2;
    int y = (scH - h) / 2;

    HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Monitor Config Tool",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    InitializeUI(hwnd, hInstance);
    ApplyFontAndTheme(hwnd);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// ... (WndProc remains same)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case BTN_SAVE:
            SaveCurrentDisplayConfig();
            break;
        case BTN_RESTORE:
            RestoreDisplayConfig();
            break;
        case BTN_EXIT:
            PostQuitMessage(0);
            break;
        default:
            break;
        }
        break;

    case WM_DRAWITEM:
        DrawButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;

    case WM_MOUSEMOVE:
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            HWND hChild = ChildWindowFromPoint(hwnd, pt);
            if (hChild && hChild != hwnd && hChild != g_hBtnHover)
            {
                if (g_hBtnHover) InvalidateRect(g_hBtnHover, NULL, TRUE);
                g_hBtnHover = hChild;
                InvalidateRect(g_hBtnHover, NULL, TRUE);
                
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }
        }
        break;
    
    case WM_MOUSELEAVE:
        if (g_hBtnHover)
        {
            HWND hOld = g_hBtnHover;
            g_hBtnHover = NULL;
            InvalidateRect(hOld, NULL, TRUE);
        }
        break;

    case WM_DPICHANGED:
        {
            RECT* const prcNewWindow = (RECT*)lParam;
            SetWindowPos(hwnd,
                NULL,
                prcNewWindow->left,
                prcNewWindow->top,
                prcNewWindow->right - prcNewWindow->left,
                prcNewWindow->bottom - prcNewWindow->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
             ApplyFontAndTheme(hwnd);
        }
        break;

    case WM_DESTROY:
        if (g_hFontBtn)  DeleteObject(g_hFontBtn);
        if (g_hFontType) DeleteObject(g_hFontType);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void InitializeUI(HWND hwnd, HINSTANCE hInst)
{
    // Layout - Clean Vertical Stack
    int btnW = 240;
    int btnH = 46;
    int startY = 50;
    int gapY = 15;
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientW = rc.right - rc.left;
    int x = (clientW - btnW) / 2;

    // Shortened Text
    CreateWindowW(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        x, startY, btnW, btnH, hwnd, (HMENU)BTN_SAVE, hInst, NULL);

    CreateWindowW(L"BUTTON", L"Restore", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        x, startY + btnH + gapY, btnW, btnH, hwnd, (HMENU)BTN_RESTORE, hInst, NULL);

    int exitY = startY + (btnH + gapY) * 2 + 10;
    CreateWindowW(L"BUTTON", L"Exit", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        x, exitY, btnW, btnH, hwnd, (HMENU)BTN_EXIT, hInst, NULL);
}

void ApplyFontAndTheme(HWND hwnd)
{
    int iDpi = GetDpiForWindow(hwnd);
    int hText = MulDiv(-16, iDpi, 96); // Semi-bold 10ptish

    if (g_hFontBtn) DeleteObject(g_hFontBtn);
    
    LOGFONT lf{};
    wcscpy_s(lf.lfFaceName, L"Segoe UI"); // Modern Font
    lf.lfHeight = hText;
    lf.lfWeight = FW_SEMIBOLD;
    g_hFontBtn = CreateFontIndirect(&lf);
}

void DrawButton(LPDRAWITEMSTRUCT pDIS)
{
    HDC hdc = pDIS->hDC;
    RECT rc = pDIS->rcItem;
    UINT id = pDIS->CtlID;
    BOOL isPressed = (pDIS->itemState & ODS_SELECTED);
    BOOL isHover   = (pDIS->hwndItem == g_hBtnHover);

    // Colors
    COLORREF bg, txt;
    if (id == BTN_SAVE) {
        // Primary Action (Blue)
        if (isPressed) bg = RGB(0, 100, 190);
        else if (isHover) bg = RGB(20, 140, 235);
        else bg = COL_BTN_PRI;
        txt = COL_BTN_TXT_P;
    }
    else {
        // Secondary Action (Gray/White)
        if (isPressed) bg = RGB(200, 200, 200);
        else if (isHover) bg = RGB(242, 242, 242);
        else bg = RGB(255, 255, 255); // White clean look
        txt = COL_BTN_TXT_S;
    }

    // Double buffering for smooth painting
    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hBm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hMemDC, hBm);

    // Background (Window BG first to clear corners)
    HBRUSH hBrWin = CreateSolidBrush(COL_BG);
    FillRect(hMemDC, &rc, hBrWin);
    DeleteObject(hBrWin);

    // Button Rounded Rect
    HBRUSH hBrBtn = CreateSolidBrush(bg);
    HPEN hPen = CreatePen(PS_SOLID, 1, (id == BTN_SAVE) ? bg : RGB(200,200,200)); // Border for secondary
    HGDIOBJ hOldBr = SelectObject(hMemDC, hBrBtn);
    HGDIOBJ hOldPen = SelectObject(hMemDC, hPen);

    RoundRect(hMemDC, rc.left, rc.top, rc.right, rc.bottom, 12, 12); // 12px radius

    SelectObject(hMemDC, hOldBr);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hBrBtn);
    DeleteObject(hPen);

    // Text
    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, txt);
    HGDIOBJ hOldFont = SelectObject(hMemDC, g_hFontBtn);
    
    wchar_t buf[256];
    GetWindowTextW(pDIS->hwndItem, buf, 256);
    DrawTextW(hMemDC, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hMemDC, hOldFont);

    // Blit back
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, hMemDC, 0, 0, SRCCOPY);

    // Clean
    SelectObject(hMemDC, hOldBm);
    DeleteObject(hBm);
    DeleteDC(hMemDC);
}

// ==============================
//  Save / Restore (Logic Unchanged)
// ==============================
// Helper to convert wstring to utf8 for JSON keys
static std::string WideToUtf8(const std::wstring& ws)
{
    if (ws.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), out.data(), len, nullptr, nullptr);
    return out;
}

void SaveCurrentDisplayConfig()
{
    json j;

    DISPLAY_DEVICEW dd{};
    dd.cb = sizeof(dd);

    for (int deviceIndex = 0; EnumDisplayDevicesW(NULL, deviceIndex, &dd, 0); ++deviceIndex)
    {
        if (!(dd.StateFlags & DISPLAY_DEVICE_ACTIVE))
            continue;

        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);

        if (!EnumDisplaySettingsExW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
            continue;

        std::string devUtf8 = WideToUtf8(dd.DeviceName);

        j[devUtf8] = {
            {"x", dm.dmPosition.x},
            {"y", dm.dmPosition.y},
            {"width",  dm.dmPelsWidth},
            {"height", dm.dmPelsHeight},
            {"orientation", dm.dmDisplayOrientation}
        };
    }

    fs::path cfgPath = GetConfigPath();
    std::ofstream out(cfgPath, std::ios::binary);
    if (!out.good())
    {
        std::wstring msg = L"Failed to create config:\n" + cfgPath.wstring();
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_ICONERROR);
        return;
    }

    out << j.dump(4);
    out.close();

    std::wstring ok = L"Configuration Saved:\n" + cfgPath.wstring();
    MessageBoxW(NULL, ok.c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
}

void RestoreDisplayConfig()
{
    fs::path cfgPath = GetConfigPath();

    if (!fs::exists(cfgPath))
    {
        std::wstring msg = L"Config file not found:\n" + cfgPath.wstring();
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_ICONERROR);
        return;
    }

    std::ifstream in(cfgPath, std::ios::binary);
    if (!in.good())
    {
        std::wstring msg = L"Failed to open file:\n" + cfgPath.wstring();
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_ICONERROR);
        return;
    }

    json j;
    try
    {
        in >> j;
    }
    catch (...)
    {
        std::wstring msg = L"JSON Parse Error:\n" + cfgPath.wstring();
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_ICONERROR);
        return;
    }

    for (auto& [device, cfg] : j.items())
    {
        std::wstring deviceW;
        {
            int wlen = MultiByteToWideChar(CP_UTF8, 0, device.c_str(), (int)device.size(), nullptr, 0);
            deviceW.resize(wlen);
            MultiByteToWideChar(CP_UTF8, 0, device.c_str(), (int)device.size(), deviceW.data(), wlen);
        }

        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsExW(deviceW.c_str(), ENUM_CURRENT_SETTINGS, &dm, 0))
            continue;

        dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYORIENTATION;
        dm.dmPosition.x = cfg.value("x", dm.dmPosition.x);
        dm.dmPosition.y = cfg.value("y", dm.dmPosition.y);
        dm.dmPelsWidth  = cfg.value("width",  dm.dmPelsWidth);
        dm.dmPelsHeight = cfg.value("height", dm.dmPelsHeight);
        dm.dmDisplayOrientation = cfg.value("orientation", (int)dm.dmDisplayOrientation);

        ChangeDisplaySettingsExW(deviceW.c_str(), &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
    }

    ChangeDisplaySettingsExW(NULL, NULL, NULL, 0, NULL);
    MessageBoxW(NULL, L"Configuration Restored", L"Success", MB_OK | MB_ICONINFORMATION);
}
