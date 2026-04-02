#pragma once
#include <string>

namespace Utils {
class Logger {
public:
  static void Info(const std::string &message);
  static void Warn(const std::string &message);
  static void Error(const std::string &message);
};
} // namespace Utils
