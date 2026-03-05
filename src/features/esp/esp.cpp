#include "esp.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/sdk/entity.h"
#include "esp_config.h"
#include "render/draw/draw_list.h"
#include <imgui.h>
#include <string>

namespace Features {

// в”Ђв”Ђв”Ђ Bone connections defined in entity.h (uses real BoneIndex enum)
// в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
// ::s_boneConnections is the inline constexpr array from entity.h

void Esp::Update() {
  // Lightweight вЂ” main work done in GameManager::Update()
}

// в”Ђв”Ђв”Ђ Helpers
// в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

// Draw corner-bracket style box (4 corners, each with two short lines)
static void DrawCornerBox(float x, float y, float w, float h, float col[4],
                          float thickness) {
  auto *dl = ImGui::GetBackgroundDrawList();
  ImU32 c =
      ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], col[3]));
  float cx = w * 0.22f; // corner extent X
  float cy = h * 0.12f; // corner extent Y

  // Top-left
  dl->AddLine(ImVec2(x, y), ImVec2(x + cx, y), c, thickness);
  dl->AddLine(ImVec2(x, y), ImVec2(x, y + cy), c, thickness);
  // Top-right
  dl->AddLine(ImVec2(x + w, y), ImVec2(x + w - cx, y), c, thickness);
  dl->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + cy), c, thickness);
  // Bottom-left
  dl->AddLine(ImVec2(x, y + h), ImVec2(x + cx, y + h), c, thickness);
  dl->AddLine(ImVec2(x, y + h), ImVec2(x, y + h - cy), c, thickness);
  // Bottom-right
  dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w - cx, y + h), c, thickness);
  dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - cy), c, thickness);
}

// в”Ђв”Ђв”Ђ Render
// в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

