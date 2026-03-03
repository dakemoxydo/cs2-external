#pragma once

namespace Input {
class InputManager {
public:
  static void Poll();
  static bool IsKeyDown(int virtualKey);
  static bool IsKeyPressed(int virtualKey);

private:
  static bool keyStates[256];
  static bool prevKeyStates[256];
};
} // namespace Input
