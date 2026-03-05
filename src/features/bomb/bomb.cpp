#include "bomb.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "render/draw/draw_list.h"
#include <chrono>
#include <cmath>
#include <imgui.h>
#include <string>
#include <windows.h>

namespace Features {

BombConfig bombConfig;

// C4 member offsets verified from a2x dumper / reference
namespace C4Off {
static constexpr ptrdiff_t m_nBombSite = 0x119C;    // int  (0=A, 1=B)
static constexpr ptrdiff_t m_bBombTicking = 0x120C; // bool
} // namespace C4Off

// ─── State
// ────────────────────────────────────────────────────────────────────────────
static bool s_planted = false;
static float s_timeLeft = 0.0f;
static int s_site = -1;

void Bomb::Update() {
  SDK::BombInfo info = Core::GameManager::GetBombInfo();

  if (!info.isPlanted) {
    s_planted = false;
    s_timeLeft = 0.0f;
    s_site = -1;
    return;
  }

  s_planted = true;
  s_site = info.site;
  s_timeLeft = info.timeLeft;
}

void Bomb::Render(Render::DrawList & /*drawList*/) {
  if (!Config::Settings.bomb.enabled || !s_planted || s_timeLeft <= 0.0f)
    return;

  constexpr float BOMB_TIME = 40.0f;
  float progress = s_timeLeft / BOMB_TIME;
  const char *site = (s_site == 1) ? "B" : "A";

  ImGuiIO &io = ImGui::GetIO();
  const float winW = 240.0f;
  ImGui::SetNextWindowPos(
      ImVec2((io.DisplaySize.x - winW) * 0.5f, io.DisplaySize.y - 80.0f),
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
  ImGui::TextColored(col, "%.1fs left  |  Site: %s", s_timeLeft, site);
  ImGui::End();
}

} // namespace Features
