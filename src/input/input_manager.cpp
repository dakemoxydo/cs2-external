#include "input_manager.h"

namespace Input {
bool InputManager::keyStates[256] = {};
bool InputManager::prevKeyStates[256] = {};

void InputManager::Poll() {}

bool InputManager::IsKeyDown(int virtualKey) { return false; }

bool InputManager::IsKeyPressed(int virtualKey) { return false; }
} // namespace Input