void Esp::Render(Render::DrawList &drawList) {
  const bool anyActive =
      Config::Settings.esp.showBox || Config::Settings.esp.showHealth ||
      Config::Settings.esp.showName || Config::Settings.esp.showWeapon ||
      Config::Settings.esp.showDistance || Config::Settings.esp.showBones ||
      Config::Settings.esp.showSnapLines;
  if (!anyActive)
    return;

  const ImGuiIO &io = ImGui::GetIO();
  const int screenWidth = static_cast<int>(io.DisplaySize.x);
  const int screenHeight = static_cast<int>(io.DisplaySize.y);
  const SDK::Matrix4x4 viewMatrix = Core::GameManager::GetViewMatrix();
  auto *bgDL = ImGui::GetBackgroundDrawList();

  const auto players = Core::GameManager::GetRenderPlayers();

  for (const auto &player : players) {
    if (!player.IsValid() || !player.IsAlive())
      continue;
    if (player.isTeammate && !Config::Settings.esp.showTeammates)
      continue;

    // в”Ђв”Ђв”Ђ Pick color palette
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    const float *currentBoxColor = player.isTeammate
                                       ? Config::Settings.esp.teamColor
                                       : Config::Settings.esp.boxColor;
    float drawColor[4] = {currentBoxColor[0], currentBoxColor[1],
                          currentBoxColor[2], currentBoxColor[3]};

    // в”Ђв”Ђв”Ђ World-to-screen
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    SDK::Vector3 feetPos = player.position;
    SDK::Vector3 headPos = feetPos;
    headPos.z += 72.0f;

    SDK::Vector2 screenFeet, screenHead;
    if (!Core::Math::WorldToScreen(feetPos, screenFeet, viewMatrix, screenWidth,
                                   screenHeight) ||
        !Core::Math::WorldToScreen(headPos, screenHead, viewMatrix, screenWidth,
                                   screenHeight))
      continue;

    const float height = screenFeet.y - screenHead.y;
    if (height <= 0.0f)
      continue;
    const float width = height / 2.0f;
    const float x = screenFeet.x - width / 2.0f;
    const float y = screenHead.y;

    // в”Ђв”Ђв”Ђ Snap Lines
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    if (Config::Settings.esp.showSnapLines) {
      float sc[4] = {Config::Settings.esp.snapLineColor[0],
                     Config::Settings.esp.snapLineColor[1],
                     Config::Settings.esp.snapLineColor[2],
                     Config::Settings.esp.snapLineColor[3]};
      drawList.DrawLine(static_cast<float>(screenWidth) / 2.0f,
                        static_cast<float>(screenHeight), screenFeet.x,
                        screenFeet.y, sc, 1.0f);
    }

    // в”Ђв”Ђв”Ђ Box
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    if (Config::Settings.esp.showBox) {
      float outlineColor[4] = {0.0f, 0.0f, 0.0f, 0.85f};

      switch (Config::Settings.esp.boxStyle) {
      case BoxStyle::Corners: {
        // Dark shadow
        float shadowCol[4] = {0.f, 0.f, 0.f, 0.7f};
        DrawCornerBox(x - 1, y - 1, width + 2, height + 2, shadowCol, 3.0f);
        DrawCornerBox(x, y, width, height, drawColor, 1.5f);
        break;
      }
      case BoxStyle::Filled: {
        // Semi-transparent fill
        ImVec4 fillV(drawColor[0], drawColor[1], drawColor[2],
                     Config::Settings.esp.fillBoxAlpha);
        bgDL->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height),
                            ImGui::ColorConvertFloat4ToU32(fillV));
        // Outer glow / outline
        drawList.DrawBox(x - 1, y - 1, width + 2, height + 2, outlineColor,
                         1.0f);
        drawList.DrawBox(x, y, width, height, drawColor, 1.5f);
        break;
      }
      default: // BoxStyle::Rect
        drawList.DrawBox(x - 1, y - 1, width + 2, height + 2, outlineColor,
                         1.0f);
        drawList.DrawBox(x, y, width, height, drawColor, 1.5f);
        drawList.DrawBox(x + 1, y + 1, width - 2, height - 2, outlineColor,
                         1.0f);
        break;
      }
    }

    // в”Ђв”Ђв”Ђ Health Bar
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    if (Config::Settings.esp.showHealth) {
      float hpPercent = static_cast<float>(player.health) / 100.0f;
      if (hpPercent > 1.0f)
        hpPercent = 1.0f;
      if (hpPercent < 0.0f)
        hpPercent = 0.0f;
      float hpColor[4] = {1.0f - hpPercent, hpPercent, 0.0f, 1.0f};
      float bgColor[4] = {0.0f, 0.0f, 0.0f, 0.9f};

      if (Config::Settings.esp.healthBarStyle == HealthBarStyle::Bottom) {
        // Horizontal bar under the box
        const float barY = y + height + 3.0f;
        const float barH = 4.0f;
        const float barW = width * hpPercent;
        drawList.DrawLine(x, barY, x + width, barY, bgColor, barH);
        drawList.DrawLine(x, barY, x + barW, barY, hpColor, barH - 1.0f);
      } else {
        // Side vertical bar (default)
        const float hpHeight = height * hpPercent;
        drawList.DrawLine(x - 6, y, x - 6, y + height, bgColor, 3.0f);
        drawList.DrawLine(x - 6, y + height - hpHeight, x - 6, y + height,
                          hpColor, 2.0f);
      }

      // Optional HP text
      if (Config::Settings.esp.showHealthText) {
        std::string hpStr = std::to_string(player.health) + " HP";
        float textCol[4] = {hpColor[0], hpColor[1], hpColor[2], 1.0f};
        if (Config::Settings.esp.healthBarStyle == HealthBarStyle::Bottom)
          drawList.AddText(x, y + height + 9.0f, hpStr.c_str(), textCol);
        else
          drawList.AddText(x - 22.0f, y, hpStr.c_str(), textCol);
      }
    }

    // в”Ђв”Ђв”Ђ Name
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    if (Config::Settings.esp.showName) {
      float nCol[4] = {
          Config::Settings.esp.nameColor[0], Config::Settings.esp.nameColor[1],
          Config::Settings.esp.nameColor[2], Config::Settings.esp.nameColor[3]};
      std::string drawName = player.name.empty() ? "Player" : player.name;
      drawList.AddText(x + width / 2.0f - 15.0f, y - 15.0f, drawName.c_str(),
                       nCol);
    }

    // в”Ђв”Ђв”Ђ Weapon
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    if (Config::Settings.esp.showWeapon && !player.weapon.empty()) {
      float wCol[4] = {Config::Settings.esp.weaponColor[0],
                       Config::Settings.esp.weaponColor[1],
                       Config::Settings.esp.weaponColor[2],
                       Config::Settings.esp.weaponColor[3]};
      drawList.AddText(x + width / 2.0f - 15.0f, y + height + 2.0f,
                       player.weapon.c_str(), wCol);
    }

    // в”Ђв”Ђв”Ђ Distance
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    if (Config::Settings.esp.showDistance && player.distance > 0.0f) {
      float dCol[4] = {
          Config::Settings.esp.distColor[0], Config::Settings.esp.distColor[1],
          Config::Settings.esp.distColor[2], Config::Settings.esp.distColor[3]};
      std::string distStr =
          std::to_string(static_cast<int>(player.distance)) + "m";
      drawList.AddText(x + width - 5.0f, y + height + 2.0f, distStr.c_str(),
                       dCol);
    }

    // в”Ђв”Ђв”Ђ Skeleton & Head Circle
    // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
    if (Config::Settings.esp.showBones && !player.bonePositions.empty() &&
        (Config::Settings.esp.skeletonMaxDistance <= 0.0f ||
         player.distance <= Config::Settings.esp.skeletonMaxDistance)) {

      float boneCol[4] = {
          Config::Settings.esp.boneColor[0], Config::Settings.esp.boneColor[1],
          Config::Settings.esp.boneColor[2], Config::Settings.esp.boneColor[3]};
      float shadowC[4] = {Config::Settings.esp.skeletonOutlineColor[0],
                          Config::Settings.esp.skeletonOutlineColor[1],
                          Config::Settings.esp.skeletonOutlineColor[2],
                          Config::Settings.esp.skeletonOutlineColor[3]};

      for (const auto &conn : ::s_boneConnections) {
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
        if (okA && okB) {
          if (Config::Settings.esp.skeletonOutline)
            drawList.DrawLine(scrA.x, scrA.y, scrB.x, scrB.y, shadowC, 2.8f);
          drawList.DrawLine(scrA.x, scrA.y, scrB.x, scrB.y, boneCol, 1.2f);
        }
      }

      // Draw Head Circle as part of skeleton
      if ((int)player.bonePositions.size() > BONE_HEAD) {
        SDK::Vector2 boneHead;
        if (Core::Math::WorldToScreen(player.bonePositions[BONE_HEAD], boneHead,
                                      viewMatrix, screenWidth, screenHeight)) {
          // Make head circle size halfway between old (0.22f) and the very
          // small (0.12f)
          const float headRadius = width * 0.18f;
          if (Config::Settings.esp.skeletonOutline)
            drawList.DrawCircle(boneHead.x, boneHead.y, headRadius + 1.0f,
                                shadowC, 28, 2.5f);
          drawList.DrawCircle(boneHead.x, boneHead.y, headRadius, boneCol, 28,
                              1.2f);
        }
      }
    }
  }
}

} // namespace Features
