#pragma once
#include <string>
#include <vector>

namespace Config {

class ConfigManager {
public:
  /// Save current settings to configs/<name>.json
  static bool Save(const std::string &name);

  /// Load settings from configs/<name>.json
  static bool Load(const std::string &name);

  /// Apply current settings to features
  static void ApplySettings();

  /// List all available config files in the configs/ dir
  static std::vector<std::string> ListConfigs();

  /// Load built-in default settings
  static void LoadDefault();

  /// Last error description
  static std::string LastError;
};

} // namespace Config
