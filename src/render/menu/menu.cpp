#include "menu.h"
#include "../overlay/overlay.h"
#include "config/config_manager.h"
#include "features/aimbot/aimbot.h"
#include "features/aimbot/aimbot_config.h"
#include "features/bomb/bomb.h"
#include "features/debug_overlay/debug_overlay.h"
#include "features/esp/esp.h"
#include "features/feature_manager.h"
#include "features/misc/misc.h"
#include "features/misc/misc_config.h"
#include "features/triggerbot/triggerbot.h"
#include "features/triggerbot/triggerbot_config.h"
#include <imgui.h>
#include <string>
#include <vector>

// Extern global configs
namespace Features {
extern AimbotConfig aimbotConfig;
extern TriggerbotConfig triggerbotConfig;
extern MiscConfig miscConfig;
} // namespace Features

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
// keyTarget is updated in place.
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
    // Poll for any key press
    static const int candidateVKs[] = {
        0x01,       0x02,    0x04,      0x05,   0x06, VK_SHIFT,
        VK_CONTROL, VK_MENU, VK_INSERT, VK_END, 'Z',  'X',
        'C',        'V',     'F',       'G',    'H'};
    for (int v : candidateVKs) {
      if (GetAsyncKeyState(v) & 0x8000) {
        keyTarget = v;
        s_listening = nullptr;
        changed = true;
        break;
      }
    }
  } else {
    if (ImGui::Button(btnLabel, ImVec2(180, 0)))
      s_listening = &keyTarget;
  }
  return changed;
}

