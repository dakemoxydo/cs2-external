#include "menu.h"
#include "../overlay/overlay.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include <imgui.h>
#include <string>
#include <vector>

// ─── Key picker ──────────────────────────────────────────────────────────────
// Returns button label given VK code
static const char *KeyLabel(int vk) {
  switch (vk) {
  case 0x01:
    return "LMB";
  case 0x02:
    return "RMB";
  case 0x04:
    return "MMB";
  case 0x05:
    return "Mouse4"; // XButton1
  case 0x06:
    return "Mouse5"; // XButton2
  case 0x10:
    return "Shift";
  case 0x11:
    return "Ctrl";
  case 0x12:
    return "Alt";
  case VK_INSERT:
    return "Insert";
  case VK_END:
    return "End";
  case 'Z':
    return "Z";
  case 'X':
    return "X";
  case 'C':
    return "C";
  default: {
    static char buf[8];
    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
  }
  }
}

// Draws a "pick key" inline button. Returns true when a new key was selected.
static bool HotkeyPicker(const char *label, int &keyTarget) {
  static int *s_listening = nullptr;
  bool changed = false;

  char btnLabel[64];
  snprintf(btnLabel, sizeof(btnLabel), "%s  [%s]", label, KeyLabel(keyTarget));

  bool listening = (s_listening == &keyTarget);

  if (listening) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.0f, 1.0f));
    if (ImGui::Button("... Press key ...", ImVec2(180, 0))) {
      s_listening = nullptr;
    }
    ImGui::PopStyleColor();

    // ESC cancels listening without changing the key
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
      s_listening = nullptr;
    } else {
      // Extended candidate list: mouse buttons, modifiers, F-keys, arrows,
      // digits, common letters
      static const int candidateVKs[] = {
          // Mouse
          0x01,
          0x02,
          0x04,
          0x05,
          0x06,
          // Modifiers
          VK_SHIFT,
          VK_CONTROL,
          VK_MENU,
          // Navigation
          VK_INSERT,
          VK_END,
          VK_HOME,
          VK_DELETE,
          VK_LEFT,
          VK_RIGHT,
          VK_UP,
          VK_DOWN,
          // F-keys
          VK_F1,
          VK_F2,
          VK_F3,
          VK_F4,
          VK_F5,
          VK_F6,
          VK_F7,
          VK_F8,
          VK_F9,
          VK_F10,
          VK_F11,
          VK_F12,
          // Digits
          '0',
          '1',
          '2',
          '3',
          '4',
          '5',
          '6',
          '7',
          '8',
          '9',
          // Common letters
          'Z',
          'X',
          'C',
          'V',
          'F',
          'G',
          'H',
          'R',
          'T',
          'B',
      };
      for (int v : candidateVKs) {
        if (GetAsyncKeyState(v) & 0x8000) {
          keyTarget = v;
          s_listening = nullptr;
          changed = true;
          break;
        }
      }
    }
  } else {
    if (ImGui::Button(btnLabel, ImVec2(180, 0)))
      s_listening = &keyTarget;
  }
  return changed;
}

// в”Ђв”Ђв”Ђ Config UI state
// в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
static char s_configName[64] = "default";
static std::vector<std::string> s_configList;
static int s_configSelected = 0;
static bool s_configDirty = false; // show * in title

namespace Render {
bool Menu::isOpen = false;
bool Menu::shouldClose = false;

void Menu::Render() {
  if (!isOpen)
    return;

  long ex = GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
  if (ex & WS_EX_TRANSPARENT)
    SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE,
                  ex & ~WS_EX_TRANSPARENT);

  ImGui::SetNextWindowSize(ImVec2(780, 560), ImGuiCond_FirstUseEver);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

  char title[64];
  snprintf(title, sizeof(title), "CS2 External v2.0%s###MainWindow",
           s_configDirty ? " *" : "");

