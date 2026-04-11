#include "tab_visuals.h"
#include "ui_components.h"
#include "config/settings.h"
#include "features/esp/esp_config.h"
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

} // namespace

void RenderTabVisuals() {
  Config::GlobalSettings settings = Config::CopySettings();

  if (UI::BeginCard("Player ESP")) {
    UI::SectionHeader("Visibility");

    bool espEnabled = settings.esp.enabled;
    if (UI::SettingToggle("Enable ESP", &espEnabled)) {
      Commit([&](auto &state) { state.esp.enabled = espEnabled; });
      settings.esp.enabled = espEnabled;
    }

    if (settings.esp.enabled) {
      bool showTeammates = settings.esp.showTeammates;
      if (UI::SettingToggle("Show Teammates", &showTeammates)) {
        Commit([&](auto &state) { state.esp.showTeammates = showTeammates; });
        settings.esp.showTeammates = showTeammates;
      }

      bool showBox = settings.esp.showBox;
      if (UI::SettingToggle("Draw Box", &showBox)) {
        Commit([&](auto &state) { state.esp.showBox = showBox; });
        settings.esp.showBox = showBox;
      }

      if (settings.esp.showBox) {
        float boxColor[4];
        CopyColor(boxColor, settings.esp.boxColor);
        if (UI::ColorRow("Box Color", boxColor)) {
          Commit([&](auto &state) { CopyColor(state.esp.boxColor, boxColor); });
          CopyColor(settings.esp.boxColor, boxColor);
        }

        const char *boxStyles[] = {"Rect", "Corners", "Filled"};
        int boxStyle = static_cast<int>(settings.esp.boxStyle);
        if (ImGui::Combo("Box Style", &boxStyle, boxStyles, 3)) {
          auto finalStyle = static_cast<Features::BoxStyle>(boxStyle);
          Commit([&](auto &state) { state.esp.boxStyle = finalStyle; });
          settings.esp.boxStyle = finalStyle;
        }

        if (settings.esp.boxStyle == Features::BoxStyle::Filled) {
          float fillBoxAlpha = settings.esp.fillBoxAlpha;
          if (ImGui::SliderFloat("Fill Alpha", &fillBoxAlpha, 0.02f, 0.5f, "%.2f")) {
            Commit([&](auto &state) { state.esp.fillBoxAlpha = fillBoxAlpha; });
            settings.esp.fillBoxAlpha = fillBoxAlpha;
          }
        }
      }

      UI::SectionHeader("Labels");

      bool showHealth = settings.esp.showHealth;
      if (UI::SettingToggle("Health Bar", &showHealth)) {
        Commit([&](auto &state) { state.esp.showHealth = showHealth; });
        settings.esp.showHealth = showHealth;
      }

      if (settings.esp.showHealth) {
        const char *hpStyles[] = {"Side", "Bottom"};
        int healthStyle = static_cast<int>(settings.esp.healthBarStyle);
        if (ImGui::Combo("Health Layout", &healthStyle, hpStyles, 2)) {
          auto finalStyle = static_cast<Features::HealthBarStyle>(healthStyle);
          Commit([&](auto &state) { state.esp.healthBarStyle = finalStyle; });
          settings.esp.healthBarStyle = finalStyle;
        }

        bool showHealthText = settings.esp.showHealthText;
        if (UI::SettingToggle("Show Health Text", &showHealthText)) {
          Commit([&](auto &state) { state.esp.showHealthText = showHealthText; });
          settings.esp.showHealthText = showHealthText;
        }
      }

      bool showName = settings.esp.showName;
      if (UI::SettingToggle("Show Name", &showName)) {
        Commit([&](auto &state) { state.esp.showName = showName; });
        settings.esp.showName = showName;
      }

      if (settings.esp.showName) {
        float nameColor[4];
        CopyColor(nameColor, settings.esp.nameColor);
        if (UI::ColorRow("Name Color", nameColor)) {
          Commit([&](auto &state) { CopyColor(state.esp.nameColor, nameColor); });
          CopyColor(settings.esp.nameColor, nameColor);
        }
      }

      bool showWeapon = settings.esp.showWeapon;
      if (UI::SettingToggle("Show Weapon", &showWeapon)) {
        Commit([&](auto &state) { state.esp.showWeapon = showWeapon; });
        settings.esp.showWeapon = showWeapon;
      }

      if (settings.esp.showWeapon) {
        float weaponColor[4];
        CopyColor(weaponColor, settings.esp.weaponColor);
        if (UI::ColorRow("Weapon Color", weaponColor)) {
          Commit([&](auto &state) { CopyColor(state.esp.weaponColor, weaponColor); });
          CopyColor(settings.esp.weaponColor, weaponColor);
        }
      }

      bool showDistance = settings.esp.showDistance;
      if (UI::SettingToggle("Show Distance", &showDistance)) {
        Commit([&](auto &state) { state.esp.showDistance = showDistance; });
        settings.esp.showDistance = showDistance;
      }

      bool showSnapLines = settings.esp.showSnapLines;
      if (UI::SettingToggle("Snap Lines", &showSnapLines)) {
        Commit([&](auto &state) { state.esp.showSnapLines = showSnapLines; });
        settings.esp.showSnapLines = showSnapLines;
      }

      if (settings.esp.showSnapLines) {
        float snapLineColor[4];
        CopyColor(snapLineColor, settings.esp.snapLineColor);
        if (UI::ColorRow("Snap Color", snapLineColor)) {
          Commit([&](auto &state) { CopyColor(state.esp.snapLineColor, snapLineColor); });
          CopyColor(settings.esp.snapLineColor, snapLineColor);
        }
      }

      UI::SectionHeader("Skeleton");

      bool showBones = settings.esp.showBones;
      if (UI::SettingToggle("Draw Skeleton", &showBones)) {
        Commit([&](auto &state) { state.esp.showBones = showBones; });
        settings.esp.showBones = showBones;
      }

      if (settings.esp.showBones) {
        float boneColor[4];
        CopyColor(boneColor, settings.esp.boneColor);
        if (UI::ColorRow("Bone Color", boneColor)) {
          Commit([&](auto &state) { CopyColor(state.esp.boneColor, boneColor); });
          CopyColor(settings.esp.boneColor, boneColor);
        }

        bool skeletonOutline = settings.esp.skeletonOutline;
        if (UI::SettingToggle("Skeleton Outline", &skeletonOutline)) {
          Commit([&](auto &state) { state.esp.skeletonOutline = skeletonOutline; });
          settings.esp.skeletonOutline = skeletonOutline;
        }

        if (settings.esp.skeletonOutline) {
          float outlineColor[4];
          CopyColor(outlineColor, settings.esp.skeletonOutlineColor);
          if (UI::ColorRow("Outline Color", outlineColor)) {
            Commit([&](auto &state) {
              CopyColor(state.esp.skeletonOutlineColor, outlineColor);
            });
            CopyColor(settings.esp.skeletonOutlineColor, outlineColor);
          }
        }

        float skeletonMaxDistance = settings.esp.skeletonMaxDistance;
        if (ImGui::SliderFloat("Skeleton Max Distance", &skeletonMaxDistance, 0.0f,
                               100.0f, "%.0f m")) {
          Commit([&](auto &state) { state.esp.skeletonMaxDistance = skeletonMaxDistance; });
          settings.esp.skeletonMaxDistance = skeletonMaxDistance;
        }
      }
    }
  }
  UI::EndCard();

  if (UI::BeginCard("World And Motion")) {
    UI::SectionHeader("Frustum / Off-Screen");

    bool frustumCullingEnabled = settings.esp.frustumCullingEnabled;
    if (UI::SettingToggle("Frustum Culling", &frustumCullingEnabled)) {
      Commit([&](auto &state) { state.esp.frustumCullingEnabled = frustumCullingEnabled; });
      settings.esp.frustumCullingEnabled = frustumCullingEnabled;
    }

    bool showOffscreen = settings.esp.showOffscreen;
    if (UI::SettingToggle("Off-Screen Arrows", &showOffscreen)) {
      Commit([&](auto &state) { state.esp.showOffscreen = showOffscreen; });
      settings.esp.showOffscreen = showOffscreen;
    }

    if (settings.esp.showOffscreen) {
      float offscreenColor[4];
      CopyColor(offscreenColor, settings.esp.offscreenColor);
      if (UI::ColorRow("Off-Screen Color", offscreenColor)) {
        Commit([&](auto &state) { CopyColor(state.esp.offscreenColor, offscreenColor); });
        CopyColor(settings.esp.offscreenColor, offscreenColor);
      }
    }

    UI::SectionHeader("Radar");

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
        Commit([&](auto &state) { state.radar.showTeammates = radarShowTeammates; });
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
      if (ImGui::SliderFloat("Zoom", &radarZoom, 0.1f, 3.0f, "%.2f")) {
        Commit([&](auto &state) { state.radar.zoom = radarZoom; });
        settings.radar.zoom = radarZoom;
      }

      float radarPointSize = settings.radar.pointSize;
      if (ImGui::SliderFloat("Point Size", &radarPointSize, 2.0f, 8.0f, "%.1f")) {
        Commit([&](auto &state) { state.radar.pointSize = radarPointSize; });
        settings.radar.pointSize = radarPointSize;
      }

      float radarBgAlpha = settings.radar.bgAlpha;
      if (ImGui::SliderFloat("Background Alpha", &radarBgAlpha, 0.05f, 1.0f, "%.2f")) {
        Commit([&](auto &state) { state.radar.bgAlpha = radarBgAlpha; });
        settings.radar.bgAlpha = radarBgAlpha;
      }
    }

    UI::SectionHeader("Sound ESP");

    bool soundEspEnabled = settings.soundEsp.enabled;
    if (UI::SettingToggle("Enable Sound ESP", &soundEspEnabled)) {
      Commit([&](auto &state) { state.soundEsp.enabled = soundEspEnabled; });
      settings.soundEsp.enabled = soundEspEnabled;
    }

    if (settings.soundEsp.enabled) {
      bool soundEspShowTeammates = settings.soundEsp.showTeammates;
      if (UI::SettingToggle("Show Teammates", &soundEspShowTeammates)) {
        Commit([&](auto &state) { state.soundEsp.showTeammates = soundEspShowTeammates; });
        settings.soundEsp.showTeammates = soundEspShowTeammates;
      }

      float footstepColor[4];
      float jumpColor[4];
      float landColor[4];
      CopyColor(footstepColor, settings.soundEsp.footstepColor);
      CopyColor(jumpColor, settings.soundEsp.jumpColor);
      CopyColor(landColor, settings.soundEsp.landColor);

      if (UI::ColorRow("Footstep Color", footstepColor)) {
        Commit([&](auto &state) { CopyColor(state.soundEsp.footstepColor, footstepColor); });
        CopyColor(settings.soundEsp.footstepColor, footstepColor);
      }
      if (UI::ColorRow("Jump Color", jumpColor)) {
        Commit([&](auto &state) { CopyColor(state.soundEsp.jumpColor, jumpColor); });
        CopyColor(settings.soundEsp.jumpColor, jumpColor);
      }
      if (UI::ColorRow("Land Color", landColor)) {
        Commit([&](auto &state) { CopyColor(state.soundEsp.landColor, landColor); });
        CopyColor(settings.soundEsp.landColor, landColor);
      }

      float footstepRadius = settings.soundEsp.footstepMaxRadius;
      if (ImGui::SliderFloat("Footstep Radius", &footstepRadius, 10.0f, 60.0f, "%.0f")) {
        Commit([&](auto &state) { state.soundEsp.footstepMaxRadius = footstepRadius; });
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
      if (ImGui::SliderFloat("Expand Time", &expandDuration, 0.1f, 2.0f, "%.1f s")) {
        Commit([&](auto &state) { state.soundEsp.expandDuration = expandDuration; });
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
    }

    UI::SectionHeader("Alerts");
    bool bombEnabled = settings.bomb.enabled;
    if (UI::SettingToggle("Bomb Timer", &bombEnabled)) {
      Commit([&](auto &state) { state.bomb.enabled = bombEnabled; });
      settings.bomb.enabled = bombEnabled;
    }
  }
  UI::EndCard();
}

} // namespace Render
