#include "updater.h"
#include <mutex>
#include <thread>
#include <vector>

namespace SDK {

struct OffsetUpdateJobState {
  std::atomic<bool> ready{false};
  std::atomic<bool> success{false};
};

namespace {

std::mutex g_loaderMutex;
std::mutex g_startMutex;
std::mutex g_jobsMutex;
struct ActiveJob {
  std::shared_ptr<OffsetUpdateJobState> state;
  std::jthread thread;
};
std::vector<ActiveJob> g_jobs;

template <typename Fn>
OffsetUpdateJob StartJob(Fn &&fn) {
  std::lock_guard<std::mutex> startLock(g_startMutex);
  {
    std::lock_guard<std::mutex> lock(g_jobsMutex);
    for (auto it = g_jobs.begin(); it != g_jobs.end();) {
      if (it->state->ready.load(std::memory_order_acquire)) {
        it = g_jobs.erase(it);
      } else {
        // Offset generations must be applied in order; coalesce duplicate
        // requests instead of accumulating blocked worker threads.
        return OffsetUpdateJob(it->state);
      }
    }
  }
  auto state = std::make_shared<OffsetUpdateJobState>();
  std::jthread worker([state, task = std::forward<Fn>(fn)]() mutable {
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
  });
  {
    std::lock_guard<std::mutex> lock(g_jobsMutex);
    g_jobs.push_back({state, std::move(worker)});
  }
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

void Updater::Shutdown() {
  std::vector<ActiveJob> jobs;
  {
    std::lock_guard<std::mutex> lock(g_jobsMutex);
    jobs.swap(g_jobs);
  }
  for (auto &job : jobs) {
    job.thread.request_stop();
  }
  // std::jthread joins as each element is destroyed. Network operations have
  // bounded WinINet timeouts, so teardown cannot wait indefinitely.
}

} // namespace SDK
