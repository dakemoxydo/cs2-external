#pragma once
#include "app_state.h"
#include <thread>
#include <atomic>

namespace Core {

class Application {
public:
    Application();
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

    AppState& GetState() { return state_; }
    const AppState& GetState() const { return state_; }

private:
    void MemoryThreadLoop();
    void RenderLoop();
    void ProcessInput();
    void CheckOffsetsUpdate();

    AppState state_;
    std::thread memoryThread_;
    std::atomic<float> overlayFrameTimeMs_{4.17f};
    std::atomic<bool> shutdownCalled_{false};
    bool vsyncEnabled_{false};
    bool vsyncInitialized_{false};
};

} // namespace Core
