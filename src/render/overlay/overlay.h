#pragma once
#include <windows.h>

// Forward declaration for ImGui's Win32 handler
#include <imgui.h>

namespace Render {
class Overlay {
public:
  static bool Create();
  static void Destroy();
  static HWND GetWindowHandle();
  static void UpdatePosition();
  static int GetGameWidth();
  static int GetGameHeight();

private:
  static HWND hwnd;
  static HWND hwndCS2;
  static int gameWidth;
  static int gameHeight;
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam);
  static bool FindCS2Window();
};
} // namespace Render
