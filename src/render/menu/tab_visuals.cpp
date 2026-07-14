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

void RenderPlayerEspCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("PlayerEsp")) {
    return;
  }

  UI::SectionHeader("Player ESP", "Core player overlays and box presentation.");

  bool espEnabled = settings.esp.enabled;
  if (UI::SettingToggle("Master Enable", &espEnabled)) {
    Commit([&](auto &state) { state.esp.enabled = espEnabled; });
    settings.esp.enabled = espEnabled;
  }

  if (settings.esp.enabled) {
    bool showTeammates = settings.esp.showTeammates;
    if (UI::SettingToggle("Team Check", &showTeammates)) {
      Commit([&](auto &state) { state.esp.showTeammates = showTeammates; });
      settings.esp.showTeammates = showTeammates;
    }

    bool showBox = settings.esp.showBox;
    if (UI::SettingToggle("Boxes", &showBox)) {
      Commit([&](auto &state) { state.esp.showBox = showBox; });
      settings.esp.showBox = showBox;
    }

    if (settings.esp.showBox) {
      const char *boxStyles[] = {"Rect", "Corners", "Filled"};
      int boxStyle = static_cast<int>(settings.esp.boxStyle);
      if (ImGui::Combo("Box Style", &boxStyle, boxStyles, 3)) {
        auto finalStyle = static_cast<Features::BoxStyle>(boxStyle);
        Commit([&](auto &state) { state.esp.boxStyle = finalStyle; });
        settings.esp.boxStyle = finalStyle;
      }

      if (settings.esp.boxStyle == Features::BoxStyle::Filled) {
        float fillBoxAlpha = settings.esp.fillBoxAlpha;
        if (ImGui::SliderFloat("Box Fill", &fillBoxAlpha, 0.02f, 0.50f,
                               "%.2f")) {
          Commit([&](auto &state) { state.esp.fillBoxAlpha = fillBoxAlpha; });
          settings.esp.fillBoxAlpha = fillBoxAlpha;
        }
      }
    }

    bool showHealth = settings.esp.showHealth;
    if (UI::SettingToggle("Health Bar", &showHealth)) {
      Commit([&](auto &state) { state.esp.showHealth = showHealth; });
      settings.esp.showHealth = showHealth;
    }

    bool showBones = settings.esp.showBones;
    if (UI::SettingToggle("Skeleton", &showBones)) {
      Commit([&](auto &state) { state.esp.showBones = showBones; });
      settings.esp.showBones = showBones;
    }

    bool showName = settings.esp.showName;
    if (UI::SettingToggle("Names", &showName)) {
      Commit([&](auto &state) { state.esp.showName = showName; });
      settings.esp.showName = showName;
    }

    bool showWeapon = settings.esp.showWeapon;
    if (UI::SettingToggle("Weapon", &showWeapon)) {
      Commit([&](auto &state) { state.esp.showWeapon = showWeapon; });
      settings.esp.showWeapon = showWeapon;
    }

    bool showDistance = settings.esp.showDistance;
    if (UI::SettingToggle("Distance", &showDistance)) {
      Commit([&](auto &state) { state.esp.showDistance = showDistance; });
      settings.esp.showDistance = showDistance;
    }
  }

  UI::EndCard();
}

void RenderVisibilityCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("VisibilityStack")) {
    return;
  }

  UI::SectionHeader("Visibility", "Color rules and overlay visibility cues.");

  if (settings.esp.enabled) {
    float boxColor[4];
    CopyColor(boxColor, settings.esp.boxColor);
    if (UI::ColorRow("Visible Color", boxColor)) {
      Commit([&](auto &state) { CopyColor(state.esp.boxColor, boxColor); });
      CopyColor(settings.esp.boxColor, boxColor);
    }

    float teamColor[4];
    CopyColor(teamColor, settings.esp.teamColor);
    if (UI::ColorRow("Team Color", teamColor)) {
      Commit([&](auto &state) { CopyColor(state.esp.teamColor, teamColor); });
      CopyColor(settings.esp.teamColor, teamColor);
    }

    float snapLineColor[4];
    CopyColor(snapLineColor, settings.esp.snapLineColor);
    if (UI::ColorRow("Snap Line", snapLineColor)) {
      Commit([&](auto &state) {
        CopyColor(state.esp.snapLineColor, snapLineColor);
      });
      CopyColor(settings.esp.snapLineColor, snapLineColor);
    }

    float nameColor[4];
    CopyColor(nameColor, settings.esp.nameColor);
    if (UI::ColorRow("Name Color", nameColor)) {
      Commit([&](auto &state) { CopyColor(state.esp.nameColor, nameColor); });
      CopyColor(settings.esp.nameColor, nameColor);
    }

    float weaponColor[4];
    CopyColor(weaponColor, settings.esp.weaponColor);
    if (UI::ColorRow("Weapon Color", weaponColor)) {
      Commit([&](auto &state) { CopyColor(state.esp.weaponColor, weaponColor); });
      CopyColor(settings.esp.weaponColor, weaponColor);
    }

    bool showSnapLines = settings.esp.showSnapLines;
    if (UI::SettingToggle("Snap Lines", &showSnapLines)) {
      Commit([&](auto &state) { state.esp.showSnapLines = showSnapLines; });
      settings.esp.showSnapLines = showSnapLines;
    }

    if (settings.esp.showHealth) {
      const char *hpStyles[] = {"Side", "Bottom"};
      int healthStyle = static_cast<int>(settings.esp.healthBarStyle);
      if (ImGui::Combo("Health Layout", &healthStyle, hpStyles, 2)) {
        auto finalStyle = static_cast<Features::HealthBarStyle>(healthStyle);
        Commit([&](auto &state) { state.esp.healthBarStyle = finalStyle; });
        settings.esp.healthBarStyle = finalStyle;
      }
    }
  } else {
    UI::HelpText("Enable Player ESP to tune visibility colors.");
  }

  UI::EndCard();
}

void RenderTraceCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("TraceFx")) {
    return;
  }

  UI::SectionHeader("Preview Helpers",
                    "Screen-space feedback for shots and hit registration.");

  bool showBulletTracers = settings.esp.showBulletTracers;
  if (UI::SettingToggle("Bullet Tracers", &showBulletTracers)) {
    Commit([&](auto &state) { state.esp.showBulletTracers = showBulletTracers; });
    settings.esp.showBulletTracers = showBulletTracers;
  }

  if (settings.esp.showBulletTracers) {
    float tracerColor[4];
    CopyColor(tracerColor, settings.esp.bulletTracerColor);
    if (UI::ColorRow("Tracer Color", tracerColor)) {
      Commit([&](auto &state) {
        CopyColor(state.esp.bulletTracerColor, tracerColor);
      });
      CopyColor(settings.esp.bulletTracerColor, tracerColor);
    }

    float tracerThickness = settings.esp.bulletTracerThickness;
    if (ImGui::SliderFloat("Tracer Thickness", &tracerThickness, 1.0f, 5.0f,
                           "%.1f")) {
      Commit([&](auto &state) {
        state.esp.bulletTracerThickness = tracerThickness;
      });
      settings.esp.bulletTracerThickness = tracerThickness;
    }

    float tracerLife = settings.esp.bulletTracerLife;
    if (ImGui::SliderFloat("Tracer Life", &tracerLife, 0.15f, 2.0f,
                           "%.2f s")) {
      Commit([&](auto &state) { state.esp.bulletTracerLife = tracerLife; });
      settings.esp.bulletTracerLife = tracerLife;
    }
  }

  bool showHitmarker = settings.esp.showHitmarker;
  if (UI::SettingToggle("Hitmarker", &showHitmarker)) {
    Commit([&](auto &state) { state.esp.showHitmarker = showHitmarker; });
    settings.esp.showHitmarker = showHitmarker;
  }

  if (settings.esp.showHitmarker) {
    float hitmarkerColor[4];
    CopyColor(hitmarkerColor, settings.esp.hitmarkerColor);
    if (UI::ColorRow("Hitmarker Color", hitmarkerColor)) {
      Commit([&](auto &state) {
        CopyColor(state.esp.hitmarkerColor, hitmarkerColor);
      });
      CopyColor(settings.esp.hitmarkerColor, hitmarkerColor);
    }

    float hitmarkerLife = settings.esp.hitmarkerLife;
    if (ImGui::SliderFloat("Hitmarker Life", &hitmarkerLife, 0.08f, 0.60f,
                           "%.2f s")) {
      Commit([&](auto &state) { state.esp.hitmarkerLife = hitmarkerLife; });
      settings.esp.hitmarkerLife = hitmarkerLife;
    }
  }

  UI::EndCard();
}

void RenderEffectsCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("ChamsFx")) {
    return;
  }

  UI::SectionHeader("Effects", "Model materials, skeleton overlays and chams.");

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
    if (UI::SettingToggle("Outline", &skeletonOutline)) {
      Commit([&](auto &state) {
        state.esp.skeletonOutline = skeletonOutline;
      });
      settings.esp.skeletonOutline = skeletonOutline;
    }
  }

  bool chamsEnabled = settings.chams.enabled;
  if (UI::SettingToggle("Chams", &chamsEnabled)) {
    Commit([&](auto &state) { state.chams.enabled = chamsEnabled; });
    settings.chams.enabled = chamsEnabled;
  }

  if (settings.chams.enabled) {
    bool chamsShowTeammates = settings.chams.showTeammates;
    if (UI::SettingToggle("Team Chams", &chamsShowTeammates)) {
      Commit([&](auto &state) {
        state.chams.showTeammates = chamsShowTeammates;
      });
      settings.chams.showTeammates = chamsShowTeammates;
    }

    bool chamsVisibleCheck = settings.chams.visibleCheck;
    if (UI::SettingToggle("Visible Check", &chamsVisibleCheck)) {
      Commit([&](auto &state) { state.chams.visibleCheck = chamsVisibleCheck; });
      settings.chams.visibleCheck = chamsVisibleCheck;
    }

    const char *materialItems[] = {"Flat", "Shaded", "Rim"};
    int materialType = settings.chams.materialType;
    if (ImGui::Combo("Material", &materialType, materialItems, 3)) {
      Commit([&](auto &state) { state.chams.materialType = materialType; });
      settings.chams.materialType = materialType;
    }

    float chamsAlpha = settings.chams.alpha;
    if (ImGui::SliderFloat("Alpha", &chamsAlpha, 0.10f, 1.0f, "%.2f")) {
      Commit([&](auto &state) { state.chams.alpha = chamsAlpha; });
      settings.chams.alpha = chamsAlpha;
    }

    bool chamsWireframe = settings.chams.wireframe;
    if (UI::SettingToggle("Wireframe", &chamsWireframe)) {
      Commit([&](auto &state) { state.chams.wireframe = chamsWireframe; });
      settings.chams.wireframe = chamsWireframe;
    }
  }

  UI::EndCard();
}

