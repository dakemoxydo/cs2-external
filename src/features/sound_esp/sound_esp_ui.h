#pragma once
#include "sound_esp.h"
#include "sound_esp_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void SoundEsp::RenderUI() {
    UI::SettingToggle("Enable Sound ESP", &Config::Settings.soundEsp.enabled);
    UI::SettingToggle("Show on Teammates", &Config::Settings.soundEsp.showTeammates);

    ImGui::SeparatorText("Colors");
    UI::SettingColor("##footstep_color", Config::Settings.soundEsp.footstepColor);
    ImGui::SameLine();
    ImGui::Text("Footstep");

    UI::SettingColor("##jump_color", Config::Settings.soundEsp.jumpColor);
    ImGui::SameLine();
    ImGui::Text("Jump");

    UI::SettingColor("##land_color", Config::Settings.soundEsp.landColor);
    ImGui::SameLine();
    ImGui::Text("Land");

    ImGui::SeparatorText("Settings");
    ImGui::SliderFloat("Footstep Radius", &Config::Settings.soundEsp.footstepMaxRadius, 10.0f, 60.0f, "%.0f");
    ImGui::SliderFloat("Jump Radius", &Config::Settings.soundEsp.jumpMaxRadius, 15.0f, 80.0f, "%.0f");
    ImGui::SliderFloat("Land Radius", &Config::Settings.soundEsp.landMaxRadius, 20.0f, 100.0f, "%.0f");

    ImGui::SliderFloat("Expand Time", &Config::Settings.soundEsp.expandDuration, 0.1f, 2.0f, "%.1fs");
    ImGui::SliderFloat("Fade Time", &Config::Settings.soundEsp.fadeDuration, 0.3f, 3.0f, "%.1fs");

    ImGui::SliderFloat("Thickness", &Config::Settings.soundEsp.thickness, 1.0f, 5.0f, "%.1f");
    ImGui::SliderInt("Segments", &Config::Settings.soundEsp.segments, 16, 64, "%d");
}

} // namespace Features
