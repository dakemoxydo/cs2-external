#include "bomb.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/sdk/entity.h"
#include "features/feature_frame.h"
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

void Bomb::ResetState() {
  planted_ = false;
  timeLeft_ = 0.0f;
  totalTime_ = 40.0f;
  isBeingDefused_ = false;
  defuseTimeLeft_ = 0.0f;
  site_ = -1;
}

void Bomb::OnDisable() { ResetState(); }

void Bomb::Update(const FeatureFrame &frame) {
  const SDK::BombInfo &info = frame.game.bombInfo;

  if (!info.isPlanted) {
    ResetState();
    return;
  }

  planted_ = true;
  site_ = info.site;
  timeLeft_ = info.timeLeft;
  totalTime_ = info.totalTime > 0.0f ? info.totalTime : 40.0f;
  isBeingDefused_ = info.isBeingDefused;
  defuseTimeLeft_ = info.defuseTimeLeft;
}

void Bomb::Render(const FeatureFrame &frame, Render::DrawList & /*drawList*/) {
  const bool bombEnabled = frame.settings.bomb.enabled;
  if (!bombEnabled || !planted_ || timeLeft_ <= 0.0f)
    return;

  float progress = totalTime_ > 0.0f ? timeLeft_ / totalTime_ : 0.0f;
  progress = (std::clamp)(progress, 0.0f, 1.0f);
  const char *site = (site_ == 1) ? "B" : "A";

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
  snprintf(label, sizeof(label), "BOMB [%s]  %.0fs", site, ceilf(timeLeft_));
  ImGui::ProgressBar(progress, ImVec2(-1.0f, 22.0f), label);
  ImGui::PopStyleColor();
  if (isBeingDefused_ && defuseTimeLeft_ > 0.0f) {
    ImGui::TextColored(col, "%.1fs left  |  Defuse %.1fs  |  Site: %s", timeLeft_,
                       defuseTimeLeft_, site);
  } else {
    ImGui::TextColored(col, "%.1fs left  |  Site: %s", timeLeft_, site);
  }
  ImGui::End();
}

void Bomb::RenderUI() {}

} // namespace Features
