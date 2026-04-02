#pragma once
#include "radar.h"
#include "radar_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void Radar::RenderUI() {
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

} // namespace Features