void RenderEffectColorsCard(Config::GlobalSettings &settings) {
  if (!UI::BeginCard("EffectColors")) {
    return;
  }

  UI::SectionHeader("Material Colors",
                    "Per-team material colors and off-screen cues.");

  if (settings.chams.enabled) {
    float enemyChamsColor[4];
    CopyColor(enemyChamsColor, settings.chams.fillColor);
    if (UI::ColorRow(settings.chams.visibleCheck ? "Enemy Visible"
                                                 : "Enemy Fill",
                     enemyChamsColor)) {
      Commit([&](auto &state) {
        CopyColor(state.chams.fillColor, enemyChamsColor);
      });
      CopyColor(settings.chams.fillColor, enemyChamsColor);
    }

    float teamChamsColor[4];
    CopyColor(teamChamsColor, settings.chams.fillColorTeam);
    if (UI::ColorRow(settings.chams.visibleCheck ? "Team Visible" : "Team Fill",
                     teamChamsColor)) {
      Commit([&](auto &state) {
        CopyColor(state.chams.fillColorTeam, teamChamsColor);
      });
      CopyColor(settings.chams.fillColorTeam, teamChamsColor);
    }

    if (settings.chams.visibleCheck) {
      float enemyHiddenColor[4];
      CopyColor(enemyHiddenColor, settings.chams.hiddenColor);
      if (UI::ColorRow("Enemy Hidden", enemyHiddenColor)) {
        Commit([&](auto &state) {
          CopyColor(state.chams.hiddenColor, enemyHiddenColor);
        });
        CopyColor(settings.chams.hiddenColor, enemyHiddenColor);
      }

      float teamHiddenColor[4];
      CopyColor(teamHiddenColor, settings.chams.hiddenColorTeam);
      if (UI::ColorRow("Team Hidden", teamHiddenColor)) {
        Commit([&](auto &state) {
          CopyColor(state.chams.hiddenColorTeam, teamHiddenColor);
        });
        CopyColor(settings.chams.hiddenColorTeam, teamHiddenColor);
      }
    }

    float chamsWireColor[4];
    CopyColor(chamsWireColor, settings.chams.wireColor);
    if (UI::ColorRow("Wire Color", chamsWireColor)) {
      Commit([&](auto &state) {
        CopyColor(state.chams.wireColor, chamsWireColor);
      });
      CopyColor(settings.chams.wireColor, chamsWireColor);
    }
  } else {
    UI::HelpText("Enable Chams to tune material colors.");
  }

  bool frustumCullingEnabled = settings.esp.frustumCullingEnabled;
  if (UI::SettingToggle("Frustum Culling", &frustumCullingEnabled)) {
    Commit([&](auto &state) {
      state.esp.frustumCullingEnabled = frustumCullingEnabled;
    });
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
    if (UI::ColorRow("Arrow Color", offscreenColor)) {
      Commit([&](auto &state) {
        CopyColor(state.esp.offscreenColor, offscreenColor);
      });
      CopyColor(settings.esp.offscreenColor, offscreenColor);
    }
  }

  UI::EndCard();
}

} // namespace

void RenderTabVisuals(int subTab) {
  Config::GlobalSettings settings = Config::CopySettings();
  const float gap = 14.0f;
  const float totalWidth = ImGui::GetContentRegionAvail().x;
  const float columnWidth = (totalWidth - gap) * 0.5f;

  ImGui::BeginChild("VisualsLeftColumn", ImVec2(columnWidth, 0.0f), false,
                    ImGuiWindowFlags_NoScrollbar);
  if (subTab == 0) {
    RenderPlayerEspCard(settings);
  } else {
    RenderEffectsCard(settings);
  }
  ImGui::EndChild();

  ImGui::SameLine(0.0f, gap);

  ImGui::BeginChild("VisualsRightColumn", ImVec2(0.0f, 0.0f), false,
                    ImGuiWindowFlags_NoScrollbar);
  if (subTab == 0) {
    RenderVisibilityCard(settings);
    RenderTraceCard(settings);
  } else {
    RenderEffectColorsCard(settings);
  }
  ImGui::EndChild();
}

} // namespace Render
