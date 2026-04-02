#pragma once

namespace Features {

struct RadarConfig {
  bool enabled = false;

  // ── Display ───────────────────────────────────────────────────────────
  bool rotate = true;        // rotate map by local player yaw
  bool showTeammates = true; // show teammate dots
  float bgAlpha = 0.63f;     // background circle alpha (0 = transparent window)
  float zoom = 0.50f;        // map zoom factor (higher = see further)
  float pointSize = 4.0f;    // dot radius in pixels

  // ── Overlay alignment (Old Version parity) ─────────────
  int mapIndex =
      0; // 0=Custom, 1=1.88, 2=2.32, etc. (Mirage, Dust2... from old code)
  float mapCalibration = 1.0f; // custom scale
  int stretchType = 0;         // 0=None, 1=16:9/4:3, 2=16:9/16:10, 3=16:9/5:4

  // ── Visible check ─────────────────────────────────────────────────────
  bool visibleCheck = true;                         // color by isSpotted flag
  float visibleColor[4] = {0.2f, 1.0f, 0.2f, 1.0f}; // enemy in LOS
  float hiddenColor[4] = {1.0f, 0.2f, 0.2f, 1.0f};  // enemy out of LOS
  float enemyColor[4] = {1.0f, 0.2f, 0.2f, 1.0f};   // enemy (no vis check)
  float teamColor[4] = {0.2f, 0.6f, 1.0f, 1.0f};    // teammate
};

} // namespace Features
