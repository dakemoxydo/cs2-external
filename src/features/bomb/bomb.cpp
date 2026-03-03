#include "bomb.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/entity.h"
#include "core/sdk/offsets.h"
#include "render/draw/draw_list.h"
#include <cmath>
#include <ctime>
#include <imgui.h>
#include <string>
#include <windows.h>

namespace Features {

BombConfig bombConfig;

// Bomb member offsets (from reference Offsets.hpp)
namespace BombOffsets {
static constexpr ptrdiff_t m_isPlanted = 0x8;    // unk
static constexpr ptrdiff_t m_nBombSite = 0x1174; // int32
static constexpr ptrdiff_t m_pGameSceneNode = 0x338;
static constexpr ptrdiff_t m_vecAbsOrigin = 0xD0; // Vec3 in CGameSceneNode
} // namespace BombOffsets

// ─── State ───
static bool s_prevPlanted = false;
static bool s_planted = false;
static std::time_t s_plantTime = 0;
static float s_timeLeft = 0.0f;
static int s_site = -1; // 0=A, 1=B

// ─── IFeature::Update ───
void Bomb::Update() {
  // enabled check uses IFeature::enabled (set via menu)
  uintptr_t clientBase = Core::GameManager::GetClientBase();
  if (!clientBase)
    return;

  // dwPlantedC4 must be non-zero (parsed from a2x offsets.json)
  if (SDK::Offsets::dwPlantedC4 == 0)
    return;

  // Check if bomb is planted at all
  // Reference: is_planted = read<uintptr_t>(client.base + plantedC4 -
  // m_isPlanted) This reads the field _before_ the entity pointer to check
  // validity
  uintptr_t isPlanted = Core::MemoryManager::Read<uintptr_t>(
      clientBase + SDK::Offsets::dwPlantedC4 - BombOffsets::m_isPlanted);

  if (!isPlanted) {
    s_prevPlanted = false;
    s_planted = false;
    s_timeLeft = 0.0f;
    return;
  }

  // Double-deref to get actual entity address
  uintptr_t addr = Core::MemoryManager::Read<uintptr_t>(
      clientBase + SDK::Offsets::dwPlantedC4);
  addr = Core::MemoryManager::Read<uintptr_t>(addr);

  if (!addr || addr < 0x10000)
    return;

  s_planted = true;
  s_site = Core::MemoryManager::Read<uint32_t>(addr + BombOffsets::m_nBombSite);

  // Record plant time only once (same as reference: if !prev_is_planted)
  if (!s_prevPlanted) {
    s_plantTime = std::time(nullptr);
  }
  s_prevPlanted = true;

  // Time left: reference uses 41s constant (slightly generous)
  constexpr float BOMB_FULL_TIME = 41.0f;
  float elapsed = static_cast<float>(std::time(nullptr) - s_plantTime);
  s_timeLeft = BOMB_FULL_TIME - elapsed;
  if (s_timeLeft < 0.0f)
    s_timeLeft = 0.0f;
}

// ─── IFeature::Render ───
void Bomb::Render(Render::DrawList & /*drawList*/) {
  if (!s_planted || s_timeLeft <= 0.0f)
    return;

  constexpr float BOMB_FULL_TIME = 41.0f;
  float progress = s_timeLeft / BOMB_FULL_TIME;
  const char *siteName = (s_site == 1) ? "B" : "A";

  ImGuiIO &io = ImGui::GetIO();
  const float winW = 230.0f;
  ImGui::SetNextWindowPos(
      ImVec2((io.DisplaySize.x - winW) * 0.5f, io.DisplaySize.y - 75.0f),
      ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(winW, 60.0f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.78f);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoInputs |
                           ImGuiWindowFlags_NoScrollbar;

  if (!ImGui::Begin("##BombTimer", nullptr, flags)) {
    ImGui::End();
    return;
  }

  // Color: green→yellow→red
  ImVec4 col;
  if (progress > 0.5f)
    col = ImVec4(0.05f, 0.90f, 0.30f, 1.0f);
  else if (progress > 0.2f)
    col = ImVec4(1.00f, 0.80f, 0.00f, 1.0f);
  else
    col = ImVec4(1.00f, 0.15f, 0.15f, 1.0f);

  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
  char label[32];
  snprintf(label, sizeof(label), "BOMB [%s]  %.0fs", siteName,
           ceilf(s_timeLeft));
  ImGui::ProgressBar(progress, ImVec2(-1.0f, 22.0f), label);
  ImGui::PopStyleColor();

  ImGui::TextColored(col, "%.1fs remaining  |  Site: %s", s_timeLeft, siteName);
  ImGui::End();
}

} // namespace Features
