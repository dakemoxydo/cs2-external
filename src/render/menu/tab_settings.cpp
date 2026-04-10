#include "tab_settings.h"
#include "menu.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "core/sdk/updater.h"
#include <chrono>
#include <cctype>
#include <cstring>
#include <future>
#include <imgui.h>
#include <string>
#include <vector>

namespace Render {

static char s_configName[64] = "default";
static std::vector<std::string> s_configList;
static int s_configSelected = 0;
static bool s_configListInitialized = false;

static bool s_offsetUpdatePending = false;
static bool s_offsetUpdateSuccess = false;
static std::string s_offsetUpdateMessage;
static std::future<bool> s_offsetUpdateFuture;

static bool s_offsetReloadPending = false;
static bool s_offsetReloadSuccess = false;
static std::string s_offsetReloadMessage;
static std::future<bool> s_offsetReloadFuture;

static bool s_profileActionSuccess = false;
static std::string s_profileActionMessage;

namespace {

bool HasNonWhitespace(const char *text) {
    if (!text) {
        return false;
    }

    while (*text) {
        if (!std::isspace(static_cast<unsigned char>(*text))) {
            return true;
        }
        ++text;
    }
    return false;
}

void RefreshConfigList() {
    s_configList = Config::ConfigManager::ListConfigs();
    if (s_configSelected >= static_cast<int>(s_configList.size())) {
        s_configSelected = s_configList.empty() ? 0 : static_cast<int>(s_configList.size()) - 1;
    }
}

void PollOffsetJobs() {
    if (s_offsetUpdatePending && s_offsetUpdateFuture.valid() &&
        s_offsetUpdateFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        try {
            s_offsetUpdateSuccess = s_offsetUpdateFuture.get();
        } catch (...) {
            s_offsetUpdateSuccess = false;
        }
        s_offsetUpdatePending = false;
        s_offsetUpdateMessage = s_offsetUpdateSuccess ? "Offsets were updated from GitHub." : "Offset update failed.";
    }

    if (s_offsetReloadPending && s_offsetReloadFuture.valid() &&
        s_offsetReloadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        try {
            s_offsetReloadSuccess = s_offsetReloadFuture.get();
        } catch (...) {
            s_offsetReloadSuccess = false;
        }
        s_offsetReloadPending = false;
        s_offsetReloadMessage = s_offsetReloadSuccess ? "Offsets were reloaded from disk." : "Offset reload failed.";
    }
}

void DrawStatusMessage(const std::string &message, bool pending, bool success) {
    if (message.empty()) {
        return;
    }

    ImVec4 color = pending
        ? ImVec4(0.92f, 0.78f, 0.28f, 1.0f)
        : (success ? ImVec4(0.25f, 0.84f, 0.42f, 1.0f) : ImVec4(0.86f, 0.33f, 0.33f, 1.0f));
    ImGui::TextColored(color, "%s", message.c_str());
}

}

void RenderTabSettings() {
    if (!s_configListInitialized) {
        RefreshConfigList();
        s_configListInitialized = true;
    }

    PollOffsetJobs();

    ImGui::Columns(2, "SetCols", false);

    if (UI::BeginCard("Profiles And Offsets")) {
        UI::SectionHeader("Theme", "Choose the menu look without touching runtime logic.");
        const char *themes[] = {"Midnight", "Blood", "Cyber", "Lavender", "Gold", "Monochrome", "Toxic"};
        if (ImGui::Combo("Theme Preset", &Config::Settings.misc.menuTheme, themes, 7)) {
            Config::ConfigManager::ApplySettings();
        }

        UI::SectionHeader("Configuration Profiles", "Save and load named presets stored next to the executable.");
        ImGui::InputText("Profile Name", s_configName, sizeof(s_configName));
        if (ImGui::Button("Save Current Profile", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            if (!HasNonWhitespace(s_configName)) {
                s_profileActionSuccess = false;
                s_profileActionMessage = "Enter a profile name before saving.";
            } else if (Config::ConfigManager::Save(s_configName)) {
                RefreshConfigList();
                s_profileActionSuccess = true;
                s_profileActionMessage = "Profile saved successfully.";
            } else {
                s_profileActionSuccess = false;
                s_profileActionMessage = Config::ConfigManager::LastError.empty()
                    ? "Profile save failed."
                    : Config::ConfigManager::LastError;
            }
        }
        if (ImGui::Button("Refresh Profile List", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            RefreshConfigList();
            s_profileActionSuccess = true;
            s_profileActionMessage = "Profile list refreshed.";
        }

        ImGui::BeginChild("ConfigList", ImVec2(0, 140), true);
        for (int i = 0; i < static_cast<int>(s_configList.size()); ++i) {
            if (ImGui::Selectable(s_configList[i].c_str(), i == s_configSelected)) {
                s_configSelected = i;
                strncpy_s(s_configName, sizeof(s_configName), s_configList[i].c_str(), _TRUNCATE);
            }
        }
        ImGui::EndChild();

        if (!s_configList.empty()) {
            if (ImGui::Button("Load Selected Profile", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                strncpy_s(s_configName, sizeof(s_configName), s_configList[s_configSelected].c_str(), _TRUNCATE);
                if (Config::ConfigManager::Load(s_configList[s_configSelected])) {
                    s_profileActionSuccess = true;
                    s_profileActionMessage = "Profile loaded successfully.";
                } else {
                    s_profileActionSuccess = false;
                    s_profileActionMessage = Config::ConfigManager::LastError.empty()
                        ? "Profile load failed."
                        : Config::ConfigManager::LastError;
                }
            }
        } else {
            UI::HelpText("No saved profiles were found yet. Saving the current setup will create the first one.");
        }
        DrawStatusMessage(s_profileActionMessage, false, s_profileActionSuccess);

        UI::SectionHeader("Offsets", "Refresh live offsets when the game updates or force a disk reload for troubleshooting.");
        if (ImGui::Button("Update Offsets From GitHub", ImVec2(ImGui::GetContentRegionAvail().x, 32))) {
            s_offsetUpdatePending = true;
            s_offsetUpdateSuccess = false;
            s_offsetUpdateMessage = "Updating offsets...";
            s_offsetUpdateFuture = SDK::Updater::ForceUpdateOffsets();
        }
        if (ImGui::Button("Reload Offsets From Disk", ImVec2(ImGui::GetContentRegionAvail().x, 32))) {
            s_offsetReloadPending = true;
            s_offsetReloadSuccess = false;
            s_offsetReloadMessage = "Reloading cached offsets...";
            s_offsetReloadFuture = SDK::Updater::ReloadOffsets();
        }
        DrawStatusMessage(s_offsetUpdateMessage, s_offsetUpdatePending, s_offsetUpdateSuccess);
        DrawStatusMessage(s_offsetReloadMessage, s_offsetReloadPending, s_offsetReloadSuccess);
    }
    UI::EndCard();

    ImGui::NextColumn();

    if (UI::BeginCard("Performance And Safety")) {
        UI::SectionHeader("Frame Control", "Balance render smoothness, input timing, and memory update cadence.");
        UI::SettingToggle("Enable VSync", &Config::Settings.performance.vsyncEnabled);
        if (ImGui::SliderInt("FPS Limit", &Config::Settings.performance.fpsLimit, 10, 500, "%d FPS")) {
            Config::ConfigManager::ApplySettings();
        }
        if (ImGui::SliderInt("UPS Limit", &Config::Settings.performance.upsLimit, 10, 500, "%d UPS")) {
            Config::ConfigManager::ApplySettings();
        }
        if (Config::Settings.performance.vsyncEnabled) {
            UI::HelpText("When VSync is enabled, render pacing follows the display and UPS can sync to frame timing.");
        }

        UI::SectionHeader("Diagnostics", "Debug output and rendering behavior for troubleshooting.");
        UI::SettingToggle("Enable Debug Overlay", &Config::Settings.debug.enabled);
        UI::SettingToggle("Developer Mode", &Config::Settings.debug.devMode);
        UI::SettingToggle("Frustum Culling", &Config::Settings.esp.frustumCullingEnabled);

        UI::SectionHeader("Session Control", "Leave the overlay cleanly from inside the menu.");
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.12f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.84f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.06f, 0.06f, 1.0f));
        if (ImGui::Button("Unload And Exit", ImVec2(ImGui::GetContentRegionAvail().x, 38))) {
            Menu::shouldClose = true;
        }
        ImGui::PopStyleColor(3);
    }
    UI::EndCard();

    ImGui::Columns(1);
}

} // namespace Render