  if (ImGui::Begin(title, &isOpen, ImGuiWindowFlags_NoCollapse)) {
    if (ImGui::BeginTabBar("Tabs")) {

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ ESP
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("ESP")) {

        // в”Ђв”Ђ Enable
        // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
        if (ImGui::Checkbox("Enable ESP", &Config::Settings.esp.enabled)) {
          Config::ConfigManager::ApplySettings();
        }
        ImGui::Checkbox("Team ESP", &Config::Settings.esp.showTeammates);

        ImGui::Spacing();

        // в”Ђв”Ђ Box
        // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.f), "  Box");
        ImGui::Separator();
        ImGui::Checkbox("Draw Box", &Config::Settings.esp.showBox);
        if (Config::Settings.esp.showBox) {
          const char *boxStyles[] = {"Rect", "Corners", "Filled"};
          int bsInt = static_cast<int>(Config::Settings.esp.boxStyle);
          if (ImGui::Combo("Box Style", &bsInt, boxStyles, 3))
            Config::Settings.esp.boxStyle =
                static_cast<Features::BoxStyle>(bsInt);
          if (Config::Settings.esp.boxStyle == Features::BoxStyle::Filled)
            ImGui::SliderFloat("Fill Alpha", &Config::Settings.esp.fillBoxAlpha,
                               0.02f, 0.5f, "%.2f");
        }

        ImGui::Spacing();

        ImGui::Spacing();

        // в”Ђв”Ђ Health Bar
        // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
        ImGui::TextColored(ImVec4(0.3f, 1.f, 0.4f, 1.f), "  Health");
        ImGui::Separator();
        ImGui::Checkbox("Draw Health Bar", &Config::Settings.esp.showHealth);
        if (Config::Settings.esp.showHealth) {
          const char *hpStyles[] = {"Side (Vertical)", "Bottom (Horizontal)"};
          int hsInt = static_cast<int>(Config::Settings.esp.healthBarStyle);
          if (ImGui::Combo("Bar Style", &hsInt, hpStyles, 2))
            Config::Settings.esp.healthBarStyle =
                static_cast<Features::HealthBarStyle>(hsInt);
          ImGui::Checkbox("Show HP Text", &Config::Settings.esp.showHealthText);
        }

        ImGui::Spacing();

        // в”Ђв”Ђ Labels
        // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
        ImGui::TextColored(ImVec4(1.f, 1.f, 0.5f, 1.f), "  Labels");
        ImGui::Separator();
        ImGui::Checkbox("Draw Name", &Config::Settings.esp.showName);
        ImGui::Checkbox("Draw Weapon", &Config::Settings.esp.showWeapon);
        ImGui::Checkbox("Draw Distance", &Config::Settings.esp.showDistance);

        ImGui::Spacing();

        // в”Ђв”Ђ Snap Lines
        // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.f, 1.f), "  Snap Lines");
        ImGui::Separator();
        ImGui::Checkbox("Show Snap Lines", &Config::Settings.esp.showSnapLines);

        ImGui::Spacing();

        // в”Ђв”Ђ Skeleton
        // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
        ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 0.6f), "  Skeleton");
        ImGui::Separator();
        ImGui::Checkbox("Show Skeleton", &Config::Settings.esp.showBones);
        if (Config::Settings.esp.showBones) {
          ImGui::Checkbox("Skeleton Outline",
                          &Config::Settings.esp.skeletonOutline);
          ImGui::SliderFloat("Max Distance",
                             &Config::Settings.esp.skeletonMaxDistance, 5.0f,
                             60.0f, "%.0f m");
        }

        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ AIMBOT
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Aimbot")) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "Aimbot");
        ImGui::Separator();
        if (ImGui::Checkbox("Enable Aimbot",
                            &Config::Settings.aimbot.enabled)) {
          Config::ConfigManager::ApplySettings();
        }

