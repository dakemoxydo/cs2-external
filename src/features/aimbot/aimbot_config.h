#pragma once

namespace Features {
struct AimbotConfig {
  float fov = 5.0f;
  float smoothing = 10.0f;
  int targetBone = 6; // HEAD
  int hotkey = 0x01;  // VK_LBUTTON
};
} // namespace Features
