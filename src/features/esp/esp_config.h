#pragma once

namespace Features {

// Box drawing style
enum class BoxStyle : int {
  Rect = 0,    // full rectangle (default)
  Corners = 1, // corner brackets only
  Filled = 2,  // semi-transparent filled rect
};

// Health bar orientation
enum class HealthBarStyle : int {
  Side = 0,   // vertical bar on the left (default)
  Bottom = 1, // horizontal bar below the box
};

struct EspConfig {
  bool enabled = false;
  bool showTeammates = false;

  // ── Box ──────────────────────────────────────────────────────────────
  bool showBox = true;
  BoxStyle boxStyle = BoxStyle::Rect;
  float fillBoxAlpha = 0.15f; // used when boxStyle == Filled
  float boxColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  float teamColor[4] = {0.0f, 0.5f, 1.0f, 1.0f};

  // ── Health ───────────────────────────────────────────────────────────
  bool showHealth = true;
  HealthBarStyle healthBarStyle = HealthBarStyle::Side;
  bool showHealthText = false;

  // ── Labels ───────────────────────────────────────────────────────────
  bool showName = true;
  float nameColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  bool showWeapon = true;
  float weaponColor[4] = {1.0f, 0.85f, 0.0f, 1.0f};

  bool showDistance = false;
  float distColor[4] = {0.7f, 0.7f, 0.7f, 1.0f};

  // ── Skeleton ─────────────────────────────────────────────────────────
  bool showBones = false;
  bool skeletonOutline = false;
  float skeletonOutlineColor[4] = {0.0f, 0.0f, 0.0f, 0.75f};
  float skeletonMaxDistance = 25.0f; // meters
  float boneColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  // ── Snap Lines ───────────────────────────────────────────────────────
  bool showSnapLines = false;
  float snapLineColor[4] = {1.0f, 1.0f, 1.0f, 0.4f};
};

} // namespace Features
