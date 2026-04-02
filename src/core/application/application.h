#pragma once
#include "app_state.h"
#include "app_config.h"
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
    AppConfig config_;
    std::thread memoryThread_;
    bool insertPressed_ = false;
    std::atomic<float> overlayFrameTimeMs_{4.17f};
};

} // namespace Core
