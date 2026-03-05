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

// в”Ђв”Ђв”Ђ State
// в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
static bool s_planted = false;
static bool s_prevPlanted = false;
static std::chrono::steady_clock::time_point s_plantTime;
static float s_timeLeft = 0.0f;
static int s_site = -1;

void Bomb::Update() {
  uintptr_t clientBase = Core::GameManager::GetClientBase();
  if (!clientBase || SDK::Offsets::dwPlantedC4 == 0)
    return;

  // Reference bomb detection (same pattern as reference esp):
  // 1. read pointer-to-C4 from [client.base + dwPlantedC4]
  // 2. single deref в†’ entity pointer
  // 3. sanity-check: health > 0 means entity is alive (planted C4 has "health")
  uintptr_t c4Ptr = Core::MemoryManager::Read<uintptr_t>(
      clientBase + SDK::Offsets::dwPlantedC4);

  // c4Ptr == 0 means no planted bomb
  if (!c4Ptr || c4Ptr < 0x10000) {
    s_planted = false;
    s_prevPlanted = false;
    s_timeLeft = 0.0f;
    s_site = -1;
    return;
  }

  // Read actual entity address (single deref from the pointer)
  uintptr_t c4 = Core::MemoryManager::Read<uintptr_t>(c4Ptr);
  if (!c4 || c4 < 0x10000) {
    // Sometimes dwPlantedC4 IS already the entity directly (no extra deref)
    c4 = c4Ptr;
  }

  // Verify bomb is actually ticking
  bool ticking = Core::MemoryManager::Read<bool>(c4 + C4Off::m_bBombTicking);
  if (!ticking) {
    s_planted = false;
    s_prevPlanted = false;
    s_timeLeft = 0.0f;
    s_site = -1;
    return;
  }

  s_site = Core::MemoryManager::Read<int>(c4 + C4Off::m_nBombSite);
  s_planted = true;

  if (!s_prevPlanted) {
    s_plantTime = std::chrono::steady_clock::now();
    s_prevPlanted = true;
  }

  constexpr float BOMB_TIME = 40.0f;
  float elapsed = std::chrono::duration<float>(
                      std::chrono::steady_clock::now() - s_plantTime)
                      .count();
  s_timeLeft = BOMB_TIME - elapsed;
  if (s_timeLeft < 0.0f)
    s_timeLeft = 0.0f;
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
