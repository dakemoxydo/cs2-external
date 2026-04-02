#pragma once

namespace Render {
class Menu {
public:
  static void Render();
  static bool IsOpen();
  static void Toggle();
  static bool ShouldClose();

private:
  static bool isOpen;
  static bool shouldClose;
};
} // namespace Render
