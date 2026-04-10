#include "tab_legit.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include <imgui.h>

namespace Render {

void RenderTabLegit() {
    ImGui::Columns(2, "LegitCols", false);

    if (UI::BeginCard("Aimbot")) {
        UI::SectionHeader("Core", "Main target acquisition and motion tuning.");
        UI::SettingToggle("Enable Aimbot", &Config::Settings.aimbot.enabled);
        UI::SettingHotkey("Aim Hold Key", Config::Settings.aimbot.hotkey);

        if (Config::Settings.aimbot.enabled) {
            if (ImGui::SliderFloat("FOV", &Config::Settings.aimbot.fov, 1.0f, 30.0f, "%.1f deg")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Smooth", &Config::Settings.aimbot.smooth, 1.0f, 20.0f, "%.1f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Sensitivity", &Config::Settings.aimbot.sensitivity, 0.10f, 10.0f, "%.2f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Jitter", &Config::Settings.aimbot.jitter, 0.0f, 0.15f, "%.3f")) {
                Config::ConfigManager::ApplySettings();
            }

            UI::SectionHeader("Target Rules", "Filters that decide which target is allowed.");
            const char *bones[] = {"Pelvis", "Chest", "Neck", "Head"};
            const int boneValues[] = {0, 4, 5, 6};
            int selectedBone = 3;
            for (int i = 0; i < 4; ++i) {
                if (boneValues[i] == Config::Settings.aimbot.targetBone) {
                    selectedBone = i;
                    break;
                }
            }
            if (ImGui::Combo("Target Bone", &selectedBone, bones, 4)) {
                Config::Settings.aimbot.targetBone = boneValues[selectedBone];
                Config::ConfigManager::ApplySettings();
            }

            UI::SettingToggle("Target Lock", &Config::Settings.aimbot.targetLock);
            UI::SettingToggle("Visible Only", &Config::Settings.aimbot.visibleOnly);
            UI::SettingToggle("Team Check", &Config::Settings.aimbot.teamCheck);
            UI::SettingToggle("Only Scoped", &Config::Settings.aimbot.onlyScoped);
            UI::SettingToggle("Show FOV Circle", &Config::Settings.aimbot.showFov);
        } else {
            UI::HelpText("Enable the aimbot to unlock target rules and motion tuning.");
        }
    }
    UI::EndCard();

    ImGui::NextColumn();

    if (UI::BeginCard("Triggerbot")) {
        UI::SectionHeader("Reaction", "Fire only when a valid target enters your crosshair.");
        UI::SettingToggle("Enable Triggerbot", &Config::Settings.triggerbot.enabled);
        UI::SettingHotkey("Trigger Hold Key", Config::Settings.triggerbot.hotkey);

        if (Config::Settings.triggerbot.enabled) {
            if (ImGui::SliderInt("Min Delay", &Config::Settings.triggerbot.delayMin, 0, 150, "%d ms")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderInt("Max Delay", &Config::Settings.triggerbot.delayMax, Config::Settings.triggerbot.delayMin, 300, "%d ms")) {
                Config::ConfigManager::ApplySettings();
            }
            UI::SettingToggle("Team Check", &Config::Settings.triggerbot.teamCheck);
        } else {
            UI::HelpText("Use triggerbot for simple reaction shots without manual click timing.");
        }

        UI::SectionHeader("Recoil Control", "Standalone recoil compensation when you want spray support without full aim assist.");
        UI::SettingToggle("Enable RCS", &Config::Settings.rcs.enabled);
        if (Config::Settings.rcs.enabled) {
            if (ImGui::SliderFloat("Pitch Strength", &Config::Settings.rcs.pitchStrength, 0.0f, 2.0f, "%.2f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Yaw Strength", &Config::Settings.rcs.yawStrength, 0.0f, 2.0f, "%.2f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderInt("Start Bullet", &Config::Settings.rcs.startBullet, 1, 10)) {
                Config::ConfigManager::ApplySettings();
            }
            UI::HelpText("RCS is automatically suppressed while the aimbot actively drives the mouse.");
        }
    }
    UI::EndCard();

    ImGui::Columns(1);
}

} // namespace Render
