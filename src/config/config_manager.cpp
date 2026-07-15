#include "config_manager.h"
#include "../features/aimbot/aimbot_config.h"
#include "../features/bomb/bomb_config.h"
#include "../features/chams/chams_config.h"
#include "../features/esp/esp_config.h"
#include "../features/feature_manager.h"
#include "../features/misc/misc_config.h"
#include "../features/radar/radar_config.h"
#include "../features/triggerbot/triggerbot_config.h"
#include "../features/rcs/rcs_config.h"
#include "../features/sound_esp/sound_esp_config.h"
#include "settings.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
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
static constexpr int kConfigSchemaVersion = 1;

static std::string NormalizeConfigName(std::string name) {
  if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
    name.resize(name.size() - 5);
  }
  return name;
}

static bool IsSafeConfigName(const std::string &name) {
  const std::string normalized = NormalizeConfigName(name);
  if (normalized.empty() || normalized == "." || normalized == ".." ||
      normalized.size() > 63 || normalized.find("..") != std::string::npos) {
    return false;
  }
  for (const unsigned char ch : normalized) {
    if (!(std::isalnum(ch) || ch == '_' || ch == '-' || ch == ' ')) return false;
  }
  return true;
}

static void ClampSettings(GlobalSettings &settings) {
  auto color = [](float (&v)[4]) {
    for (float &component : v) {
      component = std::isfinite(component) ? std::clamp(component, 0.0f, 1.0f) : 0.0f;
    }
  };
  auto finite = [](float &v, float low, float high) {
    v = std::isfinite(v) ? std::clamp(v, low, high) : low;
  };
  settings.performance.fpsLimit = std::clamp(settings.performance.fpsLimit, 10, 500);
  settings.performance.upsLimit = std::clamp(settings.performance.upsLimit, 10, 500);
  settings.misc.menuTheme = std::clamp(settings.misc.menuTheme, 0, 6);
  settings.misc.crosshairStyle = std::clamp(settings.misc.crosshairStyle, 0, 3);
  finite(settings.misc.crosshairSize, 1.0f, 100.0f);
  finite(settings.misc.crosshairThickness, 0.5f, 10.0f);
  finite(settings.aimbot.fov, 0.1f, 180.0f);
  finite(settings.aimbot.smooth, 1.0f, 100.0f);
  finite(settings.aimbot.sensitivity, 0.01f, 20.0f);
  finite(settings.aimbot.jitter, 0.0f, 10.0f);
  settings.triggerbot.delayMin = std::clamp(settings.triggerbot.delayMin, 0, 1000);
  settings.triggerbot.delayMax = std::clamp(settings.triggerbot.delayMax, settings.triggerbot.delayMin, 2000);
  settings.rcs.startBullet = std::clamp(settings.rcs.startBullet, 1, 100);
  finite(settings.rcs.pitchStrength, 0.0f, 10.0f);
  finite(settings.rcs.yawStrength, 0.0f, 10.0f);
  finite(settings.rcs.smooth, 1.0f, 100.0f);
  finite(settings.radar.zoom, 0.01f, 10.0f);
  finite(settings.radar.pointSize, 1.0f, 32.0f);
  settings.soundEsp.segments = std::clamp(settings.soundEsp.segments, 12, 128);
  finite(settings.soundEsp.expandDuration, 0.01f, 10.0f);
  finite(settings.soundEsp.fadeDuration, 0.01f, 10.0f);
  finite(settings.soundEsp.thickness, 0.1f, 20.0f);
  finite(settings.chams.alpha, 0.1f, 1.0f);
  finite(settings.chams.wireAmount, 0.0f, 1.0f);
  color(settings.misc.crosshairColor);
  color(settings.esp.boxColor); color(settings.esp.teamColor); color(settings.esp.nameColor);
  color(settings.esp.weaponColor); color(settings.esp.distColor); color(settings.esp.snapLineColor);
  color(settings.esp.boneColor); color(settings.esp.skeletonOutlineColor); color(settings.esp.offscreenColor);
  color(settings.esp.bulletTracerColor); color(settings.esp.bulletTracerImpactColor); color(settings.esp.hitmarkerColor);
  color(settings.chams.fillColor); color(settings.chams.hiddenColor); color(settings.chams.fillColorTeam);
  color(settings.chams.hiddenColorTeam); color(settings.chams.wireColor);
  color(settings.radar.visibleColor); color(settings.radar.hiddenColor); color(settings.radar.enemyColor);
  color(settings.radar.teamColor); color(settings.soundEsp.footstepColor); color(settings.soundEsp.jumpColor);
  color(settings.soundEsp.landColor);
}

