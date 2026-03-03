#include "esp.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/sdk/entity.h"
#include "esp_config.h"
#include "render/draw/draw_list.h"
#include <string>
#include <windows.h>

namespace Features {

EspConfig config;

void Esp::Update() {
  // Lightweight — main work done in GameManager::Update()
}

void Esp::Render(Render::DrawList &drawList) {
  if (!config.showBox && !config.showHealth && !config.showName &&
      !config.showWeapon && !config.showDistance && !config.showBones)
    return;

  int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  SDK::Matrix4x4 viewMatrix = Core::GameManager::GetViewMatrix();

  auto players = Core::GameManager::GetPlayers();

  for (const auto &player : players) {
    if (!player.IsValid() || !player.IsAlive())
      continue;

    if (player.isTeammate && !config.showTeammates)
      continue;

    const float *currentBoxColor =
        player.isTeammate ? config.teamColor : config.boxColor;
    float drawColor[4] = {currentBoxColor[0], currentBoxColor[1],
                          currentBoxColor[2], currentBoxColor[3]};

    SDK::Vector3 feetPos = player.position;
    SDK::Vector3 headPos = feetPos;
    headPos.z += 72.0f; // Approx head height

    SDK::Vector2 screenFeet, screenHead;
    if (Core::Math::WorldToScreen(feetPos, screenFeet, viewMatrix, screenWidth,
                                  screenHeight) &&
        Core::Math::WorldToScreen(headPos, screenHead, viewMatrix, screenWidth,
                                  screenHeight)) {

      float height = screenFeet.y - screenHead.y;
      float width = height / 2.0f;
      float x = screenFeet.x - width / 2.0f;
      float y = screenHead.y;

      if (config.showBox) {
        drawList.DrawBox(x, y, width, height, drawColor, 1.5f);
        float outlineColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        drawList.DrawBox(x - 1, y - 1, width + 2, height + 2, outlineColor,
                         1.0f);
        drawList.DrawBox(x + 1, y + 1, width - 2, height - 2, outlineColor,
                         1.0f);
      }

      if (config.showHealth) {
        int health = player.health;
        float hpPercent = health / 100.0f;
        if (hpPercent > 1.0f)
          hpPercent = 1.0f;
        if (hpPercent < 0.0f)
          hpPercent = 0.0f;

        float hpColor[4] = {1.0f - hpPercent, hpPercent, 0.0f, 1.0f};
        float hpHeight = height * hpPercent;

        // Background
        float bgColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        drawList.DrawLine(x - 6, y, x - 6, y + height, bgColor, 3.0f);
        // Health bar
        drawList.DrawLine(x - 6, y + height - hpHeight, x - 6, y + height,
                          hpColor, 2.0f);
      }

      if (config.showName) {
        float textColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        std::string drawName = player.name.empty() ? "Player" : player.name;
        drawList.AddText(x + width / 2.0f - 15.0f, y - 15.0f, drawName.c_str(),
                         textColor);
      }

      if (config.showWeapon && !player.weapon.empty()) {
        float weapColor[4] = {1.0f, 0.85f, 0.0f, 1.0f}; // yellow
        drawList.AddText(x + width / 2.0f - 15.0f, y + height + 2.0f,
                         player.weapon.c_str(), weapColor);
      }

      if (config.showDistance && player.distance > 0.0f) {
        float distColor[4] = {0.7f, 0.7f, 0.7f, 1.0f}; // grey
        std::string distStr =
            std::to_string(static_cast<int>(player.distance)) + "m";
        drawList.AddText(x + width - 5.0f, y + height + 2.0f, distStr.c_str(),
                         distColor);
      }

      // ─────────── Skeleton ───────────
      if (config.showBones && !player.bonePositions.empty() &&
          (player.distance <= config.skeletonMaxDistance ||
           config.skeletonMaxDistance <= 0.0f)) {
        float boneCol[4] = {config.boneColor[0], config.boneColor[1],
                            config.boneColor[2], config.boneColor[3]};

        for (auto &conn : s_boneConnections) {
          int idxA = conn[0];
          int idxB = conn[1];
          if (idxA >= (int)player.bonePositions.size() ||
              idxB >= (int)player.bonePositions.size())
            continue;

          SDK::Vector2 scrA, scrB;
          bool okA =
              Core::Math::WorldToScreen(player.bonePositions[idxA], scrA,
                                        viewMatrix, screenWidth, screenHeight);
          bool okB =
              Core::Math::WorldToScreen(player.bonePositions[idxB], scrB,
                                        viewMatrix, screenWidth, screenHeight);
          if (okA && okB)
            drawList.DrawLine(scrA.x, scrA.y, scrB.x, scrB.y, boneCol, 1.2f);
        }
      }
    }
  }
}
} // namespace Features
