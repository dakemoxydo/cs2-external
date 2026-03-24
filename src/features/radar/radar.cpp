#include "radar.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "radar_config.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace Features {


inline float GetCurrentMapScale() {
  if (Config::Settings.radar.mapIndex == 1)
    return 1.88f;
  if (Config::Settings.radar.mapIndex == 2)
    return 2.32f;
  if (Config::Settings.radar.mapIndex == 3)
    return 1.44f;
  if (Config::Settings.radar.mapIndex == 4)
    return 1.99f;
  return Config::Settings.radar.mapCalibration;
}

void Radar::Update() {
  // Logic handled in GameManager (local position, spotted flags, etc)
}

void Radar::Render(Render::DrawList &drawList) {
  if (!Config::Settings.radar.enabled)
    return;

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
  if (!Config::Settings.menuIsOpen) {
    flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize;
  }

  ImGui::SetNextWindowSize({250, 250}, ImGuiCond_FirstUseEver);

  float currentAlpha = Config::Settings.menuIsOpen
                           ? std::fmaxf(0.2f, Config::Settings.radar.bgAlpha)
                           : Config::Settings.radar.bgAlpha;
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(0.08f, 0.08f, 0.08f, currentAlpha));

  if (ImGui::Begin("Radar Overlay", nullptr, flags)) {
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 center = {pos.x + size.x / 2.0f, pos.y + size.y / 2.0f};

    float radius = (std::fminf(size.x, size.y) / 2.0f);

    float stretchX = 1.0f;
    if (Config::Settings.radar.stretchType == 1)
      stretchX = (16.0f / 9.0f) / (4.0f / 3.0f);
    else if (Config::Settings.radar.stretchType == 2)
      stretchX = (16.0f / 9.0f) / (16.0f / 10.0f);
    else if (Config::Settings.radar.stretchType == 3)
      stretchX = (16.0f / 9.0f) / (5.0f / 4.0f);

    // Background circle (from old code)
    if (Config::Settings.radar.bgAlpha > 0.01f &&
        !Config::Settings.menuIsOpen) {
      ImVec2 pts[64];
      for (int i = 0; i < 64; i++) {
        float a = (float)i / 64.0f * 3.14159265f * 2.0f;
        // Old code applies stretchX *only* to the polygon background too!
        pts[i] = {center.x + std::cosf(a) * radius * stretchX,
                  center.y + std::sinf(a) * radius};
      }
      dl->AddConvexPolyFilled(
          pts, 64,
          IM_COL32(15, 15, 15, (int)(Config::Settings.radar.bgAlpha * 255)));
    }

    // Draw local player
    dl->AddCircleFilled(center, Config::Settings.radar.pointSize + 1.0f,
                        IM_COL32(0, 0, 0, 255));
    dl->AddCircleFilled(center, Config::Settings.radar.pointSize,
                        IM_COL32(255, 255, 255, 255));

    auto players = Core::GameManager::GetRenderPlayers();
    SDK::Vector3 localPos = Core::GameManager::GetLocalPos();
    float localYaw = Core::GameManager::GetLocalAngles().y;
    int localTeam = Core::GameManager::GetLocalTeam();

    // 90 is top of radar
    float angleRad = (90.0f - localYaw) * (3.14159265f / 180.0f);
    float s = std::sinf(angleRad);
    float c = std::cosf(angleRad);

    float mapScale = GetCurrentMapScale();
    float baseScale = (Config::Settings.radar.zoom * mapScale * 7.7f) / 100.0f;
    float windowRatio = radius / 150.0f;

    for (const auto &ent : players) {
      if (!ent.IsValid() || !ent.IsAlive())
        continue;
      if (!Config::Settings.radar.showTeammates && ent.team == localTeam)
        continue;

      float dx = ent.position.x - localPos.x;
      float dy = ent.position.y - localPos.y;

      float final_x = dx * baseScale * windowRatio;
      float final_y = dy * baseScale * windowRatio;

      if (Config::Settings.radar.rotate) {
        float rot_x = final_x * c - final_y * s;
        float rot_y = final_x * s + final_y * c;
        final_x = rot_x;
        final_y = rot_y;
      }

      float dist = std::sqrtf(final_x * final_x + final_y * final_y);
      float maxDist = radius - Config::Settings.radar.pointSize;

      bool isClamped = false;
      if (dist > maxDist) {
        final_x = (final_x / dist) * maxDist;
        final_y = (final_y / dist) * maxDist;
        isClamped = true;
      }

      final_x *= stretchX;

      ImVec2 dotPos = {center.x + final_x, center.y - final_y};

      float *colSrc;
      if (ent.team == localTeam) {
        colSrc = Config::Settings.radar.teamColor;
      } else {
        if (Config::Settings.radar.visibleCheck) {
          colSrc = ent.isSpotted ? Config::Settings.radar.visibleColor
                                 : Config::Settings.radar.hiddenColor;
        } else {
          colSrc = Config::Settings.radar.enemyColor;
        }
      }

      float dotSize = isClamped ? Config::Settings.radar.pointSize * 0.7f
                                : Config::Settings.radar.pointSize;
      int alpha = isClamped ? 180 : 255;
      ImU32 finalColor = ImGui::ColorConvertFloat4ToU32(
          ImVec4(colSrc[0], colSrc[1], colSrc[2], colSrc[3])); // Get base color
      finalColor = (finalColor & 0x00FFFFFF) | (alpha << 24);

      dl->AddCircleFilled(dotPos, dotSize + 1.0f, IM_COL32(0, 0, 0, alpha));
      dl->AddCircleFilled(dotPos, dotSize, finalColor);
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
}
} // namespace Features
