#pragma once
#include <atomic>

namespace Core {

struct AppState {
    std::atomic<bool> running{true};
    std::atomic<bool> menuOpen{false};
    std::atomic<bool> shouldClose{false};
};

} // namespace Core