        if (Config::Settings.aimbot.enabled) {
          ImGui::Spacing();
          HotkeyPicker("Hold Key", Config::Settings.aimbot.hotkey);
          if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Click to rebind. Supports Mouse4/Mouse5.");

          ImGui::Spacing();
          ImGui::SliderFloat("FOV", &Config::Settings.aimbot.fov, 1.0f, 30.0f,
                             "%.1f deg");
          ImGui::SliderFloat("Smooth", &Config::Settings.aimbot.smooth, 1.0f,
                             20.0f, "%.1f");
          ImGui::SliderFloat("Sensitivity",
                             &Config::Settings.aimbot.sensitivity, 0.1f, 10.0f,
                             "%.1f");
          if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Match your CS2 in-game sensitivity setting");
          ImGui::SliderFloat("Jitter", &Config::Settings.aimbot.jitter, 0.0f,
                             0.15f, "%.3f");
          if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Adds random noise to avoid machine-perfect patterns");

          ImGui::Separator();
          const char *bones[] = {"Pelvis", "Chest", "Neck", "Head"};
          const int bVals[] = {0, 4, 5, 6};
          int sel = 3;
          for (int i = 0; i < 4; i++)
            if (bVals[i] == Config::Settings.aimbot.targetBone) {
              sel = i;
              break;
            }
          if (ImGui::Combo("Target Bone", &sel, bones, 4))
            Config::Settings.aimbot.targetBone = bVals[sel];

          ImGui::Checkbox("Target Lock", &Config::Settings.aimbot.targetLock);
          ImGui::Checkbox("Visible Only", &Config::Settings.aimbot.visibleOnly);
          ImGui::Checkbox("Team Check", &Config::Settings.aimbot.teamCheck);
          ImGui::Checkbox("Only Scoped", &Config::Settings.aimbot.onlyScoped);
          ImGui::Checkbox("Show FOV Circle", &Config::Settings.aimbot.showFov);
        }
        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ TRIGGERBOT
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Triggerbot")) {
        ImGui::TextColored(ImVec4(1, 0.7f, 0.1f, 1), "Triggerbot");
        ImGui::Separator();
        if (ImGui::Checkbox("Enable Triggerbot",
                            &Config::Settings.triggerbot.enabled)) {
          Config::ConfigManager::ApplySettings();
        }

        if (Config::Settings.triggerbot.enabled) {
          ImGui::Spacing();
          HotkeyPicker("Hold Key", Config::Settings.triggerbot.hotkey);

          ImGui::SliderInt("Min Delay (ms)",
                           &Config::Settings.triggerbot.delayMin, 0, 150);
          ImGui::SliderInt("Max Delay (ms)",
                           &Config::Settings.triggerbot.delayMax,
                           Config::Settings.triggerbot.delayMin, 300);
          ImGui::Checkbox("Team Check", &Config::Settings.triggerbot.teamCheck);

          ImGui::Spacing();
          ImGui::TextDisabled("State: IDLE->FOUND->WAIT->SHOOT->COOLDOWN");
        }
        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ RADAR
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Radar")) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.f), "Radar Overlay");
        ImGui::Separator();
        if (ImGui::Checkbox("Enable Radar", &Config::Settings.radar.enabled)) {
          Config::ConfigManager::ApplySettings();
        }

