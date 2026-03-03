#pragma once

namespace Features {
struct RadarConfig {
  float size = 150.0f;
  float zoom = 1.0f;
  float backgroundColor[4] = {0.1f, 0.1f, 0.1f, 0.7f};
  float enemyColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
};
} // namespace Features
