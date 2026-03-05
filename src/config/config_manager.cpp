#include "config_manager.h"
#include "features/aimbot/aimbot_config.h"
#include "features/bomb/bomb_config.h"
#include "features/esp/esp_config.h"
#include "features/feature_manager.h"
#include "features/misc/misc_config.h"
#include "features/radar/radar_config.h"
#include "features/triggerbot/triggerbot_config.h"
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

// ─── Save ────────────────────────────────────────────────────────────────────
bool ConfigManager::Save(const std::string &name) {
  fs::create_directories(ConfigDir());
  std::ofstream f(ConfigPath(name));
  if (!f) {
    LastError = "Cannot open file for writing: " + ConfigPath(name).string();
    return false;
  }

  auto &E = Settings.esp;
  auto &A = Settings.aimbot;
  auto &T = Settings.triggerbot;
  auto &M = Settings.misc;
  auto &B = Settings.bomb;
  auto &R = Settings.radar;

  f << "{\n";
  // ESP
  WriteBool(f, "esp_enabled", E.enabled);
  WriteBool(f, "esp_showBox", E.showBox);
  WriteBool(f, "esp_showName", E.showName);
  WriteBool(f, "esp_showHealth", E.showHealth);
  WriteBool(f, "esp_showWeapon", E.showWeapon);
  WriteBool(f, "esp_showDistance", E.showDistance);
  WriteBool(f, "esp_showTeammates", E.showTeammates);
  // Skeleton
  WriteBool(f, "esp_showBones", E.showBones);
  WriteBool(f, "esp_skeletonOutline", E.skeletonOutline);
  WriteFloatArray(f, "esp_skeletonOutlineColor", E.skeletonOutlineColor, 4);
  WriteFloat(f, "esp_skeletonMaxDistance", E.skeletonMaxDistance);
  WriteFloatArray(f, "esp_boneColor", E.boneColor, 4);
  WriteColor(f, "esp_boxColor", E.boxColor);
  WriteColor(f, "esp_teamColor", E.teamColor);
  // Aimbot
  WriteBool(f, "aim_enabled", A.enabled);
  WriteInt(f, "aim_hotkey", A.hotkey);
  WriteInt(f, "aim_bone", A.targetBone);
  WriteFloat(f, "aim_fov", A.fov);
  WriteFloat(f, "aim_smooth", A.smooth);
  WriteFloat(f, "aim_jitter", A.jitter);
  WriteBool(f, "aim_teamCheck", A.teamCheck);
  WriteBool(f, "aim_onlyScoped", A.onlyScoped);
  WriteBool(f, "aim_targetLock", A.targetLock);
  WriteBool(f, "aim_visibleOnly", A.visibleOnly);
  // Radar
  WriteBool(f, "radar_enabled", R.enabled);
  WriteBool(f, "radar_rotate", R.rotate);
  WriteBool(f, "radar_showTeammates", R.showTeammates);
  WriteBool(f, "radar_visibleCheck", R.visibleCheck);
  WriteInt(f, "radar_mapIndex", R.mapIndex);
  WriteFloat(f, "radar_mapCalibration", R.mapCalibration);
  WriteInt(f, "radar_stretchType", R.stretchType);
  WriteFloat(f, "radar_zoom", R.zoom);
  WriteFloat(f, "radar_bgAlpha", R.bgAlpha);
  WriteFloat(f, "radar_pointSize", R.pointSize);
  WriteColor(f, "radar_enemyColor", R.enemyColor);
  WriteColor(f, "radar_teamColor", R.teamColor);
  WriteColor(f, "radar_visibleColor", R.visibleColor);
  WriteColor(f, "radar_hiddenColor", R.hiddenColor);
  // Triggerbot
  WriteBool(f, "tb_enabled", T.enabled);
  WriteInt(f, "tb_hotkey", T.hotkey);
  WriteInt(f, "tb_delayMin", T.delayMin);
  WriteInt(f, "tb_delayMax", T.delayMax);
  WriteBool(f, "tb_teamCheck", T.teamCheck);
  // Misc
  WriteBool(f, "misc_awpCrosshair", M.awpCrosshair);
  WriteInt(f, "misc_style", M.crosshairStyle);
  WriteFloat(f, "misc_size", M.crosshairSize);
  WriteFloat(f, "misc_thickness", M.crosshairThickness);
  WriteColor(f, "misc_color", M.crosshairColor);
  WriteBool(f, "misc_gap", M.crosshairGap);
  WriteInt(f, "misc_menuTheme", M.menuTheme);
  // Bomb
  WriteBool(f, "bomb_enabled", B.enabled);
  // Performance
  WriteInt(f, "perf_fpsLimit", Settings.performance.fpsLimit);
  WriteInt(f, "perf_upsLimit", Settings.performance.upsLimit);
  f << "  \"_end\": 0\n}\n";

  std::cout << "[CONFIG] Saved: " << name << "\n";
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

  auto &E = Settings.esp;
  auto &A = Settings.aimbot;
  auto &T = Settings.triggerbot;
  auto &M = Settings.misc;
  auto &B = Settings.bomb;
  auto &R = Settings.radar;

  // ESP
  E.enabled = ReadB(j, "esp_enabled", E.enabled);
  E.showBox = ReadB(j, "esp_showBox", E.showBox);
  E.showName = ReadB(j, "esp_showName", E.showName);
  E.showHealth = ReadB(j, "esp_showHealth", E.showHealth);
  E.showWeapon = ReadB(j, "esp_showWeapon", E.showWeapon);
  E.showDistance = ReadB(j, "esp_showDistance", E.showDistance);
  E.showTeammates = ReadB(j, "esp_showTeammates", E.showTeammates);
  // Skeleton
  E.showBones = ReadB(j, "esp_showBones", E.showBones);
  E.skeletonOutline = ReadB(j, "esp_skeletonOutline", E.skeletonOutline);
  ReadFloatArray(j, "esp_skeletonOutlineColor", E.skeletonOutlineColor, 4);
  E.skeletonMaxDistance =
      ReadF(j, "esp_skeletonMaxDistance", E.skeletonMaxDistance);
  ReadFloatArray(j, "esp_boneColor", E.boneColor, 4);
  ReadColor(j, "esp_boxColor", E.boxColor);
  ReadColor(j, "esp_teamColor", E.teamColor);
  // Aimbot
  A.enabled = ReadB(j, "aim_enabled", A.enabled);
  A.hotkey = ReadI(j, "aim_hotkey", A.hotkey);
  A.targetBone = ReadI(j, "aim_bone", A.targetBone);
  A.fov = ReadF(j, "aim_fov", A.fov);
  A.smooth = ReadF(j, "aim_smooth", A.smooth);
  A.jitter = ReadF(j, "aim_jitter", A.jitter);
  A.teamCheck = ReadB(j, "aim_teamCheck", A.teamCheck);
  A.onlyScoped = ReadB(j, "aim_onlyScoped", A.onlyScoped);
  A.targetLock = ReadB(j, "aim_targetLock", A.targetLock);
  A.visibleOnly = ReadB(j, "aim_visibleOnly", A.visibleOnly);
  // Radar
  R.enabled = ReadB(j, "radar_enabled", R.enabled);
  R.rotate = ReadB(j, "radar_rotate", R.rotate);
  R.showTeammates = ReadB(j, "radar_showTeammates", R.showTeammates);
  R.visibleCheck = ReadB(j, "radar_visibleCheck", R.visibleCheck);
  R.mapIndex = ReadI(j, "radar_mapIndex", R.mapIndex);
  R.mapCalibration = ReadF(j, "radar_mapCalibration", R.mapCalibration);
  R.stretchType = ReadI(j, "radar_stretchType", R.stretchType);
  R.zoom = ReadF(j, "radar_zoom", R.zoom);
  R.bgAlpha = ReadF(j, "radar_bgAlpha", R.bgAlpha);
  R.pointSize = ReadF(j, "radar_pointSize", R.pointSize);
  ReadColor(j, "radar_enemyColor", R.enemyColor);
  ReadColor(j, "radar_teamColor", R.teamColor);
  ReadColor(j, "radar_visibleColor", R.visibleColor);
  ReadColor(j, "radar_hiddenColor", R.hiddenColor);
  // Triggerbot
  T.enabled = ReadB(j, "tb_enabled", T.enabled);
  T.hotkey = ReadI(j, "tb_hotkey", T.hotkey);
  T.delayMin = ReadI(j, "tb_delayMin", T.delayMin);
  T.delayMax = ReadI(j, "tb_delayMax", T.delayMax);
  T.teamCheck = ReadB(j, "tb_teamCheck", T.teamCheck);
  // Misc
  M.awpCrosshair = ReadB(j, "misc_awpCrosshair", M.awpCrosshair);
  M.crosshairStyle = ReadI(j, "misc_style", M.crosshairStyle);
  M.crosshairSize = ReadF(j, "misc_size", M.crosshairSize);
  M.crosshairThickness = ReadF(j, "misc_thickness", M.crosshairThickness);
  ReadColor(j, "misc_color", M.crosshairColor);
  M.crosshairGap = ReadB(j, "misc_gap", M.crosshairGap);
  M.menuTheme = ReadI(j, "misc_menuTheme", M.menuTheme);
  // Bomb
  B.enabled = ReadB(j, "bomb_enabled", B.enabled);
  // Performance
  Settings.performance.fpsLimit =
      ReadI(j, "perf_fpsLimit", Settings.performance.fpsLimit);
  Settings.performance.upsLimit =
      ReadI(j, "perf_upsLimit", Settings.performance.upsLimit);

  ApplySettings();

  std::cout << "[CONFIG] Loaded: " << name << "\n";
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