// ─── Config UI state ─────────────────────────────────────────────────────────
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

      // ────────── ESP ──────────────────────────────────────────────────────
      if (ImGui::BeginTabItem("ESP")) {
        ImGui::Text("Visual Settings");
        ImGui::Separator();

        bool en = Features::config.enabled;
        if (ImGui::Checkbox("Enable ESP", &en)) {
          for (auto &f : Features::FeatureManager::features)
            if (std::string(f->GetName()) == "ESP")
              f->SetEnabled(en);
          Features::config.enabled = en;
        }
        ImGui::Checkbox("Draw Boxes", &Features::config.showBox);
        ImGui::Checkbox("Draw Names", &Features::config.showName);
        ImGui::Checkbox("Draw Health", &Features::config.showHealth);
        ImGui::Checkbox("Draw Weapon", &Features::config.showWeapon);
        ImGui::Checkbox("Draw Distance", &Features::config.showDistance);
        ImGui::Separator();
        ImGui::Checkbox("Team ESP", &Features::config.showTeammates);
        ImGui::ColorEdit4("Enemy Color", Features::config.boxColor);
        if (Features::config.showTeammates)
          ImGui::ColorEdit4("Team Color", Features::config.teamColor);
        ImGui::Separator();
        ImGui::Checkbox("Show Bones", &Features::config.showBones);
        if (Features::config.showBones)
          ImGui::SliderFloat("Bone Distance",
                             &Features::config.skeletonMaxDistance, 5.0f, 60.0f,
                             "%.0f m");
        ImGui::ColorEdit4("Bone Color", Features::config.boneColor);
        ImGui::EndTabItem();
      }

      // ────────── AIMBOT ───────────────────────────────────────────────────
      if (ImGui::BeginTabItem("Aimbot")) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "Aimbot");
        ImGui::Separator();
        ImGui::Checkbox("Enable Aimbot", &Features::aimbotConfig.enabled);

        if (Features::aimbotConfig.enabled) {
          ImGui::Spacing();
          HotkeyPicker("Hold Key", Features::aimbotConfig.hotkey);
          if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Click to rebind. Supports Mouse4/Mouse5.");

          ImGui::Spacing();
          ImGui::SliderFloat("FOV", &Features::aimbotConfig.fov, 1.0f, 30.0f,
                             "%.1f deg");
          ImGui::SliderFloat("Smooth", &Features::aimbotConfig.smooth, 1.0f,
                             20.0f, "%.1f");
          ImGui::SliderFloat("Jitter", &Features::aimbotConfig.jitter, 0.0f,
                             0.15f, "%.3f");
          if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Adds random noise to avoid machine-perfect patterns");

          ImGui::Separator();
          const char *bones[] = {"Pelvis", "Chest", "Neck", "Head"};
          const int bVals[] = {0, 4, 5, 6};
          int sel = 3;
          for (int i = 0; i < 4; i++)
            if (bVals[i] == Features::aimbotConfig.targetBone) {
              sel = i;
              break;
            }
          if (ImGui::Combo("Target Bone", &sel, bones, 4))
            Features::aimbotConfig.targetBone = bVals[sel];

          ImGui::Checkbox("Team Check", &Features::aimbotConfig.teamCheck);
          ImGui::Checkbox("Only Scoped", &Features::aimbotConfig.onlyScoped);
        }
        ImGui::EndTabItem();
      }

      // ────────── TRIGGERBOT ───────────────────────────────────────────────
      if (ImGui::BeginTabItem("Triggerbot")) {
        ImGui::TextColored(ImVec4(1, 0.7f, 0.1f, 1), "Triggerbot");
        ImGui::Separator();
        ImGui::Checkbox("Enable Triggerbot",
                        &Features::triggerbotConfig.enabled);

        if (Features::triggerbotConfig.enabled) {
          ImGui::Spacing();
          HotkeyPicker("Hold Key", Features::triggerbotConfig.hotkey);

          ImGui::SliderInt("Min Delay (ms)",
                           &Features::triggerbotConfig.delayMin, 0, 150);
          ImGui::SliderInt("Max Delay (ms)",
                           &Features::triggerbotConfig.delayMax,
                           Features::triggerbotConfig.delayMin, 300);
          ImGui::Checkbox("Team Check", &Features::triggerbotConfig.teamCheck);

          ImGui::Spacing();
          ImGui::TextDisabled("State: IDLE->FOUND->WAIT->SHOOT->COOLDOWN");
        }
        ImGui::EndTabItem();
      }

      // ────────── MISC ─────────────────────────────────────────────────────
      if (ImGui::BeginTabItem("Misc")) {
        ImGui::Text("Miscellaneous");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 1, 0.6f, 1), "AWP Noscope Crosshair");
        ImGui::Checkbox("Enable", &Features::miscConfig.awpCrosshair);
        if (Features::miscConfig.awpCrosshair) {
          const char *styles[] = {"Dot", "Cross", "Circle", "All"};
          ImGui::Combo("Style", &Features::miscConfig.crosshairStyle, styles,
                       4);
          ImGui::SliderFloat("Size", &Features::miscConfig.crosshairSize, 2.0f,
                             25.0f, "%.0f px");
          ImGui::SliderFloat("Thickness",
                             &Features::miscConfig.crosshairThickness, 0.5f,
                             4.0f, "%.1f");
          ImGui::ColorEdit4("Color", Features::miscConfig.crosshairColor);
          ImGui::Checkbox("Center Gap", &Features::miscConfig.crosshairGap);
        }
        ImGui::EndTabItem();
      }

      // ────────── BOMB ─────────────────────────────────────────────────────
      if (ImGui::BeginTabItem("Bomb")) {
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Bomb Timer");
        ImGui::Separator();
        ImGui::Checkbox("Enable Bomb Timer", &Features::bombConfig.enabled);
        ImGui::TextDisabled("Timer: 40s (CS2 standard). Site auto-detected.");
        ImGui::EndTabItem();
      }

      // ────────── DEBUG ─────────────────────────────────────────────────────
      if (ImGui::BeginTabItem("Debug")) {
        ImGui::Text("Debug Overlay");
        ImGui::Separator();
        ImGui::Checkbox("Enable Debug Overlay", &Features::debugConfig.enabled);
        ImGui::Checkbox("Developer Mode", &Features::debugConfig.devMode);
        ImGui::EndTabItem();
      }

      // ────────── SETTINGS / CONFIG ────────────────────────────────────────
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
  if (!isOpen) {
    long ex = GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
    SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE,
                  ex | WS_EX_TRANSPARENT);
  }
}

} // namespace Render
