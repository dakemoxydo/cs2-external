#include "esp.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/sdk/entity.h"
#include "esp_config.h"
#include "render/draw/draw_list.h"
#include "render/overlay/overlay.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <shared_mutex>
#include <string>

namespace Features {

// Bone connections defined in entity.h (uses real BoneIndex enum)
// ::s_boneConnections is the inline constexpr array from entity.h

// NOTE: Render-side smoothing has been REMOVED.
// GameManager already interpolates positions with s_interpolationFactor.
// Double smoothing caused ESP "floating" / lag behind enemy models.

void Esp::Update() {
  // Lightweight - main work done in GameManager::Update()
}

// --- Helpers
// ---------------------------------------------------------------

// Snapshot struct for ESP settings to avoid repeated Config::Settings access
struct EspSnapshot {
  bool enabled, showTeammates, showBox, showHealth, showName, showWeapon;
  bool showDistance, showBones, showSnapLines, showOffscreen;
  bool frustumCullingEnabled, showHealthText, skeletonOutline;
  BoxStyle boxStyle;
  HealthBarStyle healthBarStyle;
  float fillBoxAlpha, skeletonMaxDistance;
  float teamColor[4], boxColor[4], nameColor[4], weaponColor[4];
  float distColor[4], snapLineColor[4], boneColor[4];
  float skeletonOutlineColor[4], offscreenColor[4];
};

static EspSnapshot SnapshotEsp() {
  EspSnapshot s;
  std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
  auto &E = Config::Settings.esp;
  s = {E.enabled, E.showTeammates, E.showBox, E.showHealth, E.showName,
       E.showWeapon, E.showDistance, E.showBones, E.showSnapLines,
       E.showOffscreen, E.frustumCullingEnabled, E.showHealthText,
       E.skeletonOutline, E.boxStyle, E.healthBarStyle, E.fillBoxAlpha,
       E.skeletonMaxDistance};
  std::copy(std::begin(E.teamColor), std::end(E.teamColor), s.teamColor);
  std::copy(std::begin(E.boxColor), std::end(E.boxColor), s.boxColor);
  std::copy(std::begin(E.nameColor), std::end(E.nameColor), s.nameColor);
  std::copy(std::begin(E.weaponColor), std::end(E.weaponColor), s.weaponColor);
  std::copy(std::begin(E.distColor), std::end(E.distColor), s.distColor);
  std::copy(std::begin(E.snapLineColor), std::end(E.snapLineColor), s.snapLineColor);
  std::copy(std::begin(E.boneColor), std::end(E.boneColor), s.boneColor);
  std::copy(std::begin(E.skeletonOutlineColor), std::end(E.skeletonOutlineColor), s.skeletonOutlineColor);
  std::copy(std::begin(E.offscreenColor), std::end(E.offscreenColor), s.offscreenColor);
  return s;
}

static void DrawOffscreenIndicator(Render::DrawList &drawList,
                                    const SDK::Entity &player,
                                    const SDK::Matrix4x4 &viewMatrix,
                                    int screenWidth, int screenHeight,
                                    const EspSnapshot &s) {
  const SDK::Vector3 &pos = player.renderPosition;
  float w = viewMatrix.m[3][0] * pos.x + viewMatrix.m[3][1] * pos.y +
            viewMatrix.m[3][2] * pos.z + viewMatrix.m[3][3];
  if (w < 0.01f) return;

  float x = viewMatrix.m[0][0] * pos.x + viewMatrix.m[0][1] * pos.y +
            viewMatrix.m[0][2] * pos.z + viewMatrix.m[0][3];
  float y = viewMatrix.m[1][0] * pos.x + viewMatrix.m[1][1] * pos.y +
            viewMatrix.m[1][2] * pos.z + viewMatrix.m[1][3];

  float screenX = (screenWidth / 2.0f) * (1.0f + x / w);
  float screenY = (screenHeight / 2.0f) * (1.0f - y / w);

  float pad = 40.0f;
  float cx = std::clamp(screenX, pad, screenWidth - pad);
  float cy = std::clamp(screenY, pad, screenHeight - pad);

  float color[4];
  if (player.isTeammate) {
    color[0] = s.teamColor[0];
    color[1] = s.teamColor[1];
    color[2] = s.teamColor[2];
    color[3] = s.teamColor[3] * 0.5f;
  } else {
    color[0] = s.offscreenColor[0];
    color[1] = s.offscreenColor[1];
    color[2] = s.offscreenColor[2];
    color[3] = s.offscreenColor[3];
  }

  float angle = std::atan2(screenY - screenHeight / 2.0f, screenX - screenWidth / 2.0f);
  float size = 12.0f;
  float tipX = cx + std::cos(angle) * size;
  float tipY = cy - std::sin(angle) * size;
  float baseAngle1 = angle + 2.5f;
  float baseAngle2 = angle - 2.5f;
  float b1x = cx + std::cos(baseAngle1) * size;
  float b1y = cy - std::sin(baseAngle1) * size;
  float b2x = cx + std::cos(baseAngle2) * size;
  float b2y = cy - std::sin(baseAngle2) * size;

  drawList.DrawLine(b1x, b1y, tipX, tipY, color, 2.0f);
  drawList.DrawLine(b2x, b2y, tipX, tipY, color, 2.0f);

  char distBuf[16];
  snprintf(distBuf, sizeof(distBuf), "%.0fm", player.distance);
  drawList.AddText(cx - 10, cy + 16, distBuf, color);
}

// --- Render
// ---------------------------------------------------------------

void Esp::Render(Render::DrawList &drawList) {
  EspSnapshot s = SnapshotEsp();
  if (!s.enabled) return;

  const bool anyActive = s.showBox || s.showHealth || s.showName ||
                         s.showWeapon || s.showDistance || s.showBones ||
                         s.showSnapLines;
  if (!anyActive) return;

  const int screenWidth = Render::Overlay::GetGameWidth();
  const int screenHeight = Render::Overlay::GetGameHeight();
  if (screenWidth <= 0 || screenHeight <= 0) return;
  const auto snapshot = Core::GameManager::GetSnapshot();
  const SDK::Matrix4x4 viewMatrix = snapshot->viewMatrix;
  const auto &players = snapshot->players;

  for (const auto &player : players) {
    if (!player.IsValid() || !player.IsAlive()) continue;
    if (player.isTeammate && !s.showTeammates) continue;

    // Frustum culling
    if (s.frustumCullingEnabled && !player.onScreen) {
      if (s.showOffscreen) {
        DrawOffscreenIndicator(drawList, player, viewMatrix, screenWidth,
                               screenHeight, s);
      }
      continue;
    }

    // Pick color palette
    const float *currentBoxColor = player.isTeammate ? s.teamColor : s.boxColor;
    float drawColor[4] = {currentBoxColor[0], currentBoxColor[1],
                          currentBoxColor[2], currentBoxColor[3]};

    // World-to-screen
    SDK::Vector3 feetPos = player.renderPosition;
    SDK::Vector3 headPos = feetPos;
    headPos.z += 72.0f;

    SDK::Vector2 screenFeet, screenHead;
    if (!Core::Math::WorldToScreen(feetPos, screenFeet, viewMatrix, screenWidth,
                                   screenHeight) ||
        !Core::Math::WorldToScreen(headPos, screenHead, viewMatrix, screenWidth,
                                   screenHeight))
      continue;

    const float height = screenFeet.y - screenHead.y;
    if (height <= 0.0f) continue;
    const float width = height / 2.0f;
    const float x = screenFeet.x - width / 2.0f;
    const float y = screenHead.y;

    // Snap Lines
    if (s.showSnapLines) {
      float sc[4] = {s.snapLineColor[0], s.snapLineColor[1],
                     s.snapLineColor[2], s.snapLineColor[3]};
      drawList.DrawLine(static_cast<float>(screenWidth) / 2.0f,
                        static_cast<float>(screenHeight), screenFeet.x,
                        screenFeet.y, sc, 1.0f);
    }

    // Box
    if (s.showBox) {
      float outlineColor[4] = {0.0f, 0.0f, 0.0f, 0.85f};

      switch (s.boxStyle) {
      case BoxStyle::Corners: {
        float shadowCol[4] = {0.f, 0.f, 0.f, 0.7f};
        drawList.DrawCornerBox(x - 1, y - 1, width + 2, height + 2, shadowCol, 3.0f);
        drawList.DrawCornerBox(x, y, width, height, drawColor, 1.5f);
        break;
      }
      case BoxStyle::Filled: {
        float fillCol[4] = {drawColor[0], drawColor[1], drawColor[2], s.fillBoxAlpha};
        drawList.DrawFilledRect(x, y, width, height, fillCol);
        drawList.DrawBox(x - 1, y - 1, width + 2, height + 2, outlineColor, 1.0f);
        drawList.DrawBox(x, y, width, height, drawColor, 1.5f);
        break;
      }
      default:
        drawList.DrawBox(x - 1, y - 1, width + 2, height + 2, outlineColor, 1.0f);
        drawList.DrawBox(x, y, width, height, drawColor, 1.5f);
        drawList.DrawBox(x + 1, y + 1, width - 2, height - 2, outlineColor, 1.0f);
        break;
      }
    }

    // Health Bar
    if (s.showHealth) {
      float hpPercent = static_cast<float>(player.health) / 100.0f;
      if (hpPercent > 1.0f) hpPercent = 1.0f;
      if (hpPercent < 0.0f) hpPercent = 0.0f;
      float hpColor[4] = {1.0f - hpPercent, hpPercent, 0.0f, 1.0f};
      float bgColor[4] = {0.0f, 0.0f, 0.0f, 0.9f};

      if (s.healthBarStyle == HealthBarStyle::Bottom) {
        const float barY = y + height + 3.0f;
        const float barH = 4.0f;
        const float barW = width * hpPercent;
        drawList.DrawLine(x, barY, x + width, barY, bgColor, barH);
        drawList.DrawLine(x, barY, x + barW, barY, hpColor, barH - 1.0f);
      } else {
        const float hpHeight = height * hpPercent;
        drawList.DrawLine(x - 6, y, x - 6, y + height, bgColor, 3.0f);
        drawList.DrawLine(x - 6, y + height - hpHeight, x - 6, y + height,
                          hpColor, 2.0f);
      }

      if (s.showHealthText) {
        std::string hpStr = std::to_string(player.health) + " HP";
        float textCol[4] = {hpColor[0], hpColor[1], hpColor[2], 1.0f};
        if (s.healthBarStyle == HealthBarStyle::Bottom)
          drawList.AddText(x, y + height + 9.0f, hpStr.c_str(), textCol);
        else
          drawList.AddText(x - 22.0f, y, hpStr.c_str(), textCol);
      }
    }

    // Name
    if (s.showName) {
      float nCol[4] = {s.nameColor[0], s.nameColor[1],
                       s.nameColor[2], s.nameColor[3]};
      std::string drawName = player.name.empty() ? "Player" : player.name;
      drawList.AddText(x + width / 2.0f - 15.0f, y - 15.0f, drawName.c_str(), nCol);
    }

    // Weapon
    if (s.showWeapon && !player.weapon.empty()) {
      float wCol[4] = {s.weaponColor[0], s.weaponColor[1],
                       s.weaponColor[2], s.weaponColor[3]};
      drawList.AddText(x + width / 2.0f - 15.0f, y + height + 2.0f,
                       player.weapon.c_str(), wCol);
    }

    // Distance
    if (s.showDistance && player.distance > 0.0f) {
      float dCol[4] = {s.distColor[0], s.distColor[1],
                       s.distColor[2], s.distColor[3]};
      std::string distStr = std::to_string(static_cast<int>(player.distance)) + "m";
      drawList.AddText(x + width - 5.0f, y + height + 2.0f, distStr.c_str(), dCol);
    }

    // Skeleton & Head Circle
    if (s.showBones && !player.bonePositions.empty() &&
        (s.skeletonMaxDistance <= 0.0f || player.distance <= s.skeletonMaxDistance)) {

      float boneCol[4] = {s.boneColor[0], s.boneColor[1],
                          s.boneColor[2], s.boneColor[3]};
      float shadowC[4] = {s.skeletonOutlineColor[0], s.skeletonOutlineColor[1],
                          s.skeletonOutlineColor[2], s.skeletonOutlineColor[3]};

      for (const auto &conn : ::s_boneConnections) {
        int idxA = conn[0];
        int idxB = conn[1];
        if (idxA >= (int)player.bonePositions.size() ||
            idxB >= (int)player.bonePositions.size())
          continue;
        SDK::Vector2 rawA, rawB;
        bool okA = Core::Math::WorldToScreen(player.bonePositions[idxA], rawA,
                                             viewMatrix, screenWidth, screenHeight);
        bool okB = Core::Math::WorldToScreen(player.bonePositions[idxB], rawB,
                                             viewMatrix, screenWidth, screenHeight);
        if (okA && okB) {
          if (s.skeletonOutline)
            drawList.DrawLine(rawA.x, rawA.y, rawB.x, rawB.y, shadowC, 2.8f);
          drawList.DrawLine(rawA.x, rawA.y, rawB.x, rawB.y, boneCol, 1.2f);
        }
      }

      if ((int)player.bonePositions.size() > BONE_HEAD) {
        SDK::Vector2 screenBoneHead;
        if (Core::Math::WorldToScreen(player.bonePositions[BONE_HEAD], screenBoneHead,
                                      viewMatrix, screenWidth, screenHeight)) {
          const float headRadius = width * 0.18f;
          if (s.skeletonOutline)
            drawList.DrawCircle(screenBoneHead.x, screenBoneHead.y, headRadius + 1.0f,
                                shadowC, 28, 2.5f);
          drawList.DrawCircle(screenBoneHead.x, screenBoneHead.y, headRadius, boneCol, 28, 1.2f);
        }
      }
    }
  }
}

void Esp::RenderUI() {}

} // namespace Features
