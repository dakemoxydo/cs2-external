#include "config_manager.h"
#include "features/aimbot/aimbot_config.h"
#include "features/bomb/bomb.h"
#include "features/esp/esp_config.h"
#include "features/misc/misc_config.h"
#include "features/triggerbot/triggerbot_config.h"
#include "settings.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <windows.h>

// Forward-declare the feature globals we need to read/write
namespace Features {
extern EspConfig config;
extern AimbotConfig aimbotConfig;
extern TriggerbotConfig triggerbotConfig;
extern MiscConfig miscConfig;
extern BombConfig bombConfig;
} // namespace Features

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
static void WriteColor(std::ostream &o, const char *k, const float c[4]) {
  o << "  \"" << k << "\": [" << c[0] << "," << c[1] << "," << c[2] << ","
    << c[3] << "],\n";
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
static void ReadColor(const std::string &j, const char *k, float c[4]) {
  std::string p = std::string("\"") + k + "\": [";
  auto pos = j.find(p);
  if (pos == j.npos)
    return;
  std::string s = j.substr(pos + p.size());
  try {
    size_t i0 = 0, i1, i2, i3;
    c[0] = std::stof(s, &i0);
    s = s.substr(i0 + 1);
    c[1] = std::stof(s, &i1);
    s = s.substr(i1 + 1);
    c[2] = std::stof(s, &i2);
    s = s.substr(i2 + 1);
    c[3] = std::stof(s, &i3);
  } catch (...) {
  }
}

// ─── Save ────────────────────────────────────────────────────────────────────
bool ConfigManager::Save(const std::string &name) {
  fs::create_directories(ConfigDir());
  std::ofstream f(ConfigPath(name));
  if (!f) {
    LastError = "Cannot open file for writing: " + ConfigPath(name).string();
    return false;
  }

  auto &E = Features::config;
  auto &A = Features::aimbotConfig;
  auto &T = Features::triggerbotConfig;
  auto &M = Features::miscConfig;
  auto &B = Features::bombConfig;

  f << "{\n";
  // ESP
  WriteBool(f, "esp_enabled", E.enabled);
  WriteBool(f, "esp_showBox", E.showBox);
  WriteBool(f, "esp_showName", E.showName);
  WriteBool(f, "esp_showHealth", E.showHealth);
  WriteBool(f, "esp_showWeapon", E.showWeapon);
  WriteBool(f, "esp_showDistance", E.showDistance);
  WriteBool(f, "esp_showTeammates", E.showTeammates);
  WriteBool(f, "esp_showBones", E.showBones);
  WriteFloat(f, "esp_skeletonMaxDist", E.skeletonMaxDistance);
  WriteColor(f, "esp_boxColor", E.boxColor);
  WriteColor(f, "esp_teamColor", E.teamColor);
  WriteColor(f, "esp_boneColor", E.boneColor);
  // Aimbot
  WriteBool(f, "aim_enabled", A.enabled);
  WriteInt(f, "aim_hotkey", A.hotkey);
  WriteInt(f, "aim_bone", A.targetBone);
  WriteFloat(f, "aim_fov", A.fov);
  WriteFloat(f, "aim_smooth", A.smooth);
  WriteFloat(f, "aim_jitter", A.jitter);
  WriteBool(f, "aim_teamCheck", A.teamCheck);
  WriteBool(f, "aim_onlyScoped", A.onlyScoped);
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
  // Bomb
  WriteBool(f, "bomb_enabled", B.enabled);
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

  auto &E = Features::config;
  auto &A = Features::aimbotConfig;
  auto &T = Features::triggerbotConfig;
  auto &M = Features::miscConfig;
  auto &B = Features::bombConfig;

  // ESP
  E.enabled = ReadB(j, "esp_enabled", E.enabled);
  E.showBox = ReadB(j, "esp_showBox", E.showBox);
  E.showName = ReadB(j, "esp_showName", E.showName);
  E.showHealth = ReadB(j, "esp_showHealth", E.showHealth);
  E.showWeapon = ReadB(j, "esp_showWeapon", E.showWeapon);
  E.showDistance = ReadB(j, "esp_showDistance", E.showDistance);
  E.showTeammates = ReadB(j, "esp_showTeammates", E.showTeammates);
  E.showBones = ReadB(j, "esp_showBones", E.showBones);
  E.skeletonMaxDistance =
      ReadF(j, "esp_skeletonMaxDist", E.skeletonMaxDistance);
  ReadColor(j, "esp_boxColor", E.boxColor);
  ReadColor(j, "esp_teamColor", E.teamColor);
  ReadColor(j, "esp_boneColor", E.boneColor);
  // Aimbot
  A.enabled = ReadB(j, "aim_enabled", A.enabled);
  A.hotkey = ReadI(j, "aim_hotkey", A.hotkey);
  A.targetBone = ReadI(j, "aim_bone", A.targetBone);
  A.fov = ReadF(j, "aim_fov", A.fov);
  A.smooth = ReadF(j, "aim_smooth", A.smooth);
  A.jitter = ReadF(j, "aim_jitter", A.jitter);
  A.teamCheck = ReadB(j, "aim_teamCheck", A.teamCheck);
  A.onlyScoped = ReadB(j, "aim_onlyScoped", A.onlyScoped);
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
  // Bomb
  B.enabled = ReadB(j, "bomb_enabled", B.enabled);

  std::cout << "[CONFIG] Loaded: " << name << "\n";
  return true;
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
