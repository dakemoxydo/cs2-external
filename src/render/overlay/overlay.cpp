#include "overlay.h"
#include <dwmapi.h>
#include <iostream>

#pragma comment(lib, "dwmapi.lib")

#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace Render {
HWND Overlay::hwnd = nullptr;
static const char* s_currentClassName = nullptr;

bool Overlay::Create(int /*width*/, int /*height*/) {
  SetProcessDPIAware();

  static const char* classNames[] = {"CEF-OSC-Widget", "DiscordOverlay", "NVIDIA GeForce Overlay", "Steam Overlay"};
  static const char* windowNames[] = {"CEF-OSC-Widget", "Discord", "NVIDIA GeForce Overlay", "Steam"};
  
  if (!s_currentClassName) {
      srand(GetTickCount());
      int randIdx = rand() % 4;
      s_currentClassName = classNames[randIdx];
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
      s_currentClassName, // Masquerade as Chrome/Discord/Nvidia overlay
      nullptr};
  RegisterClassExA(&wc);

  int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  // Use the corresponding window name randomly chosen above
  const char* currentWinName = "CS2 External"; // fallback
  for (int i = 0; i < 4; i++) {
      if (classNames[i] == s_currentClassName) {
          currentWinName = windowNames[i];
          break;
      }
  }

  hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
                         wc.lpszClassName, currentWinName, WS_POPUP,
                         screenX, screenY, screenW, screenH, nullptr, nullptr,
                         wc.hInstance, nullptr);

  if (!hwnd)
    return false;

  SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

  MARGINS margins = {-1};
  DwmExtendFrameIntoClientArea(hwnd, &margins);

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  return true;
}

void Overlay::Destroy() {
  if (hwnd) {
    DestroyWindow(hwnd);
    if (s_currentClassName) {
        UnregisterClassA(s_currentClassName, GetModuleHandle(nullptr));
    }
    hwnd = nullptr;
  }
}

HWND Overlay::GetWindowHandle() { return hwnd; }

LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    return true;

  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}
} // namespace Render
