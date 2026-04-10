#pragma once
#include "../../core/sdk/entity.h"

namespace Features {

// Bone target selector displayed in menu
enum class AimBone : int {
  Head = BONE_HEAD,     // 6
  Neck = BONE_NECK,     // 5
  Chest = BONE_SPINE_1, // 4
  Pelvis = BONE_PELVIS, // 0
};

struct AimbotConfig {
  bool enabled = false;
  int hotkey = 0x01; // VK_LBUTTON — hold to aim
  int targetBone = static_cast<int>(AimBone::Head);
  float fov = 5.0f;         // degrees, FOV cone for target lock
  float smooth = 8.0f;      // 1=instant, 20=very slow
  float jitter = 0.025f;    // random noise in degrees (0 = off)
  float sensitivity = 2.0f; // CS2 mouse sensitivity (matches in-game setting)
  bool teamCheck = true;    // don't aim at teammates
  bool onlyScoped = false;  // trigger only when scoped (AWP mode)
  bool targetLock = true;   // stay on first acquired target while holding key
  bool visibleOnly = false; // skip enemies not spotted (isSpotted)
  bool showFov = true;      // draw FOV circle on screen
};

} // namespace Features
