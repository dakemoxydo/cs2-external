#pragma once

namespace Features {

struct RCSConfig {
  bool enabled = false;
  int key = 0x01;
  float pitchStrength = 1.0f;
  float yawStrength = 1.0f;
  float smooth = 8.0f;
  int startBullet = 2;
};

} // namespace Features
