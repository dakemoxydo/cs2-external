#include "misc.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "misc_config.h"
#include "render/draw/draw_list.h"
#include <imgui.h>
#include <windows.h>


namespace Features {

void Misc::Update() {

  // Future: bhop, auto-strafe, etc.
}

void Misc::Render(Render::DrawList &drawList) {
  if (!Config::Settings.misc.awpCrosshair)
    return;

  // Only show when NOT scoped (when scoped the in-game scope is enough)
  uintptr_t localPawn = Core::MemoryManager::Read<uintptr_t>(
      Core::GameManager::GetClientBase() + SDK::Offsets::dwLocalPlayerPawn);
  if (!localPawn)
    return;

  bool isScoped =
      Core::MemoryManager::Read<bool>(localPawn + SDK::Offsets::m_bIsScoped);
  if (isScoped)
    return;

  // Read weapon to restrict crosshair to sniper rifles only
  uintptr_t cw = Core::MemoryManager::Read<uintptr_t>(
      localPawn + SDK::Offsets::m_pClippingWeapon);
  if (cw <= 0x10000)
    return;
  uintptr_t wPtr = Core::MemoryManager::Read<uintptr_t>(cw + 0x10);
  if (wPtr <= 0x10000)
    return;
  uintptr_t nPtr = Core::MemoryManager::Read<uintptr_t>(wPtr + 0x20);
  if (nPtr <= 0x10000)
    return;

  char wb[64] = {};
  Core::MemoryManager::ReadRaw(nPtr, wb, sizeof(wb) - 1);
  std::string weaponName = wb;
  if (weaponName.find("awp") == std::string::npos &&
      weaponName.find("ssg08") == std::string::npos) {
    return;
  }

  ImGuiIO &io = ImGui::GetIO();
  float cx = io.DisplaySize.x / 2.0f;
  float cy = io.DisplaySize.y / 2.0f;
  float sz = Config::Settings.misc.crosshairSize;
  float th = Config::Settings.misc.crosshairThickness;
  float gap = Config::Settings.misc.crosshairGap ? sz * 0.4f : 0.0f;
  float *col = Config::Settings.misc.crosshairColor;
  int style = Config::Settings.misc.crosshairStyle;

  // 0 = dot (small circle)
  if (style == 0 || style == 3) {
    drawList.DrawCircle(cx, cy, 1.5f, col, 8, th);
  }
  // 1 = cross
  if (style == 1 || style == 3) {
    drawList.DrawLine(cx - sz - gap, cy, cx - gap, cy, col, th);
    drawList.DrawLine(cx + gap, cy, cx + sz + gap, cy, col, th);
    drawList.DrawLine(cx, cy - sz - gap, cx, cy - gap, col, th);
    drawList.DrawLine(cx, cy + gap, cx, cy + sz + gap, col, th);
  }
  // 2 = circle
  if (style == 2 || style == 3) {
    drawList.DrawCircle(cx, cy, sz * 1.5f, col, 32, th);
  }
}

} // namespace Features
