#include "menu.h"
#include "../overlay/overlay.h"
#include "features/bomb/bomb.h"
#include "features/debug_overlay/debug_overlay.h"
#include "features/esp/esp.h"
#include "features/feature_manager.h"
#include <imgui.h>
#include <string>

namespace Render {
bool Menu::isOpen = false;
bool Menu::shouldClose = false;

void Menu::Render() {
  if (!isOpen)
    return;

  // Switch window interactivity when menu is open so we can click
  long ex_style =
      GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
  if (ex_style & WS_EX_TRANSPARENT) {
    SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE,
                  ex_style & ~WS_EX_TRANSPARENT);
  }

  ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

  // Custom styling elements can go here
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

  if (ImGui::Begin("CS2 External Control Panel", &isOpen,
                   ImGuiWindowFlags_NoCollapse)) {

    if (ImGui::BeginTabBar("MenuTabs", ImGuiTabBarFlags_None)) {

      if (ImGui::BeginTabItem("ESP")) {
        ImGui::Text("Visual Settings");
        ImGui::Separator();

        bool isEnabled =
            Features::config
                .enabled; // Using bool wrapper to trigger SetEnabled
        if (ImGui::Checkbox("Enable ESP", &isEnabled)) {
          // Find ESP feature in manager to properly toggle via SetEnabled()
          for (auto &feature : Features::FeatureManager::features) {
            if (std::string(feature->GetName()) == "ESP") {
              feature->SetEnabled(isEnabled);
            }
          }
          Features::config.enabled = isEnabled; // Fallback sync
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
          ImGui::SliderFloat("Bone Distance (m)",
                             &Features::config.skeletonMaxDistance, 5.0f, 60.0f,
                             "%.0f m");
        ImGui::ColorEdit4("Bone Color", Features::config.boneColor);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Bomb")) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Bomb Timer");
        ImGui::Separator();
        ImGui::Checkbox("Enable Bomb Timer", &Features::bombConfig.enabled);
        ImGui::TextDisabled("Timer: 41s (fixed, CS2 standard)");
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Debug")) {
        ImGui::Text("Debug Overlay");
        ImGui::Separator();
        ImGui::Checkbox("Enable Debug Overlay", &Features::debugConfig.enabled);
        ImGui::Checkbox("Developer Mode", &Features::debugConfig.devMode);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Settings")) {
        ImGui::Text("Cheat Settings");
        ImGui::Separator();

        ImGui::Spacing();
        if (ImGui::Button("Save Config", ImVec2(120, 30))) {
          // TODO: Hook to ConfigManager
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Config", ImVec2(120, 30))) {
          // TODO: Hook to ConfigManager
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Panic button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.6f, 0.0f, 0.0f, 1.0f));

        if (ImGui::Button("UNLOAD CHEAT",
                          ImVec2(ImGui::GetContentRegionAvail().x, 40))) {
          shouldClose = true;
        }

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

void Menu::Toggle() {
  isOpen = !isOpen;

  // Remove interactivity if menu closed
  if (!isOpen) {
    long ex_style =
        GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
    SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE,
                  ex_style | WS_EX_TRANSPARENT);
  }
}

bool Menu::ShouldClose() { return shouldClose; }
} // namespace Render
