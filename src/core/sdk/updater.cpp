#include "updater.h"
#include <mutex>
#include <thread>

namespace SDK {

struct OffsetUpdateJobState {
  std::atomic<bool> ready{false};
  std::atomic<bool> success{false};
};

namespace {

std::mutex g_loaderMutex;

template <typename Fn>
OffsetUpdateJob StartJob(Fn &&fn) {
  auto state = std::make_shared<OffsetUpdateJobState>();
  std::thread([state, task = std::forward<Fn>(fn)]() mutable {
    bool success = false;
    {
      std::lock_guard<std::mutex> lock(g_loaderMutex);
      try {
        static OffsetLoader loader;
        success = task(loader);
      } catch (...) {
        success = false;
      }
    }
    state->success.store(success, std::memory_order_release);
    state->ready.store(true, std::memory_order_release);
  }).detach();
  return OffsetUpdateJob(state);
}

} // namespace

OffsetUpdateJob::OffsetUpdateJob(std::shared_ptr<OffsetUpdateJobState> state)
    : state_(std::move(state)) {}

bool OffsetUpdateJob::IsValid() const {
  return static_cast<bool>(state_);
}

bool OffsetUpdateJob::IsReady() const {
  return state_ && state_->ready.load(std::memory_order_acquire);
}

bool OffsetUpdateJob::Succeeded() const {
  return state_ && state_->success.load(std::memory_order_acquire);
}

OffsetUpdateJob Updater::UpdateOffsets() {
  return StartJob([](OffsetLoader &loader) { return loader.LoadOffsets(); });
}

OffsetUpdateJob Updater::ForceUpdateOffsets() {
  return StartJob(
      [](OffsetLoader &loader) { return loader.ForceUpdateFromGitHub(); });
}

OffsetUpdateJob Updater::ReloadOffsets() {
  return StartJob([](OffsetLoader &loader) { return loader.ReloadOffsets(); });
}

} // namespace SDK
