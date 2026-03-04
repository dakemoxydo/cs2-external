#include "misc.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "misc_config.h"
#include "render/draw/draw_list.h"
#include <windows.h>


namespace Features {

MiscConfig miscConfig;

void Misc::Update() {
  // Future: bhop, auto-strafe, etc.
}

void Misc::Render(Render::DrawList &drawList) {
  if (!miscConfig.awpCrosshair)
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

  int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  float cx = sw / 2.0f;
  float cy = sh / 2.0f;
  float sz = miscConfig.crosshairSize;
  float th = miscConfig.crosshairThickness;
  float gap = miscConfig.crosshairGap ? sz * 0.4f : 0.0f;
  float *col = miscConfig.crosshairColor;
  int style = miscConfig.crosshairStyle;

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
