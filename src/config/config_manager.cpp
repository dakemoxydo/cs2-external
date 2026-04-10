#include "config_manager.h"
#include "../features/aimbot/aimbot_config.h"
#include "../features/bomb/bomb_config.h"
#include "../features/esp/esp_config.h"
#include "../features/feature_manager.h"
#include "../features/misc/misc_config.h"
#include "../features/radar/radar_config.h"
#include "../features/triggerbot/triggerbot_config.h"
#include "../features/rcs/rcs_config.h"
#include "../features/footsteps_esp/footsteps_esp_config.h"
#include "settings.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Define the central config state
namespace Config {
GlobalSettings Settings;
std::shared_mutex SettingsMutex;
}

namespace fs = std::filesystem;

namespace Config {

std::string ConfigManager::LastError;

static std::string NormalizeConfigName(std::string name) {
  if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
    name.resize(name.size() - 5);
  }
  return name;
}

static fs::path ConfigDir() {
  char exePath[MAX_PATH];
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  return fs::path(exePath).parent_path() / "configs";
}

static fs::path ConfigPath(const std::string &name) {
  return ConfigDir() / (NormalizeConfigName(name) + ".json");
}

// ─── Reflection Registry ─────────────────────────────────────────────────────
struct ConfigEntry {
  const char *key;
  enum { BOOL, INT, FLOAT, COLOR } type;
  void *ptr;
};

