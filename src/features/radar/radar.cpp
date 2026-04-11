#include "radar.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "radar_config.h"
#include "render/menu/menu.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <shared_mutex>

namespace Features {

struct RadarSnapshot {
  bool enabled, rotate, showTeammates, visibleCheck;
  int mapIndex, stretchType;
  float bgAlpha, zoom, pointSize, mapCalibration;
  float visibleColor[4], hiddenColor[4], enemyColor[4], teamColor[4];
};

static RadarSnapshot SnapshotRadar() {
  RadarSnapshot s;
  std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
  auto &R = Config::Settings.radar;
  s = {R.enabled, R.rotate, R.showTeammates, R.visibleCheck,
       R.mapIndex, R.stretchType, R.bgAlpha, R.zoom, R.pointSize,
       R.mapCalibration};
  std::copy(std::begin(R.visibleColor), std::end(R.visibleColor), s.visibleColor);
  std::copy(std::begin(R.hiddenColor), std::end(R.hiddenColor), s.hiddenColor);
  std::copy(std::begin(R.enemyColor), std::end(R.enemyColor), s.enemyColor);
  std::copy(std::begin(R.teamColor), std::end(R.teamColor), s.teamColor);
  return s;
}

inline float GetMapScaleFromSnapshot(const RadarSnapshot &s) {
  if (s.mapIndex == 1) return 1.88f;
  if (s.mapIndex == 2) return 2.32f;
  if (s.mapIndex == 3) return 1.44f;
  if (s.mapIndex == 4) return 1.99f;
  return s.mapCalibration;
}

void Radar::Update() {
  // Logic handled in GameManager (local position, spotted flags, etc)
}

void Radar::Render(Render::DrawList &) {
  RadarSnapshot s = SnapshotRadar();
  if (!s.enabled) return;
  const bool menuIsOpen = Render::Menu::IsOpen();

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
  if (!menuIsOpen) {
    flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize;
  }

  ImGui::SetNextWindowSize({250, 250}, ImGuiCond_FirstUseEver);

  float currentAlpha = menuIsOpen
                           ? std::fmaxf(0.2f, s.bgAlpha)
                           : s.bgAlpha;
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(0.08f, 0.08f, 0.08f, currentAlpha));

  if (ImGui::Begin("Radar Overlay", nullptr, flags)) {
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 center = {pos.x + size.x / 2.0f, pos.y + size.y / 2.0f};

    float radius = (std::fminf(size.x, size.y) / 2.0f);

    float stretchX = 1.0f;
    if (s.stretchType == 1)
      stretchX = (16.0f / 9.0f) / (4.0f / 3.0f);
    else if (s.stretchType == 2)
      stretchX = (16.0f / 9.0f) / (16.0f / 10.0f);
    else if (s.stretchType == 3)
      stretchX = (16.0f / 9.0f) / (5.0f / 4.0f);

    // Background circle
    if (s.bgAlpha > 0.01f && !menuIsOpen) {
      ImVec2 pts[64];
      for (int i = 0; i < 64; i++) {
        float a = (float)i / 64.0f * 3.14159265f * 2.0f;
        pts[i] = {center.x + std::cosf(a) * radius * stretchX,
                  center.y + std::sinf(a) * radius};
      }
      dl->AddConvexPolyFilled(pts, 64,
          IM_COL32(15, 15, 15, (int)(s.bgAlpha * 255)));
    }

    // Local player
    dl->AddCircleFilled(center, s.pointSize + 1.0f, IM_COL32(0, 0, 0, 255));
    dl->AddCircleFilled(center, s.pointSize, IM_COL32(255, 255, 255, 255));

    const auto snapshot = Core::GameManager::GetSnapshot();
    const auto &players = snapshot->players;
    SDK::Vector3 localPos = snapshot->localPos;
    float localYaw = snapshot->localAngles.y;
    int localTeam = snapshot->localTeam;

    float angleRad = (90.0f - localYaw) * (3.14159265f / 180.0f);
    float sn = std::sinf(angleRad);
    float c = std::cosf(angleRad);

    float mapScale = GetMapScaleFromSnapshot(s);
    float baseScale = (s.zoom * mapScale * 7.7f) / 100.0f;
    float windowRatio = radius / 150.0f;

    for (const auto &ent : players) {
      if (!ent.IsValid() || !ent.IsAlive()) continue;
      if (!s.showTeammates && ent.team == localTeam) continue;

      float dx = ent.renderPosition.x - localPos.x;
      float dy = ent.renderPosition.y - localPos.y;

      float final_x = dx * baseScale * windowRatio;
      float final_y = dy * baseScale * windowRatio;

      if (s.rotate) {
        float rot_x = final_x * c - final_y * sn;
        float rot_y = final_x * sn + final_y * c;
        final_x = rot_x;
        final_y = rot_y;
      }

      float dist = std::sqrtf(final_x * final_x + final_y * final_y);
      float maxDist = radius - s.pointSize;

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
        colSrc = s.teamColor;
      } else {
        if (s.visibleCheck) {
          colSrc = ent.isSpotted ? s.visibleColor : s.hiddenColor;
        } else {
          colSrc = s.enemyColor;
        }
      }

      float dotSize = isClamped ? s.pointSize * 0.7f : s.pointSize;
      int alpha = isClamped ? 180 : 255;
      ImU32 finalColor = ImGui::ColorConvertFloat4ToU32(
          ImVec4(colSrc[0], colSrc[1], colSrc[2], colSrc[3]));
      finalColor = (finalColor & 0x00FFFFFF) | (alpha << 24);

      dl->AddCircleFilled(dotPos, dotSize + 1.0f, IM_COL32(0, 0, 0, alpha));
      dl->AddCircleFilled(dotPos, dotSize, finalColor);
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
}

void Radar::RenderUI() {}

} // namespace Features
