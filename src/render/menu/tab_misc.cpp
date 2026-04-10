#include "tab_misc.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include <imgui.h>

namespace Render {

void RenderTabMisc() {
    if (UI::BeginCard("Crosshair Utility")) {
        UI::SectionHeader("AWP Crosshair", "Scoped weapon helper for no-scope reference and center alignment.");
        UI::SettingToggle("Enable AWP Crosshair", &Config::Settings.misc.awpCrosshair);

        if (Config::Settings.misc.awpCrosshair) {
            UI::SettingColor("##awp_cross_color", Config::Settings.misc.crosshairColor);
            const char *styles[] = {"Dot", "Cross", "Circle", "All"};
            if (ImGui::Combo("Style", &Config::Settings.misc.crosshairStyle, styles, 4)) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Size", &Config::Settings.misc.crosshairSize, 2.0f, 25.0f, "%.0f px")) {
                Config::ConfigManager::ApplySettings();
            }
            if (ImGui::SliderFloat("Thickness", &Config::Settings.misc.crosshairThickness, 0.5f, 4.0f, "%.1f")) {
                Config::ConfigManager::ApplySettings();
            }
            UI::SettingToggle("Center Gap", &Config::Settings.misc.crosshairGap);
        } else {
            UI::HelpText("Enable the helper if you want a static reference when using scoped weapons without the default crosshair.");
        }
    }
    UI::EndCard();
}

} // namespace Render
