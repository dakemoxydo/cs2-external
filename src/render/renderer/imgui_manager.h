#pragma once
struct ImFont;

namespace Render {
class ImGuiManager {
public:
  static bool Init();
  static void Shutdown();

  static void NewFrame();
  static void Render();
  static ImFont *GetRegularFont();
  static ImFont *GetSemiboldFont();
};
} // namespace Render
