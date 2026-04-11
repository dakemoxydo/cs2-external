#include "overlay.h"
#include "../../core/process/process.h"
#include <windows.h>
#include <dwmapi.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>

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
static const char* s_currentClassName = nullptr;
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
    SetProcessDPIAware();

    if (hwnd) {
        return true;
    }

    if (!FindCS2Window()) {
        std::cout << "[-] Cannot create overlay: CS2 window not found!\n";
        return false;
    }

    static const char* classNames[] = {"CEF-OSC-Widget", "DiscordOverlay", "NVIDIA GeForce Overlay", "Steam Overlay"};
    static const char* windowNames[] = {"CEF-OSC-Widget", "Discord", "NVIDIA GeForce Overlay", "Steam"};

    if (!s_currentClassName) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dist(0, 3);
        s_currentClassName = classNames[dist(gen)];
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
        s_currentClassName,
        nullptr};
    if (!RegisterClassExA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    s_classRegistered = true;

    RECT rc;
    GetWindowRect(hwndCS2, &rc);
    int overlayX = rc.left;
    int overlayY = rc.top;
    int overlayW = rc.right - rc.left;
    int overlayH = rc.bottom - rc.top;

    const char* currentWinName = "CS2 External";
    for (int i = 0; i < 4; i++) {
        if (classNames[i] == s_currentClassName) {
            currentWinName = windowNames[i];
            break;
        }
    }

    hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
                           wc.lpszClassName, currentWinName, WS_POPUP,
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

    RECT rc;
    if (!GetWindowRect(hwndCS2, &rc)) return false;

    int curW = rc.right - rc.left;
    int curH = rc.bottom - rc.top;
    int curX = rc.left;
    int curY = rc.top;

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

  if (s_classRegistered && s_currentClassName) {
      UnregisterClassA(s_currentClassName, GetModuleHandle(nullptr));
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
