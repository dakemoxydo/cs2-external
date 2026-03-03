#pragma once
#include <string>

namespace Config {
class ConfigManager {
public:
  static bool Load(const std::string &filename);
  static bool Save(const std::string &filename);
  static void LoadDefault();
};
} // namespace Config
