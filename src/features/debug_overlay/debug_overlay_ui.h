#pragma once
#include "debug_overlay.h"
#include "debug_overlay_config.h"
#include "config/settings.h"
#include "render/menu/ui_components.h"
#include <imgui.h>

namespace Features {

inline void DebugOverlay::RenderUI() {
    UI::SettingToggle("Enable Debug Overlay", &Config::Settings.debug.enabled);
    UI::SettingToggle("Developer Mode", &Config::Settings.debug.devMode);
}

} // namespace Features
