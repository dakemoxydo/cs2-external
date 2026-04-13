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
  bool showBulletTracers = false;
  float bulletTracerColor[4] = {0.30f, 0.74f, 1.00f, 0.90f};
  float bulletTracerThickness = 2.0f;
  float bulletTracerLife = 0.85f;
  float bulletTracerImpactColor[4] = {0.30f, 0.74f, 1.00f, 0.85f};
  float bulletTracerImpactRadius = 7.0f;
  float bulletTracerImpactThickness = 2.0f;
  bool showHitmarker = false;
  float hitmarkerColor[4] = {1.0f, 1.0f, 1.0f, 0.95f};
  float hitmarkerLife = 0.20f;

  // ── Off-screen ───────────────────────────────────────────────────────
  bool showOffscreen = false;
  float offscreenColor[4] = {1.0f, 0.5f, 0.0f, 0.6f};

  // ── Frustum Culling ──────────────────────────────────────────────────
  bool frustumCullingEnabled = true;
};

} // namespace Features
