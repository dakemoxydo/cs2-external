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

bool Overlay::Create(int /*width*/, int /*height*/) {
  SetProcessDPIAware();

  WNDCLASSEXA wc = {sizeof(WNDCLASSEXA),
                    CS_CLASSDC,
                    WndProc,
                    0L,
                    0L,
                    GetModuleHandle(nullptr),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    "CS2 Overlay",
                    nullptr};
  RegisterClassExA(&wc);

  int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
                         wc.lpszClassName, "CS2 External Overlay", WS_POPUP,
                         screenX, screenY, screenW, screenH, nullptr, nullptr,
                         wc.hInstance, nullptr);

  if (!hwnd)
    return false;

  SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

  MARGINS margins = {-1};
  DwmExtendFrameIntoClientArea(hwnd, &margins);

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  std::cout << "[DEBUG] Transparent Win32 overlay created (width: " << screenW
            << ", height: " << screenH << ")." << std::endl;
  return true;
}

void Overlay::Destroy() {
  if (hwnd) {
    DestroyWindow(hwnd);
    UnregisterClass("CS2 Overlay", GetModuleHandle(nullptr));
    hwnd = nullptr;
    std::cout << "[DEBUG] Win32 overlay destroyed." << std::endl;
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
