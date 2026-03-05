#pragma once

namespace Features {
struct MiscConfig {
  // AWP Noscope Crosshair
  bool awpCrosshair = false;
  int crosshairStyle = 0;     // 0=dot, 1=cross, 2=circle, 3=all
  float crosshairSize = 5.0f; // pixels (dot radius / cross length)
  float crosshairColor[4] = {0.0f, 1.0f, 0.0f, 0.85f}; // green
  float crosshairThickness = 1.5f;
  bool crosshairGap = true; // gap in center of cross

  int menuTheme = 0; // 0=Midnight, 1=Blood, 2=Cyber, 3=Lavender, 4=Gold,
                     // 5=Monochrome, 6=Toxic
};
} // namespace Features
