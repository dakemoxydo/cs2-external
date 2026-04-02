#pragma once
#include "aimbot.h"
#include "aimbot_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void Aimbot::RenderUI() {
    UI::SettingToggle("Enable Aimbot", &Config::Settings.aimbot.enabled);
    if (Config::Settings.aimbot.enabled) {
        ImGui::Spacing();
        UI::SettingHotkey("Hold Key Aimbot", Config::Settings.aimbot.hotkey);
        ImGui::Spacing();
        ImGui::SliderFloat("FOV", &Config::Settings.aimbot.fov, 1.0f, 30.0f, "%.1f deg");
        ImGui::SliderFloat("Smooth", &Config::Settings.aimbot.smooth, 1.0f, 20.0f, "%.1f");
        ImGui::InputFloat("Sensitivity (In-Game)", &Config::Settings.aimbot.sensitivity, 0.01f, 0.1f, "%.3f");
        ImGui::SliderFloat("Jitter", &Config::Settings.aimbot.jitter, 0.0f, 0.15f, "%.3f");

        ImGui::Separator();
        const char *bones[] = {"Pelvis", "Chest", "Neck", "Head"};
        const int bVals[] = {0, 4, 5, 6};
        int sel = 3;
        for (int i = 0; i < 4; i++)
            if (bVals[i] == Config::Settings.aimbot.targetBone) { sel = i; break; }
        if (ImGui::Combo("Target Bone", &sel, bones, 4))
            Config::Settings.aimbot.targetBone = bVals[sel];

        UI::SettingToggle("Target Lock", &Config::Settings.aimbot.targetLock);
        UI::SettingToggle("Visible Only", &Config::Settings.aimbot.visibleOnly);
        UI::SettingToggle("Team Check Aimbot", &Config::Settings.aimbot.teamCheck);
        UI::SettingToggle("Only Scoped", &Config::Settings.aimbot.onlyScoped);
        UI::SettingToggle("Show FOV Circle", &Config::Settings.aimbot.showFov);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered], "Recoil Control System (Standalone)");
        UI::SettingToggle("Enable RCS System", &Config::Settings.rcs.enabled);
        if (Config::Settings.rcs.enabled) {
            ImGui::SliderFloat("Pitch Strength", &Config::Settings.rcs.pitchStrength, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Yaw Strength", &Config::Settings.rcs.yawStrength, 0.0f, 2.0f, "%.2f");
            ImGui::SliderInt("Start at Bullet", &Config::Settings.rcs.startBullet, 1, 10);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
            ImGui::TextWrapped("Note: RCS is disabled while Aimbot keys are held to prevent conflict.");
            ImGui::PopStyleColor();
        }
    }
}

} // namespace Features