        if (Config::Settings.radar.enabled) {
          ImGui::Spacing();
          ImGui::Checkbox("Rotate Map", &Config::Settings.radar.rotate);
          ImGui::Checkbox("Show Teammates",
                          &Config::Settings.radar.showTeammates);
          ImGui::Checkbox("Visible Check",
                          &Config::Settings.radar.visibleCheck);

          ImGui::Spacing();
          const char *stretchModes[] = {"None", "16:9 to 4:3", "16:9 to 16:10",
                                        "16:9 to 5:4"};
          ImGui::Combo("Stretch Type", &Config::Settings.radar.stretchType,
                       stretchModes, 4);

          const char *maps[] = {"Custom", "Mirage (1.88)", "Dust2 (2.32)",
                                "Inferno (1.44)", "Nuke (1.99)"};
          ImGui::Combo("Map Scale", &Config::Settings.radar.mapIndex, maps, 5);

          if (Config::Settings.radar.mapIndex == 0) {
            ImGui::SliderFloat("Custom Calibration",
                               &Config::Settings.radar.mapCalibration, 0.5f,
                               5.0f, "%.3f");
          }

          ImGui::Spacing();
          ImGui::SliderFloat("Scale / Zoom", &Config::Settings.radar.zoom, 0.1f,
                             3.0f, "%.2f");
          ImGui::SliderFloat("Point Size", &Config::Settings.radar.pointSize,
                             2.0f, 8.0f, "%.1f px");
          ImGui::SliderFloat("Background Alpha",
                             &Config::Settings.radar.bgAlpha, 0.0f, 1.0f,
                             "%.2f");
        }
        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ MISC
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Misc")) {
        ImGui::Text("Miscellaneous");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 1, 0.6f, 1), "AWP Crosshair");
        if (ImGui::Checkbox("Enable", &Config::Settings.misc.awpCrosshair)) {
          Config::ConfigManager::ApplySettings();
        }
        if (Config::Settings.misc.awpCrosshair) {
          const char *styles[] = {"Dot", "Cross", "Circle", "All"};
          ImGui::Combo("Style", &Config::Settings.misc.crosshairStyle, styles,
                       4);
          ImGui::SliderFloat("Size", &Config::Settings.misc.crosshairSize, 2.0f,
                             25.0f, "%.0f px");
          ImGui::SliderFloat("Thickness",
                             &Config::Settings.misc.crosshairThickness, 0.5f,
                             4.0f, "%.1f");
          ImGui::Checkbox("Center Gap", &Config::Settings.misc.crosshairGap);
        }
        ImGui::EndTabItem();
      }

      // ─── PERFORMANCE
      // ───────────────────────────────────────────────────────────────────────
      if (ImGui::BeginTabItem("Performance")) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.9f, 1),
                           "Limits & Optimization");
        ImGui::Separator();

        ImGui::SliderInt("FPS Limit", &Config::Settings.performance.fpsLimit,
                         10, 500, "%d FPS");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip(
              "Limits the rendering loop framerate. Saves GPU/CPU.");

        ImGui::SliderInt("UPS Limit (Memory)",
                         &Config::Settings.performance.upsLimit, 10, 500,
                         "%d UPS");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip(
              "Limits memory read logic updates per second. Lowering this "
              "drastically increases FPS but reduces ESP smoothness.");

        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ BOMB
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Bomb")) {
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Bomb Timer");
        ImGui::Separator();
        if (ImGui::Checkbox("Enable Bomb Timer",
                            &Config::Settings.bomb.enabled)) {
          Config::ConfigManager::ApplySettings();
        }
        ImGui::TextDisabled("Timer: 40s (CS2 standard). Site auto-detected.");
        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ COLORS
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Colors")) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.8f, 1.f), "Menu & ESP Colors");
        ImGui::Separator();

        ImGuiColorEditFlags flags =
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip;

        if (ImGui::CollapsingHeader("ESP Visuals",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::ColorEdit4("Box Base", Config::Settings.esp.boxColor, flags);
          ImGui::ColorEdit4("Box Team", Config::Settings.esp.teamColor, flags);
          ImGui::ColorEdit4("Skeleton & Head", Config::Settings.esp.boneColor,
                            flags);
          ImGui::ColorEdit4("Skeleton Outline",
                            Config::Settings.esp.skeletonOutlineColor,
                            flags); // Added here
          ImGui::ColorEdit4("Snap Lines", Config::Settings.esp.snapLineColor,
                            flags);
        }

        if (ImGui::CollapsingHeader("ESP Text",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::ColorEdit4("Name Text", Config::Settings.esp.nameColor, flags);
          ImGui::ColorEdit4("Weapon Text", Config::Settings.esp.weaponColor,
                            flags);
          ImGui::ColorEdit4("Distance Text", Config::Settings.esp.distColor,
                            flags);
        }

        if (ImGui::CollapsingHeader("Radar", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::ColorEdit4("Radar Enemy (Base)",
                            Config::Settings.radar.enemyColor, flags);
          ImGui::ColorEdit4("Radar Visible Enemy",
                            Config::Settings.radar.visibleColor, flags);
          ImGui::ColorEdit4("Radar Hidden Enemy",
                            Config::Settings.radar.hiddenColor, flags);
          ImGui::ColorEdit4("Radar Team", Config::Settings.radar.teamColor,
                            flags);
        }

        if (ImGui::CollapsingHeader("Misc", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::ColorEdit4("Crosshair", Config::Settings.misc.crosshairColor,
                            flags);
        }

        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ DEBUG
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Debug")) {
        ImGui::Text("Debug Overlay");
        ImGui::Separator();
        if (ImGui::Checkbox("Enable Debug Overlay",
                            &Config::Settings.debug.enabled)) {
          Config::ConfigManager::ApplySettings();
        }
        ImGui::Checkbox("Developer Mode", &Config::Settings.debug.devMode);
        ImGui::EndTabItem();
      }

      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ SETTINGS / CONFIG
      // в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ
      if (ImGui::BeginTabItem("Settings")) {
        ImGui::Text("Config Manager");
        ImGui::Separator();
        ImGui::Spacing();

        // Config name input
        ImGui::InputText("Config Name", s_configName, sizeof(s_configName));
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
          Config::ConfigManager::Save(s_configName);
          s_configDirty = false;
          s_configList = Config::ConfigManager::ListConfigs();
        }

        // Config list
        if (ImGui::Button("Refresh List"))
          s_configList = Config::ConfigManager::ListConfigs();

        if (!s_configList.empty()) {
          ImGui::Spacing();
          ImGui::Text("Available Configs:");
          ImGui::BeginChild("ConfigList", ImVec2(0, 120), true);
          for (int i = 0; i < (int)s_configList.size(); i++) {
            bool sel = (i == s_configSelected);
            if (ImGui::Selectable(s_configList[i].c_str(), sel))
              s_configSelected = i;
          }
          ImGui::EndChild();

          if (!s_configList.empty()) {
            if (ImGui::Button("Load Selected")) {
              strncpy_s(s_configName, s_configList[s_configSelected].c_str(),
                        sizeof(s_configName) - 1);
              Config::ConfigManager::Load(s_configList[s_configSelected]);
              s_configDirty = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete Selected")) {
              // Simple: just rename to inform user (no fs::remove in release -
              // safe)
              ImGui::OpenPopup("ConfirmDelete");
            }
          }
        }

        if (!Config::ConfigManager::LastError.empty()) {
          ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Error: %s",
                             Config::ConfigManager::LastError.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Load defaults
        if (ImGui::Button("Reset to Defaults")) {
          Config::ConfigManager::LoadDefault();
        }

        // Initialize config list on first render
        if (s_configList.empty())
          s_configList = Config::ConfigManager::ListConfigs();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(1.0f, 0.2f, 0.2f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.6f, 0.0f, 0.0f, 1));
        if (ImGui::Button("UNLOAD CHEAT",
                          ImVec2(ImGui::GetContentRegionAvail().x, 40)))
          shouldClose = true;
        ImGui::PopStyleColor(3);
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
  ImGui::End();
  ImGui::PopStyleVar(2);
}

bool Menu::IsOpen() { return isOpen; }
bool Menu::ShouldClose() { return shouldClose; }
void Menu::Toggle() {
  isOpen = !isOpen;
  Config::Settings.menuIsOpen = isOpen;
  if (!isOpen) {
    long ex = GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
    SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE,
                  ex | WS_EX_TRANSPARENT);
  }
}

} // namespace Render
