#include "esp.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "esp_config.h"
#include "render/draw/draw_list.h"
#include <chrono>
#include <iostream>
#include <string>
#include <windows.h>

namespace Features {

EspConfig config;

void Esp::Update() {
  static auto lastDebugTime = std::chrono::steady_clock::now();
  auto currentTime = std::chrono::steady_clock::now();

  if (std::chrono::duration_cast<std::chrono::seconds>(currentTime -
                                                       lastDebugTime)
          .count() >= 5) {
    if (config.enabled && Core::GameManager::GetClientBase() != 0) {
      auto players = Core::GameManager::GetPlayers();
      std::cout << "[DEBUG-ESP] Running ESP loop. Players found: "
                << players.size() << std::endl;

      int validCount = 0;
      for (const auto &p : players) {
        if (p.IsValid() && p.IsAlive()) {
          validCount++;
          SDK::Vector3 pos = p.position;
          std::cout << "  -> Valid Player HT: " << p.health << " | Pos: ("
                    << pos.x << ", " << pos.y << ", " << pos.z << ")"
                    << std::endl;
        }
      }
      std::cout << "[DEBUG-ESP] Valid/Alive players: " << validCount
                << std::endl;
    }
    lastDebugTime = currentTime;
  }
}

void Esp::Render(Render::DrawList &drawList) {
  if (!config.showBox && !config.showHealth && !config.showName)
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
    }
  }
}
} // namespace Features