static std::vector<ConfigEntry> BuildRegistry() {
  auto &E = Settings.esp;
  auto &A = Settings.aimbot;
  auto &T = Settings.triggerbot;
  auto &M = Settings.misc;
  auto &B = Settings.bomb;
  auto &R = Settings.radar;
  auto &P = Settings.performance;
  auto &D = Settings.debug;
  auto &RCS = Settings.rcs;
  auto &FE = Settings.footstepsEsp;

  return {
      // ESP
      {"esp_enabled", ConfigEntry::BOOL, &E.enabled},
      {"esp_showBox", ConfigEntry::BOOL, &E.showBox},
      {"esp_boxStyle", ConfigEntry::INT, &E.boxStyle},
      {"esp_fillBoxAlpha", ConfigEntry::FLOAT, &E.fillBoxAlpha},
      {"esp_showName", ConfigEntry::BOOL, &E.showName},
      {"esp_showHealth", ConfigEntry::BOOL, &E.showHealth},
      {"esp_healthBarStyle", ConfigEntry::INT, &E.healthBarStyle},
      {"esp_showHealthText", ConfigEntry::BOOL, &E.showHealthText},
      {"esp_showWeapon", ConfigEntry::BOOL, &E.showWeapon},
      {"esp_showDistance", ConfigEntry::BOOL, &E.showDistance},
      {"esp_showTeammates", ConfigEntry::BOOL, &E.showTeammates},
      {"esp_nameColor", ConfigEntry::COLOR, E.nameColor},
      {"esp_weaponColor", ConfigEntry::COLOR, E.weaponColor},
      {"esp_distColor", ConfigEntry::COLOR, E.distColor},
      {"esp_showBones", ConfigEntry::BOOL, &E.showBones},
      {"esp_skeletonOutline", ConfigEntry::BOOL, &E.skeletonOutline},
      {"esp_skeletonOutlineColor", ConfigEntry::COLOR, E.skeletonOutlineColor},
      {"esp_skeletonMaxDistance", ConfigEntry::FLOAT, &E.skeletonMaxDistance},
      {"esp_boneColor", ConfigEntry::COLOR, E.boneColor},
      {"esp_boxColor", ConfigEntry::COLOR, E.boxColor},
      {"esp_teamColor", ConfigEntry::COLOR, E.teamColor},
      {"esp_showOffscreen", ConfigEntry::BOOL, &E.showOffscreen},
      {"esp_offscreenColor", ConfigEntry::COLOR, E.offscreenColor},
      {"esp_showSnapLines", ConfigEntry::BOOL, &E.showSnapLines},
      {"esp_snapLineColor", ConfigEntry::COLOR, E.snapLineColor},
      {"esp_frustumCullingEnabled", ConfigEntry::BOOL, &E.frustumCullingEnabled},
      // Aimbot
      {"aim_enabled", ConfigEntry::BOOL, &A.enabled},
      {"aim_hotkey", ConfigEntry::INT, &A.hotkey},
      {"aim_bone", ConfigEntry::INT, &A.targetBone},
      {"aim_fov", ConfigEntry::FLOAT, &A.fov},
      {"aim_smooth", ConfigEntry::FLOAT, &A.smooth},
      {"aim_jitter", ConfigEntry::FLOAT, &A.jitter},
      {"aim_sensitivity", ConfigEntry::FLOAT, &A.sensitivity},
      {"aim_teamCheck", ConfigEntry::BOOL, &A.teamCheck},
      {"aim_onlyScoped", ConfigEntry::BOOL, &A.onlyScoped},
      {"aim_targetLock", ConfigEntry::BOOL, &A.targetLock},
      {"aim_visibleOnly", ConfigEntry::BOOL, &A.visibleOnly},
      {"aim_showFov", ConfigEntry::BOOL, &A.showFov},
      // Radar
      {"radar_enabled", ConfigEntry::BOOL, &R.enabled},
      {"radar_rotate", ConfigEntry::BOOL, &R.rotate},
      {"radar_showTeammates", ConfigEntry::BOOL, &R.showTeammates},
      {"radar_visibleCheck", ConfigEntry::BOOL, &R.visibleCheck},
      {"radar_mapIndex", ConfigEntry::INT, &R.mapIndex},
      {"radar_mapCalibration", ConfigEntry::FLOAT, &R.mapCalibration},
      {"radar_stretchType", ConfigEntry::INT, &R.stretchType},
      {"radar_zoom", ConfigEntry::FLOAT, &R.zoom},
      {"radar_bgAlpha", ConfigEntry::FLOAT, &R.bgAlpha},
      {"radar_pointSize", ConfigEntry::FLOAT, &R.pointSize},
      {"radar_enemyColor", ConfigEntry::COLOR, R.enemyColor},
      {"radar_teamColor", ConfigEntry::COLOR, R.teamColor},
      {"radar_visibleColor", ConfigEntry::COLOR, R.visibleColor},
      {"radar_hiddenColor", ConfigEntry::COLOR, R.hiddenColor},
      // Triggerbot
      {"tb_enabled", ConfigEntry::BOOL, &T.enabled},
      {"tb_hotkey", ConfigEntry::INT, &T.hotkey},
      {"tb_delayMin", ConfigEntry::INT, &T.delayMin},
      {"tb_delayMax", ConfigEntry::INT, &T.delayMax},
      {"tb_teamCheck", ConfigEntry::BOOL, &T.teamCheck},
      {"tb_fov", ConfigEntry::FLOAT, &T.fov},
      // Standalone RCS
      {"rcs_enabled", ConfigEntry::BOOL, &RCS.enabled},
      {"rcs_pitch", ConfigEntry::FLOAT, &RCS.pitchStrength},
      {"rcs_yaw", ConfigEntry::FLOAT, &RCS.yawStrength},
      {"rcs_startBullet", ConfigEntry::INT, &RCS.startBullet},
      // Misc
      {"misc_awpCrosshair", ConfigEntry::BOOL, &M.awpCrosshair},
      {"misc_style", ConfigEntry::INT, &M.crosshairStyle},
      {"misc_size", ConfigEntry::FLOAT, &M.crosshairSize},
      {"misc_thickness", ConfigEntry::FLOAT, &M.crosshairThickness},
      {"misc_color", ConfigEntry::COLOR, M.crosshairColor},
      {"misc_gap", ConfigEntry::BOOL, &M.crosshairGap},
      {"misc_menuTheme", ConfigEntry::INT, &M.menuTheme},
      // Bomb
      {"bomb_enabled", ConfigEntry::BOOL, &B.enabled},
      // Performance
      {"perf_vsyncEnabled", ConfigEntry::BOOL, &P.vsyncEnabled},
      {"perf_fpsLimit", ConfigEntry::INT, &P.fpsLimit},
      {"perf_upsLimit", ConfigEntry::INT, &P.upsLimit},
      // Debug
      {"debug_enabled", ConfigEntry::BOOL, &D.enabled},
      {"debug_devMode", ConfigEntry::BOOL, &D.devMode},
      // Footsteps ESP
      {"footstepsEsp_enabled", ConfigEntry::BOOL, &FE.enabled},
      {"footstepsEsp_showTeammates", ConfigEntry::BOOL, &FE.showTeammates},
      {"footstepsEsp_footstepColor", ConfigEntry::COLOR, FE.footstepColor},
      {"footstepsEsp_jumpColor", ConfigEntry::COLOR, FE.jumpColor},
      {"footstepsEsp_landColor", ConfigEntry::COLOR, FE.landColor},
      {"footstepsEsp_footstepMaxRadius", ConfigEntry::FLOAT, &FE.footstepMaxRadius},
      {"footstepsEsp_jumpMaxRadius", ConfigEntry::FLOAT, &FE.jumpMaxRadius},
      {"footstepsEsp_landMaxRadius", ConfigEntry::FLOAT, &FE.landMaxRadius},
      {"footstepsEsp_expandDuration", ConfigEntry::FLOAT, &FE.expandDuration},
      {"footstepsEsp_fadeDuration", ConfigEntry::FLOAT, &FE.fadeDuration},
      {"footstepsEsp_thickness", ConfigEntry::FLOAT, &FE.thickness},
      {"footstepsEsp_segments", ConfigEntry::INT, &FE.segments},
  };
}

