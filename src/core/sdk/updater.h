#pragma once
#include <future>

namespace SDK {
class Updater {
public:
  static std::future<bool> UpdateOffsets();
  static std::future<bool> ForceUpdateOffsets();
};
} // namespace SDK
