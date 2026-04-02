#pragma once
#include "rcs.h"
#include "rcs_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void RCSSystem::RenderUI() {
    UI::SettingToggle("Enable RCS", &Config::Settings.rcs.enabled);
    if (Config::Settings.rcs.enabled) {
        ImGui::SliderFloat("Pitch Strength", &Config::Settings.rcs.pitchStrength, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Yaw Strength", &Config::Settings.rcs.yawStrength, 0.0f, 2.0f, "%.2f");
        ImGui::SliderInt("Start at Bullet", &Config::Settings.rcs.startBullet, 1, 10);
    }
}

} // namespace Features