static fs::path ConfigDir() {
  char exePath[MAX_PATH];
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  return fs::path(exePath).parent_path() / "configs";
}

static fs::path ConfigPath(const std::string &name) {
  return ConfigDir() / (NormalizeConfigName(name) + ".json");
}

static bool WriteTextAtomically(const fs::path &destination,
                                const std::string &content,
                                std::string &error) {
  const auto threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
  const fs::path temporary =
      destination.wstring() + L"." + std::to_wstring(GetCurrentProcessId()) +
      L"." + std::to_wstring(threadId) + L".tmp";
  std::error_code ec;
  fs::remove(temporary, ec);

  {
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    if (!file) {
      error = "Cannot open temporary file for writing: " + temporary.string();
      return false;
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.flush();
    if (!file.good()) {
      file.close();
      fs::remove(temporary, ec);
      error = "Failed while writing temporary file: " + temporary.string();
      return false;
    }
  }

  if (!MoveFileExW(temporary.c_str(), destination.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    const DWORD code = GetLastError();
    fs::remove(temporary, ec);
    error = "Failed to replace config file (Windows error " +
            std::to_string(code) + "): " + destination.string();
    return false;
  }
  return true;
}

// ─── Reflection Registry ─────────────────────────────────────────────────────
struct ConfigEntry {
  const char *key;
  enum { BOOL, INT, FLOAT, COLOR } type;
  void *ptr;
};

static std::vector<ConfigEntry> BuildRegistry(GlobalSettings &settings) {
  auto &E = settings.esp;
  auto &A = settings.aimbot;
  auto &T = settings.triggerbot;
  auto &M = settings.misc;
  auto &B = settings.bomb;
  auto &C = settings.chams;
  auto &R = settings.radar;
  auto &P = settings.performance;
  auto &D = settings.debug;
  auto &RCS = settings.rcs;
  auto &SE = settings.soundEsp;

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
      {"esp_showBulletTracers", ConfigEntry::BOOL, &E.showBulletTracers},
      {"esp_bulletTracerColor", ConfigEntry::COLOR, E.bulletTracerColor},
      {"esp_bulletTracerThickness", ConfigEntry::FLOAT, &E.bulletTracerThickness},
      {"esp_bulletTracerLife", ConfigEntry::FLOAT, &E.bulletTracerLife},
      {"esp_bulletTracerImpactColor", ConfigEntry::COLOR, E.bulletTracerImpactColor},
      {"esp_bulletTracerImpactRadius", ConfigEntry::FLOAT, &E.bulletTracerImpactRadius},
      {"esp_bulletTracerImpactThickness", ConfigEntry::FLOAT, &E.bulletTracerImpactThickness},
      {"esp_showHitmarker", ConfigEntry::BOOL, &E.showHitmarker},
      {"esp_hitmarkerColor", ConfigEntry::COLOR, E.hitmarkerColor},
      {"esp_hitmarkerLife", ConfigEntry::FLOAT, &E.hitmarkerLife},
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
      // Standalone RCS
      {"rcs_enabled", ConfigEntry::BOOL, &RCS.enabled},
      {"rcs_key", ConfigEntry::INT, &RCS.key},
      {"rcs_pitch", ConfigEntry::FLOAT, &RCS.pitchStrength},
      {"rcs_yaw", ConfigEntry::FLOAT, &RCS.yawStrength},
      {"rcs_smooth", ConfigEntry::FLOAT, &RCS.smooth},
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
      // Chams
      {"chams_enabled", ConfigEntry::BOOL, &C.enabled},
      {"chams_showTeammates", ConfigEntry::BOOL, &C.showTeammates},
      {"chams_wireframe", ConfigEntry::BOOL, &C.wireframe},
      {"chams_visibleCheck", ConfigEntry::BOOL, &C.visibleCheck},
      {"chams_materialType", ConfigEntry::INT, &C.materialType},
      {"chams_alpha", ConfigEntry::FLOAT, &C.alpha},
      {"chams_wireAmount", ConfigEntry::FLOAT, &C.wireAmount},
      {"chams_fillColor", ConfigEntry::COLOR, C.fillColor},
      {"chams_hiddenColor", ConfigEntry::COLOR, C.hiddenColor},
      {"chams_fillColorTeam", ConfigEntry::COLOR, C.fillColorTeam},
      {"chams_hiddenColorTeam", ConfigEntry::COLOR, C.hiddenColorTeam},
      {"chams_wireColor", ConfigEntry::COLOR, C.wireColor},
      // Performance
      {"perf_vsyncEnabled", ConfigEntry::BOOL, &P.vsyncEnabled},
      {"perf_fpsLimit", ConfigEntry::INT, &P.fpsLimit},
      {"perf_upsLimit", ConfigEntry::INT, &P.upsLimit},
      // Debug
      {"debug_enabled", ConfigEntry::BOOL, &D.enabled},
      {"debug_devMode", ConfigEntry::BOOL, &D.devMode},
      // Sound ESP
      {"soundEsp_enabled", ConfigEntry::BOOL, &SE.enabled},
      {"soundEsp_showTeammates", ConfigEntry::BOOL, &SE.showTeammates},
      {"soundEsp_footstepColor", ConfigEntry::COLOR, SE.footstepColor},
      {"soundEsp_jumpColor", ConfigEntry::COLOR, SE.jumpColor},
      {"soundEsp_landColor", ConfigEntry::COLOR, SE.landColor},
      {"soundEsp_footstepMaxRadius", ConfigEntry::FLOAT, &SE.footstepMaxRadius},
      {"soundEsp_jumpMaxRadius", ConfigEntry::FLOAT, &SE.jumpMaxRadius},
      {"soundEsp_landMaxRadius", ConfigEntry::FLOAT, &SE.landMaxRadius},
      {"soundEsp_expandDuration", ConfigEntry::FLOAT, &SE.expandDuration},
      {"soundEsp_fadeDuration", ConfigEntry::FLOAT, &SE.fadeDuration},
      {"soundEsp_thickness", ConfigEntry::FLOAT, &SE.thickness},
      {"soundEsp_segments", ConfigEntry::INT, &SE.segments},
  };
}

// ─── Save ────────────────────────────────────────────────────────────────────
bool ConfigManager::Save(const std::string &name) {
  if (!IsSafeConfigName(name)) {
    LastError = "Invalid profile name";
    return false;
  }
  GlobalSettings snapshot = CopySettings();
  std::error_code directoryError;
  fs::create_directories(ConfigDir(), directoryError);
  if (directoryError) {
    LastError = "Cannot create config directory: " + directoryError.message();
    return false;
  }
  
  json j;
  j["schema_version"] = kConfigSchemaVersion;
  
  // Основные настройки через registry
  auto reg = BuildRegistry(snapshot);
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
  
  const std::string serialized = j.dump(2);
  if (!WriteTextAtomically(ConfigPath(name), serialized, LastError)) {
    return false;
  }
  LastError.clear();
  return true;
}

// ─── Load ────────────────────────────────────────────────────────────────────
bool ConfigManager::Load(const std::string &name) {
  if (!IsSafeConfigName(name) && NormalizeConfigName(name) != "default") {
    LastError = "Invalid profile name";
    return false;
  }
  const std::string normalizedName = NormalizeConfigName(name);
  std::ifstream f(ConfigPath(name));
  if (!f) {
    if (normalizedName == "default") {
      LoadDefault();
      return true;
    }

    LastError = "Config file not found: " + ConfigPath(name).string();
    return false;
  }
  
  GlobalSettings candidate = CopySettings();
  try {
    json j = json::parse(f);
    if (j.contains("schema_version")) {
      if (!j["schema_version"].is_number_integer()) {
        LastError = "Invalid config schema_version";
        return false;
      }
      const int version = j["schema_version"].get<int>();
      if (version < 1 || version > kConfigSchemaVersion) {
        LastError = "Unsupported config schema version: " +
                    std::to_string(version);
        return false;
      }
    }
    
    // Основные настройки через registry
    auto reg = BuildRegistry(candidate);
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
    return false;
  } catch (const std::exception &e) {
    LastError = "Error loading config: " + std::string(e.what());
    std::cerr << "Config load error: " << LastError << "\n";
    return false;
  }

  ClampSettings(candidate);
  {
    std::unique_lock<std::shared_mutex> lock(SettingsMutex);
    Settings = candidate;
  }
  Detail::ApplySettings(candidate);
  LastError.clear();

  return true;
}

// ─── ApplySettings ───────────────────────────────────────────────────────────
namespace Detail {

void ApplySettings(const GlobalSettings& snapshot) {
  ConfigManager::ApplySettings(snapshot);
}

} // namespace Detail

void ConfigManager::ApplySettings(const GlobalSettings& settings) {
  // Lazy-init features when their enabled state changes to true
  // Apply all feature states through the manager facade. The manager owns
  // creation and lifecycle; config code must not inspect its storage.
  static constexpr std::pair<std::string_view, bool (*)(const GlobalSettings &)>
      features[] = {
          {"ESP", [](const auto &s) { return s.esp.enabled; }},
          {"Aimbot", [](const auto &s) { return s.aimbot.enabled; }},
          {"Triggerbot", [](const auto &s) { return s.triggerbot.enabled; }},
          {"Misc", [](const auto &s) { return s.misc.awpCrosshair; }},
          {"Bomb", [](const auto &s) { return s.bomb.enabled; }},
          {"Chams", [](const auto &s) { return s.chams.enabled; }},
          {"Radar", [](const auto &s) { return s.radar.enabled; }},
          {"DebugOverlay", [](const auto &s) { return s.debug.enabled; }},
          {"RCSSystem", [](const auto &s) { return s.rcs.enabled; }},
          {"SoundEsp", [](const auto &s) { return s.soundEsp.enabled; }},
      };
  for (const auto &[name, enabled] : features) {
    Features::FeatureManager::SetEnabled(name, enabled(settings));
  }
}

// ─── ApplySettingsThreadSafe ─────────────────────────────────────────────────

// ─── ListConfigs ─────────────────────────────────────────────────────────────
std::vector<std::string> ConfigManager::ListConfigs() {
  std::vector<std::string> names;
  std::error_code ec;
  for (auto &e : fs::directory_iterator(ConfigDir(), ec)) {
    if (e.path().extension().string() == ".json")
      names.push_back(e.path().stem().string());
  }
  std::sort(names.begin(), names.end());
  return names;
}

void ConfigManager::LoadDefault() {
  GlobalSettings defaults;
  ClampSettings(defaults);
  {
    std::unique_lock<std::shared_mutex> lock(SettingsMutex);
    Settings = defaults;
  }
  Detail::ApplySettings(defaults);
  LastError.clear();
}

} // namespace Config
