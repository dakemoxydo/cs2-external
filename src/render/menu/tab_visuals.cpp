#include "tab_visuals.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "features/feature_manager.h"
#include <imgui.h>

namespace Render {

void RenderTabVisuals() {
    ImGui::Columns(2, "VisCols", false);

    // ESP card
    if (UI::BeginCard("Player ESP")) {
        UI::SettingToggle("Enable ESP", &Config::Settings.esp.enabled);
        UI::SettingToggle("Team ESP", &Config::Settings.esp.showTeammates);

        ImGui::Spacing();
        UI::SettingToggle("Draw Box", &Config::Settings.esp.showBox);
        UI::SettingColor("##box_color", Config::Settings.esp.boxColor);
        if (Config::Settings.esp.showBox) {
            const char *boxStyles[] = {"Rect", "Corners", "Filled"};
            int bsInt = static_cast<int>(Config::Settings.esp.boxStyle);
            if (ImGui::Combo("Box Style", &bsInt, boxStyles, 3))
                Config::Settings.esp.boxStyle = static_cast<Features::BoxStyle>(bsInt);
            if (Config::Settings.esp.boxStyle == Features::BoxStyle::Filled)
                ImGui::SliderFloat("Fill Alpha", &Config::Settings.esp.fillBoxAlpha, 0.02f, 0.5f, "%.2f");
        }

        UI::SettingToggle("Draw Health Bar", &Config::Settings.esp.showHealth);
        if (Config::Settings.esp.showHealth) {
            const char *hpStyles[] = {"Side", "Bottom"};
            int hsInt = static_cast<int>(Config::Settings.esp.healthBarStyle);
            if (ImGui::Combo("Bar Style", &hsInt, hpStyles, 2))
                Config::Settings.esp.healthBarStyle = static_cast<Features::HealthBarStyle>(hsInt);
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
    UI::EndCard();

    ImGui::NextColumn();

    // Radar card
    if (UI::BeginCard("Radar Overlay", ImVec2(0, ImGui::GetWindowHeight() * 0.65f))) {
        UI::SettingToggle("Enable Radar", &Config::Settings.radar.enabled);
        if (Config::Settings.radar.enabled) {
            UI::SettingToggle("Rotate Map", &Config::Settings.radar.rotate);
            UI::SettingToggle("Show Teammates Radar", &Config::Settings.radar.showTeammates);
            UI::SettingToggle("Visible Check Radar", &Config::Settings.radar.visibleCheck);

            const char *maps[] = {"Custom", "Mirage", "Dust2", "Inferno", "Nuke"};
            ImGui::Combo("Map", &Config::Settings.radar.mapIndex, maps, 5);
            ImGui::SliderFloat("Zoom", &Config::Settings.radar.zoom, 0.1f, 3.0f, "%.2f");
            ImGui::SliderFloat("Point Size", &Config::Settings.radar.pointSize, 2.0f, 8.0f, "%.1f");
        }
    }
    UI::EndCard();

    // World Options
    if (UI::BeginCard("World Options")) {
        UI::SettingToggle("Enable Bomb Timer", &Config::Settings.bomb.enabled);
    }
    UI::EndCard();

    ImGui::Columns(1);
}

} // namespace Render
