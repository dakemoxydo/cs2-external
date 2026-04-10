#pragma once
#include "footsteps_esp.h"
#include "footsteps_esp_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void FootstepsEsp::RenderUI() {
    UI::SettingToggle("Enable Footsteps ESP", &Config::Settings.footstepsEsp.enabled);
    UI::SettingToggle("Show on Teammates", &Config::Settings.footstepsEsp.showTeammates);

    ImGui::Spacing();
    UI::SettingColor("##footstep_color", Config::Settings.footstepsEsp.footstepColor);
    ImGui::SameLine();
    ImGui::Text("Footstep");

    UI::SettingColor("##jump_color", Config::Settings.footstepsEsp.jumpColor);
    ImGui::SameLine();
    ImGui::Text("Jump");

    UI::SettingColor("##land_color", Config::Settings.footstepsEsp.landColor);
    ImGui::SameLine();
    ImGui::Text("Land");

    ImGui::Spacing();
    ImGui::SliderFloat("Footstep Radius", &Config::Settings.footstepsEsp.footstepMaxRadius, 10.0f, 60.0f, "%.0f");
    ImGui::SliderFloat("Jump Radius", &Config::Settings.footstepsEsp.jumpMaxRadius, 15.0f, 80.0f, "%.0f");
    ImGui::SliderFloat("Land Radius", &Config::Settings.footstepsEsp.landMaxRadius, 20.0f, 100.0f, "%.0f");

    ImGui::Spacing();
    ImGui::SliderFloat("Expand Time", &Config::Settings.footstepsEsp.expandDuration, 0.1f, 2.0f, "%.1fs");
    ImGui::SliderFloat("Fade Time", &Config::Settings.footstepsEsp.fadeDuration, 0.3f, 3.0f, "%.1fs");

    ImGui::Spacing();
    ImGui::SliderFloat("Thickness", &Config::Settings.footstepsEsp.thickness, 1.0f, 5.0f, "%.1f");
    ImGui::SliderInt("Segments", &Config::Settings.footstepsEsp.segments, 16, 64, "%d");
}

} // namespace Features
