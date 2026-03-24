#pragma once

namespace Features {

struct RCSConfig {
  bool enabled = false;
  float pitchStrength = 1.0f; // 0.0 to 2.0
  float yawStrength = 1.0f;   // 0.0 to 2.0
  int startBullet = 1;        // Start RCS perfectly from this bullet
};

} // namespace Features
