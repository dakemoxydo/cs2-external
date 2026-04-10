#include "tab_misc.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "features/feature_manager.h"
#include <imgui.h>

namespace Render {

void RenderTabMisc() {
    if (UI::BeginCard("Miscellaneous")) {
        UI::SettingToggle("AWP Crosshair", &Config::Settings.misc.awpCrosshair);
        UI::SettingColor("##awp_cross_color", Config::Settings.misc.crosshairColor);

        if (Config::Settings.misc.awpCrosshair) {
            const char *styles[] = {"Dot", "Cross", "Circle", "All"};
            ImGui::Combo("Style", &Config::Settings.misc.crosshairStyle, styles, 4);
            ImGui::SliderFloat("Size", &Config::Settings.misc.crosshairSize, 2.0f, 25.0f, "%.0f px");
            ImGui::SliderFloat("Thickness", &Config::Settings.misc.crosshairThickness, 0.5f, 4.0f, "%.1f");
            UI::SettingToggle("Center Gap", &Config::Settings.misc.crosshairGap);
        }
    }
    UI::EndCard();
}

} // namespace Render