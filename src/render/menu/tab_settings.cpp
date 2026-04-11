#include "tab_settings.h"
#include "menu.h"
#include "ui_components.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "core/sdk/updater.h"
#include <cctype>
#include <cstring>
#include <imgui.h>
#include <string>
#include <utility>
#include <vector>

namespace Render {

static char s_configName[64] = "default";
static std::vector<std::string> s_configList;
static int s_configSelected = 0;
static bool s_configListInitialized = false;

static bool s_offsetUpdatePending = false;
static bool s_offsetUpdateSuccess = false;
static std::string s_offsetUpdateMessage;
static SDK::OffsetUpdateJob s_offsetUpdateJob;

static bool s_offsetReloadPending = false;
static bool s_offsetReloadSuccess = false;
static std::string s_offsetReloadMessage;
static SDK::OffsetUpdateJob s_offsetReloadJob;

static bool s_profileActionSuccess = false;
static std::string s_profileActionMessage;

namespace {

template <typename Fn>
void Commit(Fn &&fn) {
  Config::MutateSettingsVoid(std::forward<Fn>(fn));
}

std::string SanitizeProfileName(const char *input) {
  std::string result;
  if (!input) {
    return result;
  }

  result.reserve(std::strlen(input));
  for (const unsigned char ch : std::string(input)) {
    switch (ch) {
    case '<':
    case '>':
    case ':':
    case '"':
    case '/':
    case '\\':
    case '|':
    case '?':
    case '*':
      result.push_back('_');
      break;
    default:
      result.push_back(static_cast<char>(ch));
      break;
    }
  }

  while (!result.empty() && (result.back() == ' ' || result.back() == '.')) {
    result.pop_back();
  }

  return result;
}

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
    s_configSelected =
        s_configList.empty() ? 0 : static_cast<int>(s_configList.size()) - 1;
  }
}

void PollOffsetJobs() {
  if (s_offsetUpdatePending && s_offsetUpdateJob.IsReady()) {
    s_offsetUpdateSuccess = s_offsetUpdateJob.Succeeded();
    s_offsetUpdatePending = false;
    s_offsetUpdateMessage = s_offsetUpdateSuccess
                                ? "Offsets were updated from GitHub."
                                : "Offset update failed.";
  }

  if (s_offsetReloadPending && s_offsetReloadJob.IsReady()) {
    s_offsetReloadSuccess = s_offsetReloadJob.Succeeded();
    s_offsetReloadPending = false;
    s_offsetReloadMessage = s_offsetReloadSuccess
                                ? "Offsets were reloaded from disk."
                                : "Offset reload failed.";
  }
}

bool AnyOffsetJobPending() {
  return s_offsetUpdatePending || s_offsetReloadPending;
}

void DrawStatusMessage(const std::string &message, bool pending, bool success) {
  if (message.empty()) {
    return;
  }

  ImVec4 color =
      pending ? ImVec4(0.92f, 0.78f, 0.28f, 1.0f)
              : (success ? ImVec4(0.25f, 0.84f, 0.42f, 1.0f)
                         : ImVec4(0.86f, 0.33f, 0.33f, 1.0f));
  ImGui::TextColored(color, "%s", message.c_str());
}

} // namespace

