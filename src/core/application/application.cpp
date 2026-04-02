#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "application.h"
#include "config/config_manager.h"
#include "config/settings.h"
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
#include <iostream>

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

    // Offsets
    std::cout << "[App] Updating offsets...\n";
    Utils::Logger::Info("Updating offsets...");
    auto offsetsFuture = SDK::Updater::UpdateOffsets();
    offsetsFuture.wait();
    std::cout << "[App] Offsets updated\n";

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
        return false;
    }
    std::cout << "[App] Overlay created\n";

    // Renderer
    std::cout << "[App] Initializing renderer...\n";
    if (!Render::Renderer::Init(Render::Overlay::GetWindowHandle())) {
        Utils::Logger::Error("Failed to initialize renderer");
        std::cout << "[App] Renderer init FAILED\n";
        return false;
    }
    std::cout << "[App] Renderer initialized\n";

    std::cout << "[App] Initializing ImGuiManager...\n";
    Render::ImGuiManager::Init();
    std::cout << "[App] Registering features...\n";
    Features::FeatureManager::RegisterAll();
    std::cout << "[App] Loading config...\n";
    Config::ConfigManager::Load("default.json");

    Utils::Logger::Info("Features initialized & config loaded");
    Utils::Logger::Info("Engine running. Press [INSERT] to toggle Menu");
    std::cout << "[App] Initialize() SUCCESS\n";

    return true;
}

void Application::Run() {
    // Запускаем Memory Thread
    memoryThread_ = std::thread(&Application::MemoryThreadLoop, this);

    // Render Loop в основном потоке
    RenderLoop();

    // Ждём завершения Memory Thread
    if (memoryThread_.joinable()) {
        memoryThread_.join();
    }
}

void Application::Shutdown() {
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
    while (state_.running) {
        auto frameStart = std::chrono::steady_clock::now();

        // Auto-attach logic
        if (Process::GetProcessId() == 0) {
            static auto lastAttach = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - lastAttach).count() >= 5) {
                Utils::Logger::Info("Waiting for cs2.exe...");
                Process::Attach(L"cs2.exe");
                if (Process::GetProcessId() != 0) {
                    Utils::Logger::Info("Hooked cs2.exe (PID: %d)", Process::GetProcessId());
                }
                lastAttach = std::chrono::steady_clock::now();
            }
        }

        GameManager::Update();

        // UPS limiting
        int ups;
        if (Config::Settings.performance.vsyncEnabled) {
            // Sync UPS with overlay FPS (measured via frame timing)
            float frameTimeMS = overlayFrameTimeMs_.load();
            ups = frameTimeMS > 0 ? std::min(256, (int)(1000.0f / frameTimeMS)) : 64;
        } else {
            ups = Config::Settings.performance.upsLimit;
            if (ups <= 0) ups = 240;
        }

        auto frameTimeTarget = std::chrono::milliseconds(1000 / ups);
        auto timeTaken = std::chrono::steady_clock::now() - frameStart;
        if (timeTaken < frameTimeTarget) {
            std::this_thread::sleep_for(frameTimeTarget - timeTaken);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void Application::RenderLoop() {
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
        if ((GetAsyncKeyState(VK_END) & 0x8000) || Render::Menu::ShouldClose()) {
            state_.running = false;
            break;
        }

        ProcessInput();

        // Update GameManager Read Flags
        bool readBones = Config::Settings.esp.showBones || Config::Settings.aimbot.enabled;
        bool readWeapons = Config::Settings.esp.showWeapon;
        GameManager::EnableBoneRead(readBones);
        GameManager::EnableWeaponRead(readWeapons);

        // UpdateAll — только из render thread
        Features::FeatureManager::UpdateAll();

        // Update overlay position
        Render::Overlay::UpdatePosition();

        // Render
        Render::Renderer::BeginFrame();
        Render::ImGuiManager::NewFrame();
        Render::Menu::Render();

        Render::DrawList drawList;
        Features::FeatureManager::RenderAll(drawList);

        Render::ImGuiManager::Render();
        Render::Renderer::EndFrame();

        // Frame timing for VSync UPS sync
        auto frameEnd = std::chrono::steady_clock::now();
        float frameTimeMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        overlayFrameTimeMs_.store(frameTimeMs);

        // FPS limiting
        Render::Renderer::SetVSync(Config::Settings.performance.vsyncEnabled);

        if (!Config::Settings.performance.vsyncEnabled) {
            int fps = Config::Settings.performance.fpsLimit;
            if (fps <= 0) fps = 240;

            auto frameTimeTarget = std::chrono::milliseconds(1000 / fps);
            auto timeTaken = std::chrono::steady_clock::now() - frameStart;
            if (timeTaken < frameTimeTarget) {
                std::this_thread::sleep_for(frameTimeTarget - timeTaken);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    Shutdown();
}

void Application::ProcessInput() {
    Input::InputManager::Poll();

    if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
        if (!insertPressed_) {
            Render::Menu::Toggle();
            state_.menuOpen = Render::Menu::IsOpen();
            insertPressed_ = true;
        }
    } else {
        insertPressed_ = false;
    }
}

void Application::CheckOffsetsUpdate() {
    // Placeholder for future offset auto-update logic
}

} // namespace Core
