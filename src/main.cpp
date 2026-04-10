#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "config/config_manager.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/process/module.h"
#include "core/process/process.h"
#include "core/process/stealth.h"
#include "core/sdk/offsets.h"
#include "features/feature_manager.h"
#include "input/input_manager.h"
#include "render/draw/draw_list.h"
#include "render/menu/menu.h"
#include "render/overlay/overlay.h"
#include "render/renderer/imgui_manager.h"
#include "render/renderer/renderer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <shared_mutex>
#include <thread>

#include "core/sdk/updater.h"

// Shared frame time for VSync UPS sync
static std::atomic<float> g_overlayFrameTimeMs{4.17f};

// Track VSync state to avoid redundant SetVSync calls every frame
static bool s_lastVSyncState = false;
static bool s_vsyncInitialized = false;

// Helper for debug output
static void DebugLog(const char* msg) {
    std::cout << msg;
}

int main() {

    // ── Anti-detection: spoof PEB process name before anything else ──
    Core::Stealth::Apply();
    DebugLog("[+] Stealth module applied (PEB spoofed).\n");

    DebugLog("[+] Updating Offsets...\n");
    auto offsetsFuture = SDK::Updater::UpdateOffsets();
    offsetsFuture.wait();

    DebugLog("[+] Attempting to attach to cs2.exe...\n");
    Core::Process::Attach(L"cs2.exe");
    if (Core::Process::GetProcessId() != 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[+] Hooked cs2.exe (PID: %lu).\n", Core::Process::GetProcessId());
        DebugLog(buf);
    }

    // Silent overlay creation — auto-detects CS2 window size and position
    DebugLog("[+] Creating hardware overlay (auto-detect CS2 window)...\n");
    if (!Render::Overlay::Create()) {
        DebugLog("[-] Overlay creation failed!\n");
        return 1;
    }

    // Silent renderer init
    if (!Render::Renderer::Init(Render::Overlay::GetWindowHandle())) {
        DebugLog("[-] Renderer init failed!\n");
        return 1;
    }

    Render::ImGuiManager::Init();
    Features::FeatureManager::RegisterAll();
    Config::ConfigManager::Load("default");
    DebugLog("[+] Features initialized & config loaded.\n");
    DebugLog("[!] Engine running. Press [INSERT] to toggle Menu.\n");

    std::atomic<bool> isRunning{true};
    bool insertPressed = false;

    // ─── Backend Memory Thread ──────────────────────────────
    std::thread memoryThread([&isRunning]() {
        while (isRunning) {
            auto frameStart = std::chrono::steady_clock::now();

            // Auto-attach logic if cs2.exe wasn't open on launch or crashed
            if (Core::Process::GetProcessId() == 0) {
                static auto lastAttach = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - lastAttach)
                        .count() >= 5) {
                    DebugLog("[!] Waiting for cs2.exe...\n");
                    Core::Process::Attach(L"cs2.exe");
                    if (Core::Process::GetProcessId() != 0) {
                        char buf[256];
                        snprintf(buf, sizeof(buf), "[+] Hooked cs2.exe (PID: %lu).\n", Core::Process::GetProcessId());
                        DebugLog(buf);
                    }
                    lastAttach = std::chrono::steady_clock::now();
                }
            }

            Core::GameManager::Update();

            // ── UPS: fixed or VSync-synced ──
            int ups;
            {
                std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
                if (Config::Settings.performance.vsyncEnabled) {
                    // Sync UPS with overlay FPS (measured via frame timing)
                    float frameTimeMs = g_overlayFrameTimeMs.load();
                    ups = frameTimeMs > 0 ? std::min(256, (int)(1000.0f / frameTimeMs)) : 64;
                } else {
                    ups = Config::Settings.performance.upsLimit;
                    if (ups <= 0) ups = 240;
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
    });

    // ─── Frontend Render Loop ──────────────────────────────
    while (isRunning) {
        auto frameStart = std::chrono::steady_clock::now();

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                isRunning = false;
            }
        }

        if (!isRunning)
            break;
        if ((GetAsyncKeyState(VK_END) & 0x8000) || Render::Menu::ShouldClose()) {
            isRunning = false;
            break;
        }

        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (!insertPressed) {
                Render::Menu::Toggle();
                insertPressed = true;
            }
        } else {
            insertPressed = false;
        }

        Input::InputManager::Poll();

        // ── Update GameManager Read Flags (Thread-safe) ──
        bool readBones, readWeapons;
        {
            std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
            readBones =
                Config::Settings.esp.showBones || Config::Settings.aimbot.enabled;
            readWeapons = Config::Settings.esp.showWeapon;
        }
        Core::GameManager::EnableBoneRead(readBones);
        Core::GameManager::EnableWeaponRead(readWeapons);

        // ── Update frustum culling screen size ──
        Core::GameManager::SetScreenSize(Render::Overlay::GetGameWidth(), Render::Overlay::GetGameHeight());

        // UpdateAll содержит GetAsyncKeyState/SendInput — только из render-треда
        Features::FeatureManager::UpdateAll();

        // Update overlay position if CS2 window moved/resized
        Render::Overlay::UpdatePosition();

        Render::Renderer::BeginFrame();
        Render::ImGuiManager::NewFrame();

        Render::Menu::Render();

        Render::DrawList drawList;
        Features::FeatureManager::RenderAll(drawList);

        Render::ImGuiManager::Render();
        Render::Renderer::EndFrame();

        // ── Frame timing for VSync UPS sync ──
        auto frameEnd = std::chrono::steady_clock::now();
        float frameTimeMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        g_overlayFrameTimeMs.store(frameTimeMs);

        // Apply VSync setting only when it changes
        {
            std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
            bool vsyncEnabled = Config::Settings.performance.vsyncEnabled;

            if (!s_vsyncInitialized || vsyncEnabled != s_lastVSyncState) {
                Render::Renderer::SetVSync(vsyncEnabled);
                s_lastVSyncState = vsyncEnabled;
                s_vsyncInitialized = true;
            }

            if (!vsyncEnabled) {
                int fps = Config::Settings.performance.fpsLimit;
                if (fps <= 0)
                    fps = 240; // Fallback

                auto frameTimeTarget = std::chrono::milliseconds(1000 / fps);
                auto timeTaken = std::chrono::steady_clock::now() - frameStart;
                if (timeTaken < frameTimeTarget) {
                    std::this_thread::sleep_for(frameTimeTarget - timeTaken);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    // ── Wait for backend thread to finish ──
    if (memoryThread.joinable()) {
        memoryThread.join();
    }

    Render::ImGuiManager::Shutdown();
    Render::Renderer::Shutdown();
    Render::Overlay::Destroy();
    Core::Process::Detach();

    return 0;
}
