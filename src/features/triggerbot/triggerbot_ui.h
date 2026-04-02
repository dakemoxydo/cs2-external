#pragma once
#include "triggerbot.h"
#include "triggerbot_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void Triggerbot::RenderUI() {
    UI::SettingToggle("Enable Triggerbot", &Config::Settings.triggerbot.enabled);
    if (Config::Settings.triggerbot.enabled) {
        ImGui::Spacing();
        UI::SettingHotkey("Hold Key Trigger", Config::Settings.triggerbot.hotkey);
        ImGui::Spacing();
        ImGui::SliderInt("Min Delay (ms)", &Config::Settings.triggerbot.delayMin, 0, 150);
        ImGui::SliderInt("Max Delay (ms)", &Config::Settings.triggerbot.delayMax, Config::Settings.triggerbot.delayMin, 300);
        UI::SettingToggle("Team Check Trigger", &Config::Settings.triggerbot.teamCheck);
    }
}

} // namespace Features
