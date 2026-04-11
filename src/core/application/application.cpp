#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "application.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "core/constants.h"
#include "core/game/game_manager.h"
#include "core/process/process.h"
#include "core/process/stealth.h"
#include "core/sdk/offsets.h"
#include "core/sdk/updater.h"
#include "features/feature_manager.h"
#include "input/input_manager.h"
#include "render/draw/draw_list.h"
#include "render/menu/menu.h"
#include "render/overlay/overlay.h"
#include "render/renderer/imgui_manager.h"
#include "render/renderer/renderer.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <shared_mutex>

namespace Core {

Application::Application() = default;
Application::~Application() = default;

bool Application::Initialize() {
    std::cout << "[App] Initialize() entry\n";
    Utils::Logger::Init("cs2overlay.log");
    Utils::Logger::Info("Application starting...");

    // Stealth
    std::cout << "[App] Applying stealth...\n";
    Stealth::Apply();
    Utils::Logger::Info("Stealth module applied (PEB spoofed)");

    // Offsets — with timeout to avoid infinite blocking
    std::cout << "[App] Updating offsets...\n";
    Utils::Logger::Info("Updating offsets...");
    auto offsetJob = SDK::Updater::UpdateOffsets();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (offsetJob.IsValid() && !offsetJob.IsReady() &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    if (offsetJob.IsValid() && !offsetJob.IsReady()) {
        Utils::Logger::Warn("Offset loading timed out (15s), continuing anyway");
        std::cout << "[App] Offset loading timed out, continuing...\n";
    } else if (!offsetJob.Succeeded()) {
        Utils::Logger::Warn("Offset loading failed, continuing with cached/default data");
        std::cout << "[App] Offset loading failed, continuing...\n";
    } else {
        std::cout << "[App] Offsets updated\n";
    }

    // Process
    std::cout << "[App] Attaching to cs2.exe...\n";
    Utils::Logger::Info("Attempting to attach to cs2.exe...");
    Process::Attach(L"cs2.exe");
    if (Process::GetProcessId() != 0) {
        Utils::Logger::Info("Hooked cs2.exe (PID: %d)", Process::GetProcessId());
        std::cout << "[App] cs2.exe hooked\n";
    } else {
        std::cout << "[App] cs2.exe NOT found, will retry in memory thread\n";
    }

    // Overlay
    std::cout << "[App] Creating overlay...\n";
    Utils::Logger::Info("Creating hardware overlay...");
    if (!Render::Overlay::Create()) {
        Utils::Logger::Error("Failed to create overlay");
        std::cout << "[App] Overlay creation FAILED\n";
        Process::Detach();
        return false;
    }
    std::cout << "[App] Overlay created\n";

    // Renderer
    std::cout << "[App] Initializing renderer...\n";
    if (!Render::Renderer::Init(Render::Overlay::GetWindowHandle())) {
        Utils::Logger::Error("Failed to initialize renderer");
        std::cout << "[App] Renderer init FAILED\n";
        Render::Overlay::Destroy();
        Process::Detach();
        return false;
    }
    std::cout << "[App] Renderer initialized\n";

    std::cout << "[App] Initializing ImGuiManager...\n";
    if (!Render::ImGuiManager::Init()) {
        Utils::Logger::Error("Failed to initialize ImGuiManager");
        std::cout << "[App] ImGuiManager init FAILED\n";
        Render::Renderer::Shutdown();
        Render::Overlay::Destroy();
        Process::Detach();
        return false;
    }
    std::cout << "[App] ImGuiManager initialized\n";
    std::cout << "[App] Registering features...\n";
    Features::FeatureManager::RegisterAll();
    std::cout << "[App] Loading config...\n";
    Config::ConfigManager::Load("default");

    Utils::Logger::Info("Features initialized & config loaded");
    Utils::Logger::Info("Engine running. Press [INSERT] to toggle Menu");
    std::cout << "[App] Initialize() SUCCESS\n";

    return true;
}

void Application::Run() {
    memoryThread_ = std::thread(&Application::MemoryThreadLoop, this);
    RenderLoop();
    if (memoryThread_.joinable()) {
        memoryThread_.join();
    }
}

void Application::Shutdown() {
    // Guard against double shutdown
    bool expected = false;
    if (!shutdownCalled_.compare_exchange_strong(expected, true)) {
        return; // Already shutting down
    }

    Utils::Logger::Info("Shutting down...");
    state_.running = false;
    state_.shouldClose = true;

    Render::ImGuiManager::Shutdown();
    Render::Renderer::Shutdown();
    Render::Overlay::Destroy();
    Process::Detach();
    Utils::Logger::Shutdown();
}

void Application::MemoryThreadLoop() {
    try {
        while (state_.running) {
            auto frameStart = std::chrono::steady_clock::now();

            if (Process::GetProcessId() == 0) {
                static auto lastAttach = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - lastAttach).count() >= Constants::AUTO_ATTACH_INTERVAL_SEC) {
                    Utils::Logger::Info("Waiting for cs2.exe...");
                    Process::Attach(L"cs2.exe");
                    if (Process::GetProcessId() != 0) {
                        Utils::Logger::Info("Hooked cs2.exe (PID: %d)", Process::GetProcessId());
                    }
                    lastAttach = std::chrono::steady_clock::now();
                }
            }

            GameManager::Update();

            int ups;
            {
                std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
                if (Config::Settings.performance.vsyncEnabled) {
                    float frameTimeMS = overlayFrameTimeMs_.load();
                    ups = frameTimeMS > 0 ? std::min(256, (int)(1000.0f / frameTimeMS)) : 64;
                } else {
                    ups = Config::Settings.performance.upsLimit;
                    if (ups <= 0) ups = Constants::DEFAULT_UPS_LIMIT;
                }
            }

            auto frameTimeTarget = std::chrono::milliseconds(1000 / ups);
            auto timeTaken = std::chrono::steady_clock::now() - frameStart;
            if (timeTaken < frameTimeTarget) {
                std::this_thread::sleep_for(frameTimeTarget - timeTaken);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    } catch (const std::exception& e) {
        Utils::Logger::Error("MemoryThreadLoop exception: %s", e.what());
    } catch (...) {
        Utils::Logger::Error("MemoryThreadLoop unknown exception");
    }
}

void Application::RenderLoop() {
    try {
        while (state_.running) {
            auto frameStart = std::chrono::steady_clock::now();

            MSG msg;
            while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) {
                    state_.running = false;
                }
            }

            if (!state_.running) break;
            if (Input::InputManager::IsKeyDown(VK_END) || Render::Menu::ShouldClose()) {
                state_.running = false;
                break;
            }

            ProcessInput();

            // Read config settings under shared_lock to avoid data race with
            // MemoryThread and ConfigManager::Load/Save which write under unique_lock.
            {
                std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
                bool readBones = Config::Settings.esp.showBones || Config::Settings.aimbot.enabled;
                bool readWeapons = Config::Settings.esp.showWeapon;
                Core::GameManager::EnableBoneRead(readBones);
                Core::GameManager::EnableWeaponRead(readWeapons);
            }

            Features::FeatureManager::UpdateAll();

            // Update frustum culling screen size
            Core::GameManager::SetScreenSize(Render::Overlay::GetGameWidth(), Render::Overlay::GetGameHeight());

            // Update overlay position if CS2 window moved/resized
            if (Render::Overlay::UpdatePosition()) {
                Render::Renderer::HandleResize(Render::Overlay::GetGameWidth(),
                                              Render::Overlay::GetGameHeight());
            }

            Render::Renderer::BeginFrame();
            Render::ImGuiManager::NewFrame();
            Render::Menu::Render();

            Render::DrawList drawList;
            Features::FeatureManager::RenderAll(drawList);

            Render::ImGuiManager::Render();
            Render::Renderer::EndFrame();

            auto frameEnd = std::chrono::steady_clock::now();
            float frameTimeMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
            overlayFrameTimeMs_.store(frameTimeMs);

            // Read VSync/fps settings under shared_lock
            bool vsyncEnabled;
            int fpsLimit;
            {
                std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
                vsyncEnabled = Config::Settings.performance.vsyncEnabled;
                fpsLimit = Config::Settings.performance.fpsLimit;
            }

            // Apply VSync only when state changes
            if (!vsyncInitialized_ || vsyncEnabled != vsyncEnabled_) {
                Render::Renderer::SetVSync(vsyncEnabled);
                vsyncEnabled_ = vsyncEnabled;
                vsyncInitialized_ = true;
            }

            if (!vsyncEnabled) {
                int fps = fpsLimit;
                if (fps <= 0) fps = Constants::DEFAULT_FPS_LIMIT;

                auto frameTimeTarget = std::chrono::milliseconds(1000 / fps);
                auto timeTaken = std::chrono::steady_clock::now() - frameStart;
                if (timeTaken < frameTimeTarget) {
                    std::this_thread::sleep_for(frameTimeTarget - timeTaken);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    } catch (const std::exception& e) {
        Utils::Logger::Error("RenderLoop exception: %s", e.what());
    } catch (...) {
        Utils::Logger::Error("RenderLoop unknown exception");
    }

    Shutdown();
}

void Application::ProcessInput() {
    Input::InputManager::Poll();

    if (Input::InputManager::IsKeyPressed(VK_INSERT)) {
        Render::Menu::Toggle();
        state_.menuOpen = Render::Menu::IsOpen();
    }
}

void Application::CheckOffsetsUpdate() {
}

} // namespace Core
