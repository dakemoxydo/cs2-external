#pragma once
#include "esp.h"
#include "esp_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void Esp::RenderUI() {
    UI::SettingToggle("Enable ESP", &Config::Settings.esp.enabled);
    UI::SettingToggle("Team ESP", &Config::Settings.esp.showTeammates);

    ImGui::Spacing();
    UI::SettingToggle("Draw Box", &Config::Settings.esp.showBox);
    UI::SettingColor("##box_color", Config::Settings.esp.boxColor);
    if (Config::Settings.esp.showBox) {
        const char *boxStyles[] = {"Rect", "Corners", "Filled"};
        int bsInt = static_cast<int>(Config::Settings.esp.boxStyle);
        if (ImGui::Combo("Box Style", &bsInt, boxStyles, 3))
            Config::Settings.esp.boxStyle = static_cast<BoxStyle>(bsInt);
        if (Config::Settings.esp.boxStyle == BoxStyle::Filled)
            ImGui::SliderFloat("Fill Alpha", &Config::Settings.esp.fillBoxAlpha, 0.02f, 0.5f, "%.2f");
    }

    UI::SettingToggle("Draw Health Bar", &Config::Settings.esp.showHealth);
    if (Config::Settings.esp.showHealth) {
        const char *hpStyles[] = {"Side", "Bottom"};
        int hsInt = static_cast<int>(Config::Settings.esp.healthBarStyle);
        if (ImGui::Combo("Bar Style", &hsInt, hpStyles, 2))
            Config::Settings.esp.healthBarStyle = static_cast<HealthBarStyle>(hsInt);
    }

    UI::SettingToggle("Draw Name", &Config::Settings.esp.showName);
    UI::SettingColor("##name_color", Config::Settings.esp.nameColor);

    UI::SettingToggle("Draw Weapon", &Config::Settings.esp.showWeapon);
    UI::SettingColor("##weap_color", Config::Settings.esp.weaponColor);

    UI::SettingToggle("Draw Distance", &Config::Settings.esp.showDistance);

    UI::SettingToggle("Snap Lines", &Config::Settings.esp.showSnapLines);
    UI::SettingColor("##snap_color", Config::Settings.esp.snapLineColor);

    UI::SettingToggle("Skeleton", &Config::Settings.esp.showBones);
    UI::SettingColor("##bone_color", Config::Settings.esp.boneColor);
    if (Config::Settings.esp.showBones) {
        UI::SettingToggle("Skeleton Outline", &Config::Settings.esp.skeletonOutline);
        UI::SettingColor("##skel_out_color", Config::Settings.esp.skeletonOutlineColor);
    }
}

} // namespace Features
