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

void RenderAimbotCoreCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("AimbotCore")) {
    return;
  }

  UI::SectionHeader("Legitbot", "Primary aim assist controls.");

  bool aimbotEnabled = settings.aimbot.enabled;
  if (UI::SettingToggle("Master Enable", &aimbotEnabled)) {
    Commit([&](auto &state) { state.aimbot.enabled = aimbotEnabled; });
    settings.aimbot.enabled = aimbotEnabled;
  }

  int aimbotHotkey = settings.aimbot.hotkey;
  if (UI::SettingHotkey("Aim Key", aimbotHotkey)) {
    Commit([&](auto &state) { state.aimbot.hotkey = aimbotHotkey; });
    settings.aimbot.hotkey = aimbotHotkey;
  }

  if (settings.aimbot.enabled) {
    float fov = settings.aimbot.fov;
    if (ImGui::SliderFloat("Aim FOV", &fov, 1.0f, 30.0f, "%.1f deg")) {
      Commit([&](auto &state) { state.aimbot.fov = fov; });
      settings.aimbot.fov = fov;
    }

    float smooth = settings.aimbot.smooth;
    if (ImGui::SliderFloat("Aim Smooth", &smooth, 1.0f, 20.0f, "%.1f")) {
      Commit([&](auto &state) { state.aimbot.smooth = smooth; });
      settings.aimbot.smooth = smooth;
    }

    float sensitivity = settings.aimbot.sensitivity;
    if (ImGui::SliderFloat("Sensitivity", &sensitivity, 0.10f, 10.0f,
                           "%.2f")) {
      Commit([&](auto &state) { state.aimbot.sensitivity = sensitivity; });
      settings.aimbot.sensitivity = sensitivity;
    }

    float jitter = settings.aimbot.jitter;
    if (ImGui::SliderFloat("Jitter", &jitter, 0.0f, 0.15f, "%.3f")) {
      Commit([&](auto &state) { state.aimbot.jitter = jitter; });
      settings.aimbot.jitter = jitter;
    }
  }

  UI::EndCard();
}

void RenderAimbotRulesCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("AimbotRules")) {
    return;
  }

  UI::SectionHeader("Target Rules", "Filtering and lock-on behavior.");

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
  if (UI::SettingToggle("FOV Preview", &showFov)) {
    Commit([&](auto &state) { state.aimbot.showFov = showFov; });
    settings.aimbot.showFov = showFov;
  }

  UI::EndCard();
}

void RenderTriggerbotCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("Triggerbot")) {
    return;
  }

  UI::SectionHeader("Triggerbot", "Reaction timing and shot release logic.");

  bool triggerbotEnabled = settings.triggerbot.enabled;
  if (UI::SettingToggle("Master Enable", &triggerbotEnabled)) {
    Commit([&](auto &state) { state.triggerbot.enabled = triggerbotEnabled; });
    settings.triggerbot.enabled = triggerbotEnabled;
  }

  int triggerHotkey = settings.triggerbot.hotkey;
  if (UI::SettingHotkey("Trigger Key", triggerHotkey)) {
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
    if (ImGui::SliderInt("Max Delay", &maxDelay, settings.triggerbot.delayMin,
                         300, "%d ms")) {
      Commit([&](auto &state) { state.triggerbot.delayMax = maxDelay; });
      settings.triggerbot.delayMax = maxDelay;
    }

    bool triggerTeamCheck = settings.triggerbot.teamCheck;
    if (UI::SettingToggle("Team Check", &triggerTeamCheck)) {
      Commit([&](auto &state) { state.triggerbot.teamCheck = triggerTeamCheck; });
      settings.triggerbot.teamCheck = triggerTeamCheck;
    }
  }

  UI::EndCard();
}

void RenderTriggerbotAssistCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("TriggerAssist")) {
    return;
  }

  UI::SectionHeader("Shot Behavior", "Trigger pacing for cleaner legit movement.");

  if (settings.triggerbot.enabled) {
    const int delaySpread =
        settings.triggerbot.delayMax - settings.triggerbot.delayMin;
    ImGui::TextDisabled("Spread");
    ImGui::Text("%d ms", delaySpread);
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    UI::HelpText(
        "Keep the spread low for snappy reactions or widen it for more natural timing.");
  } else {
    UI::HelpText("Enable Triggerbot to inspect timing behavior.");
  }

  UI::EndCard();
}

void RenderRcsCoreCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("RcsCore")) {
    return;
  }

  UI::SectionHeader("Recoil Control", "Compensation strength and activation.");

  bool rcsEnabled = settings.rcs.enabled;
  if (UI::SettingToggle("Master Enable", &rcsEnabled)) {
    Commit([&](auto &state) { state.rcs.enabled = rcsEnabled; });
    settings.rcs.enabled = rcsEnabled;
  }

  if (settings.rcs.enabled) {
    int rcsKey = settings.rcs.key;
    if (UI::SettingHotkey("RCS Key", rcsKey)) {
      Commit([&](auto &state) { state.rcs.key = rcsKey; });
      settings.rcs.key = rcsKey;
    }

    float pitchStrength = settings.rcs.pitchStrength;
    if (ImGui::SliderFloat("Pitch Strength", &pitchStrength, 0.0f, 2.0f,
                           "%.2f")) {
      Commit([&](auto &state) { state.rcs.pitchStrength = pitchStrength; });
      settings.rcs.pitchStrength = pitchStrength;
    }

    float yawStrength = settings.rcs.yawStrength;
    if (ImGui::SliderFloat("Yaw Strength", &yawStrength, 0.0f, 2.0f,
                           "%.2f")) {
      Commit([&](auto &state) { state.rcs.yawStrength = yawStrength; });
      settings.rcs.yawStrength = yawStrength;
    }
  }

  UI::EndCard();
}

void RenderRcsResponseCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("RcsResponse")) {
    return;
  }

  UI::SectionHeader("Response", "How quickly the controller settles recoil.");

  if (settings.rcs.enabled) {
    float smooth = settings.rcs.smooth;
    if (ImGui::SliderFloat("Smooth", &smooth, 1.0f, 100.0f, "%.1f")) {
      Commit([&](auto &state) { state.rcs.smooth = smooth; });
      settings.rcs.smooth = smooth;
    }

    int startBullet = settings.rcs.startBullet;
    if (ImGui::SliderInt("Start Bullet", &startBullet, 1, 10)) {
      Commit([&](auto &state) { state.rcs.startBullet = startBullet; });
      settings.rcs.startBullet = startBullet;
    }
  } else {
    UI::HelpText("Enable recoil control to tune response behavior.");
  }

  UI::EndCard();
}

} // namespace

void RenderTabLegit(int subTab) {
  const float gap = 14.0f;
  const float totalWidth = ImGui::GetContentRegionAvail().x;
  const float columnWidth = (totalWidth - gap) * 0.5f;
  Config::GlobalSettings settings = Config::CopySettings();

  ImGui::BeginChild("LegitLeftColumn", ImVec2(columnWidth, 0.0f), false,
                    ImGuiWindowFlags_NoScrollbar);
  if (subTab == 0) {
    RenderAimbotCoreCard(settings);
  } else if (subTab == 1) {
    RenderTriggerbotCard(settings);
  } else {
    RenderRcsCoreCard(settings);
  }
  ImGui::EndChild();

  ImGui::SameLine(0.0f, gap);

  ImGui::BeginChild("LegitRightColumn", ImVec2(0.0f, 0.0f), false,
                    ImGuiWindowFlags_NoScrollbar);
  if (subTab == 0) {
    RenderAimbotRulesCard(settings);
  } else if (subTab == 1) {
    RenderTriggerbotAssistCard(settings);
  } else {
    RenderRcsResponseCard(settings);
  }
  ImGui::EndChild();
}

} // namespace Render