void RenderTabSettings() {
  if (!s_configListInitialized) {
    RefreshConfigList();
    s_configListInitialized = true;
  }

  PollOffsetJobs();
  Config::GlobalSettings settings = Config::CopySettings();

  if (UI::BeginCard("Profiles And Offsets")) {
    UI::SectionHeader("Theme");
    const char *themes[] = {"Midnight", "Blood", "Cyber", "Lavender",
                            "Gold", "Monochrome", "Toxic"};
    int menuTheme = settings.misc.menuTheme;
    if (ImGui::Combo("Theme Preset", &menuTheme, themes, 7)) {
      Commit([&](auto &state) { state.misc.menuTheme = menuTheme; });
      settings.misc.menuTheme = menuTheme;
    }

    UI::SectionHeader("Profiles");
    ImGui::InputText("Profile Name", s_configName, sizeof(s_configName));
    if (ImGui::Button("Save Current Profile", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
      std::string profileName = SanitizeProfileName(s_configName);
      if (!HasNonWhitespace(profileName.c_str())) {
        s_profileActionSuccess = false;
        s_profileActionMessage = "Enter a profile name before saving.";
      } else if (Config::ConfigManager::Save(profileName)) {
        strncpy_s(s_configName, sizeof(s_configName), profileName.c_str(), _TRUNCATE);
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
        strncpy_s(s_configName, sizeof(s_configName), s_configList[i].c_str(),
                  _TRUNCATE);
      }
    }
    ImGui::EndChild();

    if (!s_configList.empty()) {
      if (ImGui::Button("Load Selected Profile",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        std::string profileName =
            SanitizeProfileName(s_configList[s_configSelected].c_str());
        if (Config::ConfigManager::Load(profileName)) {
          strncpy_s(s_configName, sizeof(s_configName),
                    s_configList[s_configSelected].c_str(), _TRUNCATE);
          s_profileActionSuccess = true;
          s_profileActionMessage = "Profile loaded successfully.";
        } else {
          s_profileActionSuccess = false;
          s_profileActionMessage = Config::ConfigManager::LastError.empty()
                                       ? "Profile load failed."
                                       : Config::ConfigManager::LastError;
        }
      }
    }
    DrawStatusMessage(s_profileActionMessage, false, s_profileActionSuccess);

    UI::SectionHeader("Offsets");
    ImGui::BeginDisabled(AnyOffsetJobPending());
    if (ImGui::Button("Update Offsets From GitHub",
                      ImVec2(ImGui::GetContentRegionAvail().x, 32))) {
      s_offsetUpdatePending = true;
      s_offsetUpdateSuccess = false;
      s_offsetUpdateMessage = "Updating offsets...";
      s_offsetUpdateJob = SDK::Updater::ForceUpdateOffsets();
    }
    if (ImGui::Button("Reload Offsets From Disk",
                      ImVec2(ImGui::GetContentRegionAvail().x, 32))) {
      s_offsetReloadPending = true;
      s_offsetReloadSuccess = false;
      s_offsetReloadMessage = "Reloading cached offsets...";
      s_offsetReloadJob = SDK::Updater::ReloadOffsets();
    }
    ImGui::EndDisabled();
    DrawStatusMessage(s_offsetUpdateMessage, s_offsetUpdatePending,
                      s_offsetUpdateSuccess);
    DrawStatusMessage(s_offsetReloadMessage, s_offsetReloadPending,
                      s_offsetReloadSuccess);
  }
  UI::EndCard();

  if (UI::BeginCard("Performance And Safety")) {
    UI::SectionHeader("Frame");

    bool vsyncEnabled = settings.performance.vsyncEnabled;
    if (UI::SettingToggle("Enable VSync", &vsyncEnabled)) {
      Commit([&](auto &state) { state.performance.vsyncEnabled = vsyncEnabled; });
      settings.performance.vsyncEnabled = vsyncEnabled;
    }

    int fpsLimit = settings.performance.fpsLimit;
    if (ImGui::SliderInt("FPS Limit", &fpsLimit, 10, 500, "%d FPS")) {
      Commit([&](auto &state) { state.performance.fpsLimit = fpsLimit; });
      settings.performance.fpsLimit = fpsLimit;
    }

    int upsLimit = settings.performance.upsLimit;
    if (ImGui::SliderInt("UPS Limit", &upsLimit, 10, 500, "%d UPS")) {
      Commit([&](auto &state) { state.performance.upsLimit = upsLimit; });
      settings.performance.upsLimit = upsLimit;
    }

    UI::SectionHeader("Diagnostics");

    bool debugEnabled = settings.debug.enabled;
    if (UI::SettingToggle("Enable Debug Overlay", &debugEnabled)) {
      Commit([&](auto &state) { state.debug.enabled = debugEnabled; });
      settings.debug.enabled = debugEnabled;
    }

    bool devMode = settings.debug.devMode;
    if (UI::SettingToggle("Developer Mode", &devMode)) {
      Commit([&](auto &state) { state.debug.devMode = devMode; });
      settings.debug.devMode = devMode;
    }

    UI::SectionHeader("Session");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.12f, 0.12f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.84f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.45f, 0.06f, 0.06f, 1.0f));
    if (ImGui::Button("Unload And Exit",
                      ImVec2(ImGui::GetContentRegionAvail().x, 38))) {
      Menu::shouldClose = true;
    }
    ImGui::PopStyleColor(3);
  }
  UI::EndCard();
}

} // namespace Render
