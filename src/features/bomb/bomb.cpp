#include "bomb.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "core/sdk/entity.h"
#include "render/draw/draw_list.h"
#include "render/overlay/overlay.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <imgui.h>
#include <shared_mutex>
#include <string>
#include <windows.h>

namespace Features {

static bool s_planted = false;
static float s_timeLeft = 0.0f;
static float s_totalTime = 40.0f;
static bool s_isBeingDefused = false;
static float s_defuseTimeLeft = 0.0f;
static int s_site = -1;

void Bomb::Update() {
  SDK::BombInfo info = Core::GameManager::GetBombInfo();

  if (!info.isPlanted) {
    s_planted = false;
    s_timeLeft = 0.0f;
    s_totalTime = 40.0f;
    s_isBeingDefused = false;
    s_defuseTimeLeft = 0.0f;
    s_site = -1;
    return;
  }

  s_planted = true;
  s_site = info.site;
  s_timeLeft = info.timeLeft;
  s_totalTime = info.totalTime > 0.0f ? info.totalTime : 40.0f;
  s_isBeingDefused = info.isBeingDefused;
  s_defuseTimeLeft = info.defuseTimeLeft;
}

void Bomb::Render(Render::DrawList & /*drawList*/) {
  bool bombEnabled;
  {
    std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
    bombEnabled = Config::Settings.bomb.enabled;
  }
  if (!bombEnabled || !s_planted || s_timeLeft <= 0.0f)
    return;

  float progress = s_totalTime > 0.0f ? s_timeLeft / s_totalTime : 0.0f;
  progress = (std::clamp)(progress, 0.0f, 1.0f);
  const char *site = (s_site == 1) ? "B" : "A";

  int gameW = Render::Overlay::GetGameWidth();
  if (gameW <= 0) return;
  const float winW = 240.0f;
  ImGui::SetNextWindowPos(
      ImVec2((gameW - winW) * 0.5f, Render::Overlay::GetGameHeight() - 80.0f),
      ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(winW, 62.0f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.82f);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoInputs |
                           ImGuiWindowFlags_NoScrollbar;

  if (!ImGui::Begin("##BombTimer", nullptr, flags)) {
    ImGui::End();
    return;
  }

  ImVec4 col;
  if (progress > 0.5f)
    col = ImVec4(0.05f, 0.90f, 0.30f, 1.0f);
  else if (progress > 0.2f)
    col = ImVec4(1.00f, 0.80f, 0.00f, 1.0f);
  else
    col = ImVec4(1.00f, 0.15f, 0.15f, 1.0f);

  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
  char label[32];
  snprintf(label, sizeof(label), "BOMB [%s]  %.0fs", site, ceilf(s_timeLeft));
  ImGui::ProgressBar(progress, ImVec2(-1.0f, 22.0f), label);
  ImGui::PopStyleColor();
  if (s_isBeingDefused && s_defuseTimeLeft > 0.0f) {
    ImGui::TextColored(col, "%.1fs left  |  Defuse %.1fs  |  Site: %s", s_timeLeft,
                       s_defuseTimeLeft, site);
  } else {
    ImGui::TextColored(col, "%.1fs left  |  Site: %s", s_timeLeft, site);
  }
  ImGui::End();
}

void Bomb::RenderUI() {}

} // namespace Features
