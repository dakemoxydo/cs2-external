#pragma once

namespace Render {
class ImGuiManager {
public:
  static bool Init();
  static void Shutdown();

  static void NewFrame();
  static void Render();
};
} // namespace Render
