#include "input_manager.h"
#include <windows.h>
namespace Input {
bool InputManager::keyStates[256] = {};
bool InputManager::prevKeyStates[256] = {};

void InputManager::Poll() {}

bool InputManager::IsKeyDown(int virtualKey) { return false; }

bool InputManager::IsKeyPressed(int virtualKey) { return false; }

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
