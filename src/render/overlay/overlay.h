#pragma once
#include <windows.h>

// Forward declaration for ImGui's Win32 handler
#include <imgui.h>

namespace Render {
class Overlay {
public:
  static bool Create(int width, int height);
  static void Destroy();
  static HWND GetWindowHandle();

private:
  static HWND hwnd;
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam);
};
} // namespace Render
