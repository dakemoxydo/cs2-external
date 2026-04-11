#include "tab_misc.h"
#include "ui_components.h"
#include "config/settings.h"
#include <algorithm>
#include <imgui.h>
#include <utility>

namespace Render {

namespace {

template <typename Fn>
void Commit(Fn &&fn) {
  Config::MutateSettingsVoid(std::forward<Fn>(fn));
}

} // namespace

void RenderTabMisc() {
  Config::GlobalSettings settings = Config::CopySettings();

  if (UI::BeginCard("Crosshair Utility")) {
    UI::SectionHeader("AWP Crosshair");

    bool awpCrosshair = settings.misc.awpCrosshair;
    if (UI::SettingToggle("Enable AWP Crosshair", &awpCrosshair)) {
      Commit([&](auto &state) { state.misc.awpCrosshair = awpCrosshair; });
      settings.misc.awpCrosshair = awpCrosshair;
    }

    if (settings.misc.awpCrosshair) {
      float crosshairColor[4] = {settings.misc.crosshairColor[0], settings.misc.crosshairColor[1],
                                 settings.misc.crosshairColor[2], settings.misc.crosshairColor[3]};
      if (UI::ColorRow("Crosshair Color", crosshairColor)) {
        Commit([&](auto &state) {
          std::copy(std::begin(crosshairColor), std::end(crosshairColor),
                    state.misc.crosshairColor);
        });
        std::copy(std::begin(crosshairColor), std::end(crosshairColor),
                  settings.misc.crosshairColor);
      }

      const char *styles[] = {"Dot", "Cross", "Circle", "All"};
      int crosshairStyle = settings.misc.crosshairStyle;
      if (ImGui::Combo("Style", &crosshairStyle, styles, 4)) {
        Commit([&](auto &state) { state.misc.crosshairStyle = crosshairStyle; });
        settings.misc.crosshairStyle = crosshairStyle;
      }

      float crosshairSize = settings.misc.crosshairSize;
      if (ImGui::SliderFloat("Size", &crosshairSize, 2.0f, 25.0f, "%.0f px")) {
        Commit([&](auto &state) { state.misc.crosshairSize = crosshairSize; });
        settings.misc.crosshairSize = crosshairSize;
      }

      float crosshairThickness = settings.misc.crosshairThickness;
      if (ImGui::SliderFloat("Thickness", &crosshairThickness, 0.5f, 4.0f, "%.1f")) {
        Commit([&](auto &state) { state.misc.crosshairThickness = crosshairThickness; });
        settings.misc.crosshairThickness = crosshairThickness;
      }

      bool crosshairGap = settings.misc.crosshairGap;
      if (UI::SettingToggle("Center Gap", &crosshairGap)) {
        Commit([&](auto &state) { state.misc.crosshairGap = crosshairGap; });
        settings.misc.crosshairGap = crosshairGap;
      }
    }
  }
  UI::EndCard();
}

} // namespace Render
