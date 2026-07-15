#include "tab_world.h"
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

void CopyColor(float (&dst)[4], const float (&src)[4]) {
  std::copy(std::begin(src), std::end(src), dst);
}

template <typename TargetColor>
void CopyColor(TargetColor &dst, const float (&src)[4]) {
  std::copy(std::begin(src), std::end(src), std::begin(dst));
}

void RenderRadarCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("RadarPanel")) {
    return;
  }

  UI::SectionHeader("Radar", "Map profile, visibility and local rotation.");

  bool radarEnabled = settings.radar.enabled;
  if (UI::SettingToggle("Enable Radar", &radarEnabled)) {
    Commit([&](auto &state) { state.radar.enabled = radarEnabled; });
    settings.radar.enabled = radarEnabled;
  }

  if (settings.radar.enabled) {
    bool radarRotate = settings.radar.rotate;
    if (UI::SettingToggle("Rotate Map", &radarRotate)) {
      Commit([&](auto &state) { state.radar.rotate = radarRotate; });
      settings.radar.rotate = radarRotate;
    }

    bool radarShowTeammates = settings.radar.showTeammates;
    if (UI::SettingToggle("Show Teammates", &radarShowTeammates)) {
      Commit([&](auto &state) {
        state.radar.showTeammates = radarShowTeammates;
      });
      settings.radar.showTeammates = radarShowTeammates;
    }

    bool radarVisibleCheck = settings.radar.visibleCheck;
    if (UI::SettingToggle("Visible Check", &radarVisibleCheck)) {
      Commit([&](auto &state) { state.radar.visibleCheck = radarVisibleCheck; });
      settings.radar.visibleCheck = radarVisibleCheck;
    }

    const char *maps[] = {"Custom", "Mirage", "Dust2", "Inferno", "Nuke"};
    int mapIndex = settings.radar.mapIndex;
    if (ImGui::Combo("Map Profile", &mapIndex, maps, 5)) {
      Commit([&](auto &state) { state.radar.mapIndex = mapIndex; });
      settings.radar.mapIndex = mapIndex;
    }

    float radarZoom = settings.radar.zoom;
    if (ImGui::SliderFloat("Zoom", &radarZoom, 0.10f, 3.0f, "%.2f")) {
      Commit([&](auto &state) { state.radar.zoom = radarZoom; });
      settings.radar.zoom = radarZoom;
    }

    float radarPointSize = settings.radar.pointSize;
    if (ImGui::SliderFloat("Point Size", &radarPointSize, 2.0f, 8.0f,
                           "%.1f")) {
      Commit([&](auto &state) { state.radar.pointSize = radarPointSize; });
      settings.radar.pointSize = radarPointSize;
    }

    float radarBgAlpha = settings.radar.bgAlpha;
    if (ImGui::SliderFloat("Background Alpha", &radarBgAlpha, 0.05f, 1.0f,
                           "%.2f")) {
      Commit([&](auto &state) { state.radar.bgAlpha = radarBgAlpha; });
      settings.radar.bgAlpha = radarBgAlpha;
    }
  }

  UI::EndCard();
}

void RenderRadarColorsCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("RadarColors")) {
    return;
  }

  UI::SectionHeader("Radar Colors", "Color channels for map dots and LOS state.");

  if (settings.radar.enabled) {
    float visibleColor[4];
    CopyColor(visibleColor, settings.radar.visibleColor);
    if (UI::ColorRow("Visible", visibleColor)) {
      Commit([&](auto &state) { CopyColor(state.radar.visibleColor, visibleColor); });
      CopyColor(settings.radar.visibleColor, visibleColor);
    }

    float hiddenColor[4];
    CopyColor(hiddenColor, settings.radar.hiddenColor);
    if (UI::ColorRow("Hidden", hiddenColor)) {
      Commit([&](auto &state) { CopyColor(state.radar.hiddenColor, hiddenColor); });
      CopyColor(settings.radar.hiddenColor, hiddenColor);
    }

    float enemyColor[4];
    CopyColor(enemyColor, settings.radar.enemyColor);
    if (UI::ColorRow("Enemy", enemyColor)) {
      Commit([&](auto &state) { CopyColor(state.radar.enemyColor, enemyColor); });
      CopyColor(settings.radar.enemyColor, enemyColor);
    }

    float teamColor[4];
    CopyColor(teamColor, settings.radar.teamColor);
    if (UI::ColorRow("Team", teamColor)) {
      Commit([&](auto &state) { CopyColor(state.radar.teamColor, teamColor); });
      CopyColor(settings.radar.teamColor, teamColor);
    }
  } else {
    UI::HelpText("Enable Radar to tune map point colors.");
  }

  UI::EndCard();
}

void RenderSoundEspCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("SoundEsp")) {
    return;
  }

  UI::SectionHeader("Sound ESP", "Footstep, jump and land propagation rings.");

  bool soundEspEnabled = settings.soundEsp.enabled;
  if (UI::SettingToggle("Enable Sound ESP", &soundEspEnabled)) {
    Commit([&](auto &state) { state.soundEsp.enabled = soundEspEnabled; });
    settings.soundEsp.enabled = soundEspEnabled;
  }

  if (settings.soundEsp.enabled) {
    bool soundEspShowTeammates = settings.soundEsp.showTeammates;
    if (UI::SettingToggle("Show Teammates", &soundEspShowTeammates)) {
      Commit([&](auto &state) {
        state.soundEsp.showTeammates = soundEspShowTeammates;
      });
      settings.soundEsp.showTeammates = soundEspShowTeammates;
    }

    float footstepColor[4];
    float jumpColor[4];
    float landColor[4];
    CopyColor(footstepColor, settings.soundEsp.footstepColor);
    CopyColor(jumpColor, settings.soundEsp.jumpColor);
    CopyColor(landColor, settings.soundEsp.landColor);

    if (UI::ColorRow("Footstep Color", footstepColor)) {
      Commit([&](auto &state) {
        CopyColor(state.soundEsp.footstepColor, footstepColor);
      });
      CopyColor(settings.soundEsp.footstepColor, footstepColor);
    }
    if (UI::ColorRow("Jump Color", jumpColor)) {
      Commit([&](auto &state) {
        CopyColor(state.soundEsp.jumpColor, jumpColor);
      });
      CopyColor(settings.soundEsp.jumpColor, jumpColor);
    }
    if (UI::ColorRow("Land Color", landColor)) {
      Commit([&](auto &state) {
        CopyColor(state.soundEsp.landColor, landColor);
      });
      CopyColor(settings.soundEsp.landColor, landColor);
    }
  }

  UI::EndCard();
}

void RenderAlertsCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("Alerts")) {
    return;
  }

  UI::SectionHeader("Alerts", "Effect radius and bomb helper controls.");

  if (settings.soundEsp.enabled) {
    float footstepRadius = settings.soundEsp.footstepMaxRadius;
    if (ImGui::SliderFloat("Footstep Radius", &footstepRadius, 10.0f, 60.0f,
                           "%.0f")) {
      Commit([&](auto &state) {
        state.soundEsp.footstepMaxRadius = footstepRadius;
      });
      settings.soundEsp.footstepMaxRadius = footstepRadius;
    }

    float jumpRadius = settings.soundEsp.jumpMaxRadius;
    if (ImGui::SliderFloat("Jump Radius", &jumpRadius, 15.0f, 80.0f, "%.0f")) {
      Commit([&](auto &state) { state.soundEsp.jumpMaxRadius = jumpRadius; });
      settings.soundEsp.jumpMaxRadius = jumpRadius;
    }

    float landRadius = settings.soundEsp.landMaxRadius;
    if (ImGui::SliderFloat("Land Radius", &landRadius, 20.0f, 100.0f, "%.0f")) {
      Commit([&](auto &state) { state.soundEsp.landMaxRadius = landRadius; });
      settings.soundEsp.landMaxRadius = landRadius;
    }

    float expandDuration = settings.soundEsp.expandDuration;
    if (ImGui::SliderFloat("Expand Time", &expandDuration, 0.1f, 2.0f,
                           "%.1f s")) {
      Commit([&](auto &state) {
        state.soundEsp.expandDuration = expandDuration;
      });
      settings.soundEsp.expandDuration = expandDuration;
    }

    float fadeDuration = settings.soundEsp.fadeDuration;
    if (ImGui::SliderFloat("Fade Time", &fadeDuration, 0.3f, 3.0f, "%.1f s")) {
      Commit([&](auto &state) { state.soundEsp.fadeDuration = fadeDuration; });
      settings.soundEsp.fadeDuration = fadeDuration;
    }

    float thickness = settings.soundEsp.thickness;
    if (ImGui::SliderFloat("Thickness", &thickness, 1.0f, 5.0f, "%.1f")) {
      Commit([&](auto &state) { state.soundEsp.thickness = thickness; });
      settings.soundEsp.thickness = thickness;
    }

    int segments = settings.soundEsp.segments;
    if (ImGui::SliderInt("Segments", &segments, 16, 64)) {
      Commit([&](auto &state) { state.soundEsp.segments = segments; });
      settings.soundEsp.segments = segments;
    }
  } else {
    UI::HelpText("Enable Sound ESP to tune radius and fade behavior.");
  }

  bool bombEnabled = settings.bomb.enabled;
  if (UI::SettingToggle("Bomb Timer", &bombEnabled)) {
    Commit([&](auto &state) { state.bomb.enabled = bombEnabled; });
    settings.bomb.enabled = bombEnabled;
  }

  UI::EndCard();
}

} // namespace

void RenderTabWorld(int subTab) {
  const float gap = 14.0f;
  const float totalWidth = ImGui::GetContentRegionAvail().x;
  const float columnWidth = (totalWidth - gap) * 0.5f;
  Config::GlobalSettings settings = Config::CopySettings();

  ImGui::BeginChild("WorldLeftColumn", ImVec2(columnWidth, 0.0f), false,
                    ImGuiWindowFlags_None);
  if (subTab == 0) {
    RenderRadarCard(settings);
  } else {
    RenderSoundEspCard(settings);
  }
  ImGui::EndChild();

  ImGui::SameLine(0.0f, gap);

  ImGui::BeginChild("WorldRightColumn", ImVec2(0.0f, 0.0f), false,
                    ImGuiWindowFlags_None);
  if (subTab == 0) {
    RenderRadarColorsCard(settings);
  } else {
    RenderAlertsCard(settings);
  }
  ImGui::EndChild();
}

} // namespace Render
