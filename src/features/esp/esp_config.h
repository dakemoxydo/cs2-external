#pragma once

namespace Features {
struct EspConfig {
  bool enabled = false;
  bool showBox = true;
  bool showHealth = true;
  bool showName = true;
  bool showWeapon = true;
  bool showDistance = false;
  bool showBones = false;
  float skeletonMaxDistance = 25.0f; // meters — don't render bones beyond this
  bool showTeammates = false;

  float boxColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  float teamColor[4] = {0.0f, 0.5f, 1.0f, 1.0f};
  float boneColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};
} // namespace Features
