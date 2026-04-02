#pragma once

namespace Features {
struct TriggerbotConfig {
  bool enabled = false;
  int hotkey = 0x12;     // VK_MENU (Alt)
  int delayMin = 10;     // ms — minimum delay before shooting
  int delayMax = 50;     // ms — maximum delay (randomized each shot)
  bool teamCheck = true; // don't shoot at teammates
  float fov = 2.0f;      // additional FOV tolerance for hit detection
};
} // namespace Features
