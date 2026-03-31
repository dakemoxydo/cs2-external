#include "misc.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/offsets.h"
#include "misc_config.h"
#include "render/draw/draw_list.h"
#include "render/overlay/overlay.h"
#include <imgui.h>
#include <windows.h>

namespace Features {

void Misc::Update() {

  // Future: bhop, auto-strafe, etc.
}

void Misc::Render(Render::DrawList &drawList) {
  if (!Config::Settings.misc.awpCrosshair)
    return;

  bool isScoped = Core::GameManager::IsLocalScoped();
  if (isScoped)
    return;

  std::string weaponName = Core::GameManager::GetLocalWeaponName();
  if (weaponName.find("awp") == std::string::npos &&
      weaponName.find("ssg08") == std::string::npos) {
    return;
  }

  int gameW = Render::Overlay::GetGameWidth();
  int gameH = Render::Overlay::GetGameHeight();
  if (gameW <= 0 || gameH <= 0) return;

  float cx = gameW / 2.0f;
  float cy = gameH / 2.0f;
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
