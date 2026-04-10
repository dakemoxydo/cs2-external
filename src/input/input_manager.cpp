#include "input_manager.h"
#include <windows.h>
namespace Input {
bool InputManager::keyStates[256] = {};
bool InputManager::prevKeyStates[256] = {};

namespace {
bool IsValidVirtualKey(int virtualKey) {
  return virtualKey >= 0 && virtualKey < 256;
}
}

void InputManager::Poll() {
  for (int i = 0; i < 256; i++) {
    prevKeyStates[i] = keyStates[i];
    keyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
  }
}

bool InputManager::IsKeyDown(int virtualKey) {
  if (!IsValidVirtualKey(virtualKey)) {
    return false;
  }
  return keyStates[virtualKey];
}

bool InputManager::IsKeyPressed(int virtualKey) {
  if (!IsValidVirtualKey(virtualKey)) {
    return false;
  }
  return keyStates[virtualKey] && !prevKeyStates[virtualKey];
}

void InputManager::SendMouseDelta(int dx, int dy) {
  INPUT inp = {};
  inp.type = INPUT_MOUSE;
  inp.mi.dx = dx;
  inp.mi.dy = dy;
  inp.mi.dwFlags = MOUSEEVENTF_MOVE;
  SendInput(1, &inp, sizeof(INPUT));
}

void InputManager::SendMouseClick(bool down) {
  INPUT inp = {};
  inp.type = INPUT_MOUSE;
  inp.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
  SendInput(1, &inp, sizeof(INPUT));
}
} // namespace Input
