#include "config_manager.h"
#include "../features/aimbot/aimbot_config.h"
#include "../features/bomb/bomb_config.h"
#include "../features/esp/esp_config.h"
#include "../features/feature_manager.h"
#include "../features/misc/misc_config.h"
#include "../features/radar/radar_config.h"
#include "../features/triggerbot/triggerbot_config.h"
#include "../features/rcs/rcs_config.h"
#include "settings.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <windows.h>

// Define the central config state
namespace Config {
GlobalSettings Settings;
}

namespace fs = std::filesystem;

namespace Config {

std::string ConfigManager::LastError;

static fs::path ConfigDir() {
  char exePath[MAX_PATH];
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  return fs::path(exePath).parent_path() / "configs";
}

static fs::path ConfigPath(const std::string &name) {
  return ConfigDir() / (name + ".json");
}

// ─── Tiny manual JSON helpers (no external library) ──────────────────────────
static void WriteFloat(std::ostream &o, const char *k, float v) {
  o << "  \"" << k << "\": " << v << ",\n";
}
static void WriteInt(std::ostream &o, const char *k, int v) {
  o << "  \"" << k << "\": " << v << ",\n";
}
static void WriteBool(std::ostream &o, const char *k, bool v) {
  o << "  \"" << k << "\": " << (v ? "true" : "false") << ",\n";
}
static void WriteFloatArray(std::ostream &o, const char *k, const float c[],
                            int size) {
  o << "  \"" << k << "\": [";
  for (int i = 0; i < size; ++i) {
    o << c[i];
    if (i < size - 1)
      o << ",";
  }
  o << "],\n";
}
static void WriteColor(std::ostream &o, const char *k, const float c[4]) {
  WriteFloatArray(o, k, c, 4);
}

static float ReadF(const std::string &j, const char *k, float def) {
  std::string p = std::string("\"") + k + "\": ";
  auto pos = j.find(p);
  if (pos == j.npos)
    return def;
  try {
    return std::stof(j.substr(pos + p.size()));
  } catch (...) {
    return def;
  }
}
static int ReadI(const std::string &j, const char *k, int def) {
  std::string p = std::string("\"") + k + "\": ";
  auto pos = j.find(p);
  if (pos == j.npos)
    return def;
  try {
    return std::stoi(j.substr(pos + p.size()));
  } catch (...) {
    return def;
  }
}
static bool ReadB(const std::string &j, const char *k, bool def) {
  std::string p = std::string("\"") + k + "\": ";
  auto pos = j.find(p);
  if (pos == j.npos)
    return def;
  return j.substr(pos + p.size(), 4) == "true";
}
static void ReadFloatArray(const std::string &j, const char *k, float c[],
                           int size) {
  std::string p = std::string("\"") + k + "\": [";
  auto pos = j.find(p);
  if (pos == j.npos)
    return;
  std::string s = j.substr(pos + p.size());
  try {
    size_t current_pos = 0;
    for (int i = 0; i < size; ++i) {
      c[i] = std::stof(s, &current_pos);
      s = s.substr(current_pos + 1); // Move past the number and comma/bracket
    }
  } catch (...) {
  }
}
static void ReadColor(const std::string &j, const char *k, float c[4]) {
  ReadFloatArray(j, k, c, 4);
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

  return {
      // ESP
      {"esp_enabled", ConfigEntry::BOOL, &E.enabled},
      {"esp_showBox", ConfigEntry::BOOL, &E.showBox},
      {"esp_showName", ConfigEntry::BOOL, &E.showName},
      {"esp_showHealth", ConfigEntry::BOOL, &E.showHealth},
      {"esp_showWeapon", ConfigEntry::BOOL, &E.showWeapon},
      {"esp_showDistance", ConfigEntry::BOOL, &E.showDistance},
      {"esp_showTeammates", ConfigEntry::BOOL, &E.showTeammates},
      {"esp_showBones", ConfigEntry::BOOL, &E.showBones},
      {"esp_skeletonOutline", ConfigEntry::BOOL, &E.skeletonOutline},
      {"esp_skeletonOutlineColor", ConfigEntry::COLOR, E.skeletonOutlineColor},
      {"esp_skeletonMaxDistance", ConfigEntry::FLOAT, &E.skeletonMaxDistance},
      {"esp_boneColor", ConfigEntry::COLOR, E.boneColor},
      {"esp_boxColor", ConfigEntry::COLOR, E.boxColor},
      {"esp_teamColor", ConfigEntry::COLOR, E.teamColor},
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
      {"perf_fpsLimit", ConfigEntry::INT, &P.fpsLimit},
      {"perf_upsLimit", ConfigEntry::INT, &P.upsLimit},
      // Debug
      {"debug_enabled", ConfigEntry::BOOL, &D.enabled},
      {"debug_devMode", ConfigEntry::BOOL, &D.devMode},
  };
}

// ─── Save ────────────────────────────────────────────────────────────────────
bool ConfigManager::Save(const std::string &name) {
  fs::create_directories(ConfigDir());
  std::ofstream f(ConfigPath(name));
  if (!f) {
    LastError = "Cannot open file for writing: " + ConfigPath(name).string();
    return false;
  }

  f << "{\n";
  auto reg = BuildRegistry();
  for (const auto &e : reg) {
    if (e.type == ConfigEntry::BOOL)
      WriteBool(f, e.key, *reinterpret_cast<bool *>(e.ptr));
    else if (e.type == ConfigEntry::INT)
      WriteInt(f, e.key, *reinterpret_cast<int *>(e.ptr));
    else if (e.type == ConfigEntry::FLOAT)
      WriteFloat(f, e.key, *reinterpret_cast<float *>(e.ptr));
    else if (e.type == ConfigEntry::COLOR)
      WriteColor(f, e.key, reinterpret_cast<float *>(e.ptr));
  }
  f << "  \"_end\": 0\n}\n";

  return true;
}

// ─── Load ────────────────────────────────────────────────────────────────────
bool ConfigManager::Load(const std::string &name) {
  std::ifstream f(ConfigPath(name));
  if (!f) {
    LastError = "Config not found: " + name;
    return false;
  }

  std::string j((std::istreambuf_iterator<char>(f)), {});
  auto reg = BuildRegistry();

  for (const auto &e : reg) {
    if (e.type == ConfigEntry::BOOL)
      *reinterpret_cast<bool *>(e.ptr) = ReadB(j, e.key, *reinterpret_cast<bool *>(e.ptr));
    else if (e.type == ConfigEntry::INT)
      *reinterpret_cast<int *>(e.ptr) = ReadI(j, e.key, *reinterpret_cast<int *>(e.ptr));
    else if (e.type == ConfigEntry::FLOAT)
      *reinterpret_cast<float *>(e.ptr) = ReadF(j, e.key, *reinterpret_cast<float *>(e.ptr));
    else if (e.type == ConfigEntry::COLOR)
      ReadColor(j, e.key, reinterpret_cast<float *>(e.ptr));
  }

  ApplySettings();

  return true;
}

// ─── ApplySettings ───────────────────────────────────────────────────────────
void ConfigManager::ApplySettings() {
  for (auto &f : Features::FeatureManager::features) {
    std::string n = f->GetName();
    if (n == "ESP")
      f->SetEnabled(Settings.esp.enabled);
    else if (n == "Aimbot")
      f->SetEnabled(Settings.aimbot.enabled);
    else if (n == "Triggerbot")
      f->SetEnabled(Settings.triggerbot.enabled);
    else if (n == "Misc")
      f->SetEnabled(Settings.misc.awpCrosshair);
    else if (n == "Bomb")
      f->SetEnabled(Settings.bomb.enabled);
    else if (n == "Radar")
      f->SetEnabled(Settings.radar.enabled);
    else if (n == "DebugOverlay")
      f->SetEnabled(Settings.debug.enabled);
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
  // No-op: defaults are already set by struct constructors
}

} // namespace Config
