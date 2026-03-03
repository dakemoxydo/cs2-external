#include "debug_overlay.h"
#include "core/game/game_manager.h"
#include "core/sdk/offsets.h"
#include <chrono>
#include <imgui.h>
#include <sstream>
#include <windows.h>

namespace Features {

DebugConfig debugConfig;

using Clock = std::chrono::steady_clock;

static Clock::time_point s_lastFrame = Clock::now();
static float s_fps = 0.0f;
static float s_espMs = 0.0f;

// Called from esp.cpp to report render time
void DebugOverlay_SetEspMs(float ms) { s_espMs = ms; }

void DebugOverlay::Update() {
  auto now = Clock::now();
  float dt = std::chrono::duration<float>(now - s_lastFrame).count();
  s_lastFrame = now;
  if (dt > 0.0f)
    s_fps = 0.9f * s_fps + 0.1f * (1.0f / dt); // smooth
}

void DebugOverlay::Render(Render::DrawList & /*drawList*/) {
  if (!debugConfig.enabled)
    return;

  const float PAD = 8.0f;
  ImGuiIO &io = ImGui::GetIO();

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
  ImGui::TextColored(white, "%.0f", s_fps);

  auto &players = Core::GameManager::GetPlayers();
  ImGui::TextColored(yellow, "Entities");
  ImGui::SameLine(60);
  ImGui::TextColored(white, "%zu", players.size());

  ImGuiIO &disp = ImGui::GetIO();
  ImGui::TextColored(yellow, "Screen");
  ImGui::SameLine(60);
  ImGui::TextColored(grey, "%.0fx%.0f", disp.DisplaySize.x, disp.DisplaySize.y);

  // ── Developer mode (extra data) ──
  if (debugConfig.devMode) {
    ImGui::Separator();
    ImGui::TextColored(cyan, "-- DEV --");

    ImGui::TextColored(yellow, "ESP ms");
    ImGui::SameLine(60);
    ImGui::TextColored(white, "%.2f", s_espMs);

    uintptr_t base = Core::GameManager::GetClientBase();
    ImGui::TextColored(yellow, "client.dll");
    ImGui::SameLine(60);
    ImGui::TextColored(grey, "0x%llX", (unsigned long long)base);

    ImGui::TextColored(yellow, "EntityList");
    ImGui::SameLine(60);
    ImGui::TextColored(grey, "0x%llX",
                       (unsigned long long)SDK::Offsets::dwEntityList);

    ImGui::TextColored(yellow, "ViewMatrix");
    ImGui::SameLine(60);
    ImGui::TextColored(grey, "0x%llX",
                       (unsigned long long)SDK::Offsets::dwViewMatrix);
  }

  ImGui::End();
}

} // namespace Features
