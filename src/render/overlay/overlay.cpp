#include "overlay.h"
#include "../../core/process/process.h"
#include <windows.h>
#include <dwmapi.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace Render {
HWND Overlay::hwnd = nullptr;
HWND Overlay::hwndCS2 = nullptr;
int Overlay::gameWidth = 0;
int Overlay::gameHeight = 0;
static constexpr const char* kOverlayClassName = "CS2OverlayWindow";
static constexpr const char* kOverlayWindowName = "CS2 Overlay";
static bool s_classRegistered = false;

bool Overlay::FindCS2Window() {
    // Primary: find window by PID (reliable across locales/modes)
    DWORD targetPid = Core::Process::GetProcessId();
    if (targetPid != 0) {
        struct EnumData {
            HWND hwnd;
            RECT rc;
            DWORD targetPid;
        };
        EnumData data = { nullptr, {0,0,0,0}, targetPid };

        EnumWindows([](HWND hEnum, LPARAM lParam) -> BOOL {
            EnumData* pData = reinterpret_cast<EnumData*>(lParam);
            DWORD winPid = 0;
            GetWindowThreadProcessId(hEnum, &winPid);
            if (winPid == pData->targetPid) {
                if (IsWindowVisible(hEnum) && GetWindowRect(hEnum, &pData->rc)) {
                    pData->hwnd = hEnum;
                    return FALSE;
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&data));

        if (data.hwnd) {
            hwndCS2 = data.hwnd;
            gameWidth = data.rc.right - data.rc.left;
            gameHeight = data.rc.bottom - data.rc.top;
            char msg[256];
            snprintf(msg, sizeof(msg), "[+] CS2 window found (PID match): %dx%d at (%d,%d)\n",
                     gameWidth, gameHeight, data.rc.left, data.rc.top);
            std::cout << msg;
            return true;
        }
    }

    // Fallback: title-based search
    const char* windowTitles[] = {
        "Counter-Strike 2",
        "Counter-Strike 2 (",
        "CS2"
    };

    for (const char* title : windowTitles) {
        hwndCS2 = FindWindowA(nullptr, title);
        if (hwndCS2) {
            RECT rc;
            if (GetWindowRect(hwndCS2, &rc)) {
                gameWidth = rc.right - rc.left;
                gameHeight = rc.bottom - rc.top;
                char msg[256];
                snprintf(msg, sizeof(msg), "[+] CS2 window found (title): %dx%d at (%d,%d)\n",
                         gameWidth, gameHeight, rc.left, rc.top);
                std::cout << msg;
                return true;
            }
        }
    }

    // Last resort: partial title match
    struct EnumData2 {
        HWND hwnd;
        RECT rc;
    };
    EnumData2 data2 = { nullptr, {0, 0, 0, 0} };

    EnumWindows([](HWND hEnum, LPARAM lParam) -> BOOL {
        EnumData2* pData = reinterpret_cast<EnumData2*>(lParam);
        char title[256];
        if (GetWindowTextA(hEnum, title, sizeof(title)) > 0) {
            if (strstr(title, "Counter-Strike") != nullptr) {
                pData->hwnd = hEnum;
                GetWindowRect(hEnum, &pData->rc);
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data2));

    if (data2.hwnd) {
        hwndCS2 = data2.hwnd;
        gameWidth = data2.rc.right - data2.rc.left;
        gameHeight = data2.rc.bottom - data2.rc.top;
        char msg[256];
        snprintf(msg, sizeof(msg), "[+] CS2 window found (partial): %dx%d at (%d,%d)\n",
                 gameWidth, gameHeight, data2.rc.left, data2.rc.top);
        std::cout << msg;
        return true;
    }

    std::cout << "[-] CS2 window not found!\n";
    return false;
}

bool Overlay::Create() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (hwnd) {
        return true;
    }

    if (!FindCS2Window()) {
        std::cout << "[-] Cannot create overlay: CS2 window not found!\n";
        return false;
    }

    WNDCLASSEXA wc = {
        sizeof(WNDCLASSEXA),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        kOverlayClassName,
        nullptr};
    if (!RegisterClassExA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    s_classRegistered = true;

    RECT clientRect{};
    GetClientRect(hwndCS2, &clientRect);
    POINT origin{clientRect.left, clientRect.top};
    ClientToScreen(hwndCS2, &origin);
    int overlayX = origin.x;
    int overlayY = origin.y;
    int overlayW = clientRect.right - clientRect.left;
    int overlayH = clientRect.bottom - clientRect.top;
    if (overlayW <= 0 || overlayH <= 0) {
        UnregisterClassA(kOverlayClassName, wc.hInstance);
        s_classRegistered = false;
        return false;
    }

    hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
                           wc.lpszClassName, kOverlayWindowName, WS_POPUP,
                           overlayX, overlayY, overlayW, overlayH, nullptr, nullptr,
                           wc.hInstance, nullptr);

    if (!hwnd) {
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return false;
    }

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    gameWidth = overlayW;
    gameHeight = overlayH;

    char msg[256];
    snprintf(msg, sizeof(msg), "[+] Overlay created at (%d,%d) with size %dx%d\n",
             overlayX, overlayY, overlayW, overlayH);
    std::cout << msg;

    return true;
}

bool Overlay::UpdatePosition() {
    if (!hwnd || !hwndCS2) return false;

    if (!IsWindow(hwndCS2)) {
        if (!FindCS2Window()) return false;
    }

    RECT clientRect{};
    if (!GetClientRect(hwndCS2, &clientRect)) return false;
    POINT origin{clientRect.left, clientRect.top};
    ClientToScreen(hwndCS2, &origin);

    int curW = clientRect.right - clientRect.left;
    int curH = clientRect.bottom - clientRect.top;
    int curX = origin.x;
    int curY = origin.y;
    if (curW <= 0 || curH <= 0) return false;

    static int lastX = curX;
    static int lastY = curY;
    static int lastW = curW;
    static int lastH = curH;

    const bool sizeChanged = curW != lastW || curH != lastH;
    const bool positionChanged = curX != lastX || curY != lastY;
    if (sizeChanged || positionChanged) {
        MoveWindow(hwnd, curX, curY, curW, curH, TRUE);
        gameWidth = curW;
        gameHeight = curH;
        lastX = curX;
        lastY = curY;
        lastW = curW;
        lastH = curH;
        return sizeChanged;
    }

    return false;
}

int Overlay::GetGameWidth() {
    return gameWidth;
}

int Overlay::GetGameHeight() {
    return gameHeight;
}

void Overlay::Destroy() {
  if (hwnd) {
    DestroyWindow(hwnd);
    hwnd = nullptr;
  }

  if (s_classRegistered) {
      UnregisterClassA(kOverlayClassName, GetModuleHandle(nullptr));
      s_classRegistered = false;
  }

  hwndCS2 = nullptr;
  gameWidth = 0;
  gameHeight = 0;
}

HWND Overlay::GetWindowHandle() { return hwnd; }

LRESULT CALLBACK Overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return 0;

  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}
} // namespace Render
