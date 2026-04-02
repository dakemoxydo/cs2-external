#pragma once
#include "bomb.h"
#include "bomb_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void Bomb::RenderUI() {
    UI::SettingToggle("Enable Bomb Timer", &Config::Settings.bomb.enabled);
}

} // namespace Features
