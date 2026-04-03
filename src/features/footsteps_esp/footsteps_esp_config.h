#pragma once

namespace Features {

struct FootstepsEspConfig {
  bool enabled = false;
  bool showTeammates = false;

  float footstepColor[4] = {1.0f, 0.8f, 0.0f, 0.6f};
  float jumpColor[4] = {0.0f, 0.8f, 1.0f, 0.6f};
  float landColor[4] = {1.0f, 0.3f, 0.0f, 0.7f};

  float footstepMaxRadius = 25.0f;
  float jumpMaxRadius = 40.0f;
  float landMaxRadius = 55.0f;

  float expandDuration = 0.5f;
  float fadeDuration = 1.0f;

  float thickness = 2.0f;
  int segments = 32;
};

} // namespace Features
