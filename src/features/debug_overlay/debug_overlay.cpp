#include "debug_overlay.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/sdk/offsets.h"
#include "features/feature_frame.h"
#include "render/overlay/overlay.h"
#include <chrono>
#include <imgui.h>
#include <shared_mutex>
#include <sstream>
#include <windows.h>


namespace Features {

using Clock = std::chrono::steady_clock;

void DebugOverlay::OnEnable() {
  lastFrame_ = Clock::now();
  fps_ = 0.0f;
}

void DebugOverlay::Update(const FeatureFrame &) {
  auto now = Clock::now();
  float dt = std::chrono::duration<float>(now - lastFrame_).count();
  lastFrame_ = now;
  if (dt > 0.0f)
    fps_ = 0.9f * fps_ + 0.1f * (1.0f / dt); // smooth
}

void DebugOverlay::Render(const FeatureFrame &frame,
                          Render::DrawList & /*drawList*/) {
  const bool isEnabled = frame.settings.debug.enabled;
  const bool devMode = frame.settings.debug.devMode;
  if (!isEnabled) return;

  const float PAD = 8.0f;
  ImGui::SetNextWindowPos(ImVec2(PAD, PAD), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.55f);
  ImGui::SetNextWindowSize(ImVec2(210.0f, 0.0f), ImGuiCond_Always);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;

  if (!ImGui::Begin("##DebugOverlay", nullptr, flags)) {
    ImGui::End();
    return;
  }

  ImVec4 yellow = ImVec4(1.0f, 0.85f, 0.0f, 1.0f);
  ImVec4 grey = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
  ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  ImVec4 cyan = ImVec4(0.0f, 0.9f, 1.0f, 1.0f);

  // ── Normal mode (always visible) ──
  ImGui::TextColored(yellow, "FPS");
  ImGui::SameLine(60);
  ImGui::TextColored(white, "%.0f", fps_);

  const auto &snapshot = frame.game;
  ImGui::TextColored(yellow, "Entities");
  ImGui::SameLine(60);
  ImGui::TextColored(white, "%zu", snapshot.players.size());

  int gameW = Render::Overlay::GetGameWidth();
  int gameH = Render::Overlay::GetGameHeight();
  ImGui::TextColored(yellow, "Screen");
  ImGui::SameLine(60);
  ImGui::TextColored(grey, "%dx%d", gameW, gameH);

  // ── Developer mode (extra data) ──
  if (devMode) {
    ImGui::Separator();
    ImGui::TextColored(cyan, "-- DEV --");

    uintptr_t base = snapshot.clientBase;
    const auto offsets = SDK::Offsets::GetCopy();
    ImGui::TextColored(yellow, "client.dll");
    ImGui::SameLine(60);
    ImGui::TextColored(grey, "0x%llX", (unsigned long long)base);

    ImGui::TextColored(yellow, "EntityList");
    ImGui::SameLine(60);
    ImGui::TextColored(grey, "0x%llX",
                       (unsigned long long)offsets.dwEntityList);

    ImGui::TextColored(yellow, "ViewMatrix");
    ImGui::SameLine(60);
    ImGui::TextColored(grey, "0x%llX",
                       (unsigned long long)offsets.dwViewMatrix);
  }

  ImGui::End();
}

void DebugOverlay::RenderUI() {}

} // namespace Features
