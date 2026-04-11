#include "tab_legit.h"
#include "ui_components.h"
#include "config/settings.h"
#include <algorithm>
#include <imgui.h>
#include <utility>

namespace Render {

namespace {

template <typename Fn>
void Commit(Fn &&fn) {
  Config::MutateSettingsVoid(std::forward<Fn>(fn));
}

} // namespace

void RenderTabLegit() {
  Config::GlobalSettings settings = Config::CopySettings();

  if (UI::BeginCard("Aimbot")) {
    UI::SectionHeader("Core");

    bool aimbotEnabled = settings.aimbot.enabled;
    if (UI::SettingToggle("Enable Aimbot", &aimbotEnabled)) {
      Commit([&](auto &state) { state.aimbot.enabled = aimbotEnabled; });
      settings.aimbot.enabled = aimbotEnabled;
    }

    int aimbotHotkey = settings.aimbot.hotkey;
    if (UI::SettingHotkey("Aim Hold Key", aimbotHotkey)) {
      Commit([&](auto &state) { state.aimbot.hotkey = aimbotHotkey; });
      settings.aimbot.hotkey = aimbotHotkey;
    }

    if (settings.aimbot.enabled) {
      float fov = settings.aimbot.fov;
      if (ImGui::SliderFloat("FOV", &fov, 1.0f, 30.0f, "%.1f deg")) {
        Commit([&](auto &state) { state.aimbot.fov = fov; });
        settings.aimbot.fov = fov;
      }

      float smooth = settings.aimbot.smooth;
      if (ImGui::SliderFloat("Smooth", &smooth, 1.0f, 20.0f, "%.1f")) {
        Commit([&](auto &state) { state.aimbot.smooth = smooth; });
        settings.aimbot.smooth = smooth;
      }

      float sensitivity = settings.aimbot.sensitivity;
      if (ImGui::SliderFloat("Sensitivity", &sensitivity, 0.10f, 10.0f, "%.2f")) {
        Commit([&](auto &state) { state.aimbot.sensitivity = sensitivity; });
        settings.aimbot.sensitivity = sensitivity;
      }

      float jitter = settings.aimbot.jitter;
      if (ImGui::SliderFloat("Jitter", &jitter, 0.0f, 0.15f, "%.3f")) {
        Commit([&](auto &state) { state.aimbot.jitter = jitter; });
        settings.aimbot.jitter = jitter;
      }

      UI::SectionHeader("Target Rules");
      const char *bones[] = {"Pelvis", "Chest", "Neck", "Head"};
      const int boneValues[] = {0, 4, 5, 6};
      int selectedBone = 3;
      for (int i = 0; i < 4; ++i) {
        if (boneValues[i] == settings.aimbot.targetBone) {
          selectedBone = i;
          break;
        }
      }
      if (ImGui::Combo("Target Bone", &selectedBone, bones, 4)) {
        const int targetBone = boneValues[selectedBone];
        Commit([&](auto &state) { state.aimbot.targetBone = targetBone; });
        settings.aimbot.targetBone = targetBone;
      }

      bool targetLock = settings.aimbot.targetLock;
      if (UI::SettingToggle("Target Lock", &targetLock)) {
        Commit([&](auto &state) { state.aimbot.targetLock = targetLock; });
        settings.aimbot.targetLock = targetLock;
      }

      bool visibleOnly = settings.aimbot.visibleOnly;
      if (UI::SettingToggle("Visible Only", &visibleOnly)) {
        Commit([&](auto &state) { state.aimbot.visibleOnly = visibleOnly; });
        settings.aimbot.visibleOnly = visibleOnly;
      }

      bool teamCheck = settings.aimbot.teamCheck;
      if (UI::SettingToggle("Team Check", &teamCheck)) {
        Commit([&](auto &state) { state.aimbot.teamCheck = teamCheck; });
        settings.aimbot.teamCheck = teamCheck;
      }

      bool onlyScoped = settings.aimbot.onlyScoped;
      if (UI::SettingToggle("Only Scoped", &onlyScoped)) {
        Commit([&](auto &state) { state.aimbot.onlyScoped = onlyScoped; });
        settings.aimbot.onlyScoped = onlyScoped;
      }

      bool showFov = settings.aimbot.showFov;
      if (UI::SettingToggle("Show FOV Circle", &showFov)) {
        Commit([&](auto &state) { state.aimbot.showFov = showFov; });
        settings.aimbot.showFov = showFov;
      }
    }
  }
  UI::EndCard();

  if (UI::BeginCard("Triggerbot")) {
    UI::SectionHeader("Reaction");

    bool triggerbotEnabled = settings.triggerbot.enabled;
    if (UI::SettingToggle("Enable Triggerbot", &triggerbotEnabled)) {
      Commit([&](auto &state) { state.triggerbot.enabled = triggerbotEnabled; });
      settings.triggerbot.enabled = triggerbotEnabled;
    }

    int triggerHotkey = settings.triggerbot.hotkey;
    if (UI::SettingHotkey("Trigger Hold Key", triggerHotkey)) {
      Commit([&](auto &state) { state.triggerbot.hotkey = triggerHotkey; });
      settings.triggerbot.hotkey = triggerHotkey;
    }

    if (settings.triggerbot.enabled) {
      int minDelay = settings.triggerbot.delayMin;
      if (ImGui::SliderInt("Min Delay", &minDelay, 0, 150, "%d ms")) {
        const int clampedMax = (std::max)(settings.triggerbot.delayMax, minDelay);
        Commit([&](auto &state) {
          state.triggerbot.delayMin = minDelay;
          state.triggerbot.delayMax = (std::max)(state.triggerbot.delayMax, minDelay);
        });
        settings.triggerbot.delayMin = minDelay;
        settings.triggerbot.delayMax = clampedMax;
      }

      int maxDelay = settings.triggerbot.delayMax;
      if (ImGui::SliderInt("Max Delay", &maxDelay, settings.triggerbot.delayMin, 300,
                           "%d ms")) {
        Commit([&](auto &state) { state.triggerbot.delayMax = maxDelay; });
        settings.triggerbot.delayMax = maxDelay;
      }

      bool triggerTeamCheck = settings.triggerbot.teamCheck;
      if (UI::SettingToggle("Team Check", &triggerTeamCheck)) {
        Commit([&](auto &state) { state.triggerbot.teamCheck = triggerTeamCheck; });
        settings.triggerbot.teamCheck = triggerTeamCheck;
      }
    }

    UI::SectionHeader("Recoil");

    bool rcsEnabled = settings.rcs.enabled;
    if (UI::SettingToggle("Enable RCS", &rcsEnabled)) {
      Commit([&](auto &state) { state.rcs.enabled = rcsEnabled; });
      settings.rcs.enabled = rcsEnabled;
    }

    if (settings.rcs.enabled) {
      float pitchStrength = settings.rcs.pitchStrength;
      if (ImGui::SliderFloat("Pitch Strength", &pitchStrength, 0.0f, 2.0f, "%.2f")) {
        Commit([&](auto &state) { state.rcs.pitchStrength = pitchStrength; });
        settings.rcs.pitchStrength = pitchStrength;
      }

      float yawStrength = settings.rcs.yawStrength;
      if (ImGui::SliderFloat("Yaw Strength", &yawStrength, 0.0f, 2.0f, "%.2f")) {
        Commit([&](auto &state) { state.rcs.yawStrength = yawStrength; });
        settings.rcs.yawStrength = yawStrength;
      }

      int startBullet = settings.rcs.startBullet;
      if (ImGui::SliderInt("Start Bullet", &startBullet, 1, 10)) {
        Commit([&](auto &state) { state.rcs.startBullet = startBullet; });
        settings.rcs.startBullet = startBullet;
      }
    }
  }
  UI::EndCard();
}

} // namespace Render