// ─── Save ────────────────────────────────────────────────────────────────────
bool ConfigManager::Save(const std::string &name) {
  std::unique_lock<std::shared_mutex> lock(SettingsMutex);
  
  fs::create_directories(ConfigDir());
  
  json j;
  
  // Основные настройки через registry
  auto reg = BuildRegistry();
  for (const auto &e : reg) {
    if (e.type == ConfigEntry::BOOL)
      j[e.key] = *reinterpret_cast<bool *>(e.ptr);
    else if (e.type == ConfigEntry::INT)
      j[e.key] = *reinterpret_cast<int *>(e.ptr);
    else if (e.type == ConfigEntry::FLOAT)
      j[e.key] = *reinterpret_cast<float *>(e.ptr);
    else if (e.type == ConfigEntry::COLOR) {
      const float* c = reinterpret_cast<float *>(e.ptr);
      j[e.key] = {c[0], c[1], c[2], c[3]};
    }
  }
  
  std::ofstream f(ConfigPath(name));
  if (!f) {
    LastError = "Cannot open file for writing: " + ConfigPath(name).string();
    return false;
  }
  
  f << j.dump(2);
  LastError.clear();
  return true;
}

// ─── Load ────────────────────────────────────────────────────────────────────
bool ConfigManager::Load(const std::string &name) {
  std::unique_lock<std::shared_mutex> lock(SettingsMutex);

  std::ifstream f(ConfigPath(name));
  if (!f) {
    LastError.clear();
    Settings = GlobalSettings{};
    ApplySettings();
    return true;
  }
  
  try {
    json j = json::parse(f);
    
    // Основные настройки через registry
    auto reg = BuildRegistry();
    for (const auto &e : reg) {
      if (j.contains(e.key)) {
        if (e.type == ConfigEntry::BOOL)
          *reinterpret_cast<bool *>(e.ptr) = j[e.key].get<bool>();
        else if (e.type == ConfigEntry::INT)
          *reinterpret_cast<int *>(e.ptr) = j[e.key].get<int>();
        else if (e.type == ConfigEntry::FLOAT)
          *reinterpret_cast<float *>(e.ptr) = j[e.key].get<float>();
        else if (e.type == ConfigEntry::COLOR) {
          float* c = reinterpret_cast<float *>(e.ptr);
          auto arr = j[e.key].get<std::vector<float>>();
          if (arr.size() >= 4) {
            c[0] = arr[0]; c[1] = arr[1]; c[2] = arr[2]; c[3] = arr[3];
          }
        }
      }
    }
  } catch (const json::parse_error &e) {
    LastError = "JSON parse error: " + std::string(e.what());
    std::cerr << "Config parse error: " << LastError << "\n";
    Settings = GlobalSettings{};
    ApplySettings();
    return false;
  } catch (const std::exception &e) {
    LastError = "Error loading config: " + std::string(e.what());
    std::cerr << "Config load error: " << LastError << "\n";
    Settings = GlobalSettings{};
    ApplySettings();
    return false;
  }

  ApplySettings();
  LastError.clear();

  return true;
}

// ─── ApplySettings ───────────────────────────────────────────────────────────
void ConfigManager::ApplySettings() {
  // Lazy-init features when their enabled state changes to true
  if (Settings.esp.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("ESP");
  if (Settings.aimbot.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("Aimbot");
  if (Settings.triggerbot.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("Triggerbot");
  if (Settings.misc.awpCrosshair)
    Features::FeatureManager::EnsureFeatureInitialized("Misc");
  if (Settings.bomb.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("Bomb");
  if (Settings.radar.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("Radar");
  if (Settings.debug.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("DebugOverlay");
  if (Settings.rcs.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("RCSSystem");
  if (Settings.footstepsEsp.enabled)
    Features::FeatureManager::EnsureFeatureInitialized("FootstepsEsp");

  // Also handle disabling for already-initialized features
  for (auto &slot : Features::FeatureManager::featureSlots) {
    if (!slot.instance) continue;
    std::string n(slot.instance->GetName());
    bool shouldBeEnabled = false;
    if (n == "ESP") shouldBeEnabled = Settings.esp.enabled;
    else if (n == "Aimbot") shouldBeEnabled = Settings.aimbot.enabled;
    else if (n == "Triggerbot") shouldBeEnabled = Settings.triggerbot.enabled;
    else if (n == "Misc") shouldBeEnabled = Settings.misc.awpCrosshair;
    else if (n == "Bomb") shouldBeEnabled = Settings.bomb.enabled;
    else if (n == "Radar") shouldBeEnabled = Settings.radar.enabled;
    else if (n == "DebugOverlay") shouldBeEnabled = Settings.debug.enabled;
    else if (n == "RCSSystem") shouldBeEnabled = Settings.rcs.enabled;
    else if (n == "FootstepsEsp") shouldBeEnabled = Settings.footstepsEsp.enabled;

    if (!shouldBeEnabled && slot.instance->IsEnabled())
      slot.instance->SetEnabled(false);
  }
}

// ─── ListConfigs ─────────────────────────────────────────────────────────────
std::vector<std::string> ConfigManager::ListConfigs() {
  std::vector<std::string> names;
  std::error_code ec;
  for (auto &e : fs::directory_iterator(ConfigDir(), ec)) {
    if (e.path().extension().string() == ".json")
      names.push_back(e.path().stem().string());
  }
  return names;
}

void ConfigManager::LoadDefault() {
  std::unique_lock<std::shared_mutex> lock(SettingsMutex);
  Settings = GlobalSettings{};
  ApplySettings();
  LastError.clear();
}

} // namespace Config
