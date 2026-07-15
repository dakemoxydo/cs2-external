#pragma once
#include <string>
#include <vector>

namespace Config {

struct GlobalSettings;

class ConfigManager {
public:
  /// Save current settings to configs/<name>.json
  static bool Save(const std::string &name);

  /// Load settings from configs/<name>.json
  static bool Load(const std::string &name);

  /// Apply a published immutable settings snapshot to feature lifecycle.
  static void ApplySettings(const GlobalSettings& settings);

  /// List all available config files in the configs/ dir
  static std::vector<std::string> ListConfigs();

  /// Load built-in default settings
  static void LoadDefault();

  /// Last error description
  static std::string LastError;
};

} // namespace Config
