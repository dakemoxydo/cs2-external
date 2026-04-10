#pragma once

namespace Render {

class Menu {
public:
  static void Render();
  static bool IsOpen();
  static bool ShouldClose();
  static void Toggle();

  static bool isOpen;
  static bool shouldClose;
};

} // namespace Render
