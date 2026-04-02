#include "tab_settings.h"
#include "menu.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "core/sdk/updater.h"
#include "features/feature_manager.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <future>

namespace Render {

static char s_configName[64] = "default";
static std::vector<std::string> s_configList;
static int s_configSelected = 0;
static bool s_configDirty = false;

static bool s_offsetUpdatePending = false;
static bool s_offsetUpdateSuccess = false;
static std::string s_offsetUpdateMessage = "";
static std::future<bool> s_offsetUpdateFuture;

void RenderTabSettings() {
    ImGui::Columns(2, "SetCols", false);

    if (UI::BeginCard("Configuration")) {
        const char *themes[] = {"Midnight", "Blood", "Cyber", "Lavender", "Gold", "Monochrome", "Toxic"};
        if (ImGui::Combo("Theme Preset", &Config::Settings.misc.menuTheme, themes, 7)) {
            Config::ConfigManager::ApplySettings();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Offset Update Section
        ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_Text], "Offsets Management");
        ImGui::Spacing();

        if (ImGui::Button("Update Offsets from GitHub", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
            s_offsetUpdatePending = true;
            s_offsetUpdateSuccess = false;
            s_offsetUpdateMessage = "Updating...";
            s_offsetUpdateFuture = SDK::Updater::ForceUpdateOffsets();
        }

        if (s_offsetUpdatePending && s_offsetUpdateFuture.valid() &&
            s_offsetUpdateFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            s_offsetUpdateSuccess = s_offsetUpdateFuture.get();
            s_offsetUpdateMessage = s_offsetUpdateSuccess ? "Offsets updated successfully!" : "Failed to update offsets.";
        }

        if (!s_offsetUpdateMessage.empty()) {
            ImGui::Spacing();
            ImVec4 msgColor = s_offsetUpdateSuccess ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
            ImGui::TextColored(msgColor, "%s", s_offsetUpdateMessage.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::InputText("Name", s_configName, sizeof(s_configName));
        if (ImGui::Button("Save Config", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            Config::ConfigManager::Save(s_configName);
            s_configList = Config::ConfigManager::ListConfigs();
        }

        ImGui::Spacing();
        if (ImGui::Button("Refresh List", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            s_configList = Config::ConfigManager::ListConfigs();

        ImGui::BeginChild("List", ImVec2(0, 120), true);
        for (int i = 0; i < (int)s_configList.size(); i++) {
            if (ImGui::Selectable(s_configList[i].c_str(), i == s_configSelected))
                s_configSelected = i;
        }
        ImGui::EndChild();

        if (!s_configList.empty()) {
            if (ImGui::Button("Load Selected", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                strncpy_s(s_configName, s_configList[s_configSelected].c_str(), sizeof(s_configName) - 1);
                Config::ConfigManager::Load(s_configList[s_configSelected]);
            }
        }
    }
    UI::EndCard();

    ImGui::NextColumn();

    if (UI::BeginCard("Performance & Debug")) {
        UI::SettingToggle("Enable VSync", &Config::Settings.performance.vsyncEnabled);
        ImGui::SliderInt("FPS Limit", &Config::Settings.performance.fpsLimit, 10, 500, "%d FPS");
        ImGui::SliderInt("UPS Limit (Max)", &Config::Settings.performance.upsLimit, 10, 500, "%d UPS");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (Config::Settings.performance.vsyncEnabled) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "VSync Active - FPS/UPS synced to display");
            ImGui::Spacing();
        }

        UI::SettingToggle("Enable Debug Overlay", &Config::Settings.debug.enabled);
        UI::SettingToggle("Developer Mode", &Config::Settings.debug.devMode);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.0f, 0.0f, 1));
        if (ImGui::Button("UNLOAD / EXIT", ImVec2(ImGui::GetContentRegionAvail().x, 35)))
            Menu::shouldClose = true;
        ImGui::PopStyleColor(3);
    }
    UI::EndCard();
    ImGui::Columns(1);
}

} // namespace Render