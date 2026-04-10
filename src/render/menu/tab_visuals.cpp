#include "tab_visuals.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "features/esp/esp_config.h"
#include <imgui.h>

namespace Render {

void RenderTabVisuals() {
    ImGui::Columns(2, "VisCols", false);

    if (UI::BeginCard("Player ESP")) {
        UI::SectionHeader("Visibility", "Main player overlay controls and box presentation.");
        UI::SettingToggle("Enable ESP", &Config::Settings.esp.enabled);
        if (Config::Settings.esp.enabled) {
            UI::SettingToggle("Show Teammates", &Config::Settings.esp.showTeammates);
            UI::SettingToggle("Draw Box", &Config::Settings.esp.showBox);

            if (Config::Settings.esp.showBox) {
                UI::SettingColor("##box_color", Config::Settings.esp.boxColor);
                const char *boxStyles[] = {"Rect", "Corners", "Filled"};
                int boxStyle = static_cast<int>(Config::Settings.esp.boxStyle);
                if (ImGui::Combo("Box Style", &boxStyle, boxStyles, 3)) {
                    Config::Settings.esp.boxStyle = static_cast<Features::BoxStyle>(boxStyle);
                    Config::ConfigManager::ApplySettings();
                }
                if (Config::Settings.esp.boxStyle == Features::BoxStyle::Filled &&
                    ImGui::SliderFloat("Fill Alpha", &Config::Settings.esp.fillBoxAlpha, 0.02f, 0.5f, "%.2f")) {
                    Config::ConfigManager::ApplySettings();
                }
            }

            UI::SectionHeader("Labels", "Name, weapon, distance, health, and helper lines.");
            UI::SettingToggle("Health Bar", &Config::Settings.esp.showHealth);
            if (Config::Settings.esp.showHealth) {
                const char *hpStyles[] = {"Side", "Bottom"};
                int healthStyle = static_cast<int>(Config::Settings.esp.healthBarStyle);
                if (ImGui::Combo("Health Layout", &healthStyle, hpStyles, 2)) {
                    Config::Settings.esp.healthBarStyle = static_cast<Features::HealthBarStyle>(healthStyle);
                    Config::ConfigManager::ApplySettings();
                }
                UI::SettingToggle("Show Health Text", &Config::Settings.esp.showHealthText);
            }

            UI::SettingToggle("Show Name", &Config::Settings.esp.showName);
            if (Config::Settings.esp.showName) {
                UI::SettingColor("##name_color", Config::Settings.esp.nameColor);
            }
            UI::SettingToggle("Show Weapon", &Config::Settings.esp.showWeapon);
            if (Config::Settings.esp.showWeapon) {
                UI::SettingColor("##weapon_color", Config::Settings.esp.weaponColor);
            }
            UI::SettingToggle("Show Distance", &Config::Settings.esp.showDistance);
            UI::SettingToggle("Snap Lines", &Config::Settings.esp.showSnapLines);
            if (Config::Settings.esp.showSnapLines) {
                UI::SettingColor("##snap_color", Config::Settings.esp.snapLineColor);
            }

            UI::SectionHeader("Skeleton", "Bone rendering and distance-aware detail.");
            UI::SettingToggle("Draw Skeleton", &Config::Settings.esp.showBones);
            if (Config::Settings.esp.showBones) {
                UI::SettingColor("##bone_color", Config::Settings.esp.boneColor);
                UI::SettingToggle("Skeleton Outline", &Config::Settings.esp.skeletonOutline);
                if (Config::Settings.esp.skeletonOutline) {
                    UI::SettingColor("##skel_out_color", Config::Settings.esp.skeletonOutlineColor);
                }
                if (ImGui::SliderFloat("Skeleton Max Distance", &Config::Settings.esp.skeletonMaxDistance, 0.0f, 100.0f, "%.0f m")) {
                    Config::ConfigManager::ApplySettings();
                }
            }
        } else {
            UI::HelpText("Enable ESP to configure player visibility, labels, and skeleton rendering.");
        }
    }
    UI::EndCard();

    ImGui::NextColumn();

    if (UI::BeginCard("World And Motion")) {
        UI::SectionHeader("Frustum And Off-Screen", "Keep the overlay readable when targets leave the camera frame.");
        UI::SettingToggle("Frustum Culling", &Config::Settings.esp.frustumCullingEnabled);
        UI::SettingToggle("Off-Screen Arrows", &Config::Settings.esp.showOffscreen);
        if (Config::Settings.esp.showOffscreen) {
            UI::SettingColor("##offscreen_color", Config::Settings.esp.offscreenColor);
        }

        UI::SectionHeader("Radar", "Compact world overview for positional awareness.");
        UI::SettingToggle("Enable Radar", &Config::Settings.radar.enabled);
        if (Config::Settings.radar.enabled) {
            UI::SettingToggle("Rotate Map", &Config::Settings.radar.rotate);
            UI::SettingToggle("Show Teammates", &Config::Settings.radar.showTeammates);
            UI::SettingToggle("Visible Check", &Config::Settings.radar.visibleCheck);

            const char *maps[] = {"Custom", "Mirage", "Dust2", "Inferno", "Nuke"};
            if (ImGui::Combo("Map Profile", &Config::Settings.radar.mapIndex, maps, 5)) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Zoom", &Config::Settings.radar.zoom, 0.1f, 3.0f, "%.2f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Point Size", &Config::Settings.radar.pointSize, 2.0f, 8.0f, "%.1f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Background Alpha", &Config::Settings.radar.bgAlpha, 0.05f, 1.0f, "%.2f")) {
                Config::ConfigManager::ApplySettings();
            }
        }

        UI::SectionHeader("Footsteps ESP", "Animated circles for movement events and landings.");
        UI::SettingToggle("Enable Footsteps ESP", &Config::Settings.footstepsEsp.enabled);
        if (Config::Settings.footstepsEsp.enabled) {
            UI::SettingToggle("Show Teammates", &Config::Settings.footstepsEsp.showTeammates);
            UI::SettingColor("##footstep_color", Config::Settings.footstepsEsp.footstepColor);
            ImGui::SameLine();
            ImGui::TextUnformatted("Footstep");
            UI::SettingColor("##jump_color", Config::Settings.footstepsEsp.jumpColor);
            ImGui::SameLine();
            ImGui::TextUnformatted("Jump");
            UI::SettingColor("##land_color", Config::Settings.footstepsEsp.landColor);
            ImGui::SameLine();
            ImGui::TextUnformatted("Land");

            if (ImGui::SliderFloat("Footstep Radius", &Config::Settings.footstepsEsp.footstepMaxRadius, 10.0f, 60.0f, "%.0f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Jump Radius", &Config::Settings.footstepsEsp.jumpMaxRadius, 15.0f, 80.0f, "%.0f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Land Radius", &Config::Settings.footstepsEsp.landMaxRadius, 20.0f, 100.0f, "%.0f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Expand Time", &Config::Settings.footstepsEsp.expandDuration, 0.1f, 2.0f, "%.1f s")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Fade Time", &Config::Settings.footstepsEsp.fadeDuration, 0.3f, 3.0f, "%.1f s")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Thickness", &Config::Settings.footstepsEsp.thickness, 1.0f, 5.0f, "%.1f")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderInt("Segments", &Config::Settings.footstepsEsp.segments, 16, 64)) {
                Config::ConfigManager::ApplySettings();
            }
        } else {
            UI::HelpText("Enable footsteps ESP to reveal teammate filters, colors, and ring behavior.");
        }

        UI::SectionHeader("World Alerts", "Simple round-state helpers.");
        UI::SettingToggle("Bomb Timer", &Config::Settings.bomb.enabled);
    }
    UI::EndCard();

    ImGui::Columns(1);
}

} // namespace Render
