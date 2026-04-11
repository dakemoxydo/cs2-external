#pragma once
#include "offset_loader.h"
#include <atomic>
#include <memory>

namespace SDK {

struct OffsetUpdateJobState;

class OffsetUpdateJob {
public:
  OffsetUpdateJob() = default;
  explicit OffsetUpdateJob(std::shared_ptr<OffsetUpdateJobState> state);

  bool IsValid() const;
  bool IsReady() const;
  bool Succeeded() const;

private:
  std::shared_ptr<OffsetUpdateJobState> state_;
};

class Updater {
public:
  static OffsetUpdateJob UpdateOffsets();
  static OffsetUpdateJob ForceUpdateOffsets();
  static OffsetUpdateJob ReloadOffsets();
};

} // namespace SDK
