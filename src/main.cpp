#include "config/config_manager.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/process/module.h"
#include "core/process/process.h"
#include "core/process/stealth.h"
#include "features/feature_manager.h"
#include "input/input_manager.h"
#include "render/draw/draw_list.h"
#include "render/menu/menu.h"
#include "render/overlay/overlay.h"
#include "render/renderer/imgui_manager.h"
#include "render/renderer/renderer.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "core/sdk/updater.h"
#include "license/license.h"

int main() {
  if (!License::validate()) {
      return 0;
  }
  // ── Anti-detection: spoof PEB process name before anything else ──
  Core::Stealth::Apply();
  std::cout << "[+] Stealth module applied (PEB spoofed)." << std::endl;

  std::cout << "[+] Updating Offsets..." << std::endl;
  SDK::Updater::UpdateOffsets();

  std::cout << "[+] Attempting to attach to cs2.exe..." << std::endl;
  Core::Process::Attach(L"cs2.exe");
  if (Core::Process::GetProcessId() != 0) {
    std::cout << "[+] Hooked cs2.exe (PID: " << Core::Process::GetProcessId() << ")." << std::endl;
  }

  // Silent overlay creation — read actual desktop resolution
  const int screenW = GetSystemMetrics(SM_CXSCREEN);
  const int screenH = GetSystemMetrics(SM_CYSCREEN);
  std::cout << "[+] Creating hardware overlay (" << screenW << "x" << screenH << ")..." << std::endl;
  if (!Render::Overlay::Create(screenW, screenH)) {
    return 1;
  }

  // Silent renderer init
  if (!Render::Renderer::Init(Render::Overlay::GetWindowHandle())) {
    return 1;
  }

  Render::ImGuiManager::Init();
  Features::FeatureManager::RegisterAll();
  Config::ConfigManager::Load("default.json");
  std::cout << "[+] Features initialized & config loaded." << std::endl;
  std::cout << "[!] Engine running. Press [INSERT] to toggle Menu." << std::endl;

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
          std::cout << "[!] Waiting for cs2.exe..." << std::endl;
          Core::Process::Attach(L"cs2.exe");
          if (Core::Process::GetProcessId() != 0) {
            std::cout << "[+] Hooked cs2.exe (PID: " << Core::Process::GetProcessId() << ")." << std::endl;
          }
          lastAttach = std::chrono::steady_clock::now();
        }
      }

      Core::GameManager::Update();

      int ups = Config::Settings.performance.upsLimit;
      if (ups <= 0)
        ups = 64; // Fallback

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
    bool readBones =
        Config::Settings.esp.showBones || Config::Settings.aimbot.enabled;
    bool readWeapons = Config::Settings.esp.showWeapon;
    Core::GameManager::EnableBoneRead(readBones);
    Core::GameManager::EnableWeaponRead(readWeapons);

    // UpdateAll содержит GetAsyncKeyState/SendInput — только из render-треда
    Features::FeatureManager::UpdateAll();

    Render::Renderer::BeginFrame();
    Render::ImGuiManager::NewFrame();

    Render::Menu::Render();

    Render::DrawList drawList;
    Features::FeatureManager::RenderAll(drawList);

    Render::ImGuiManager::Render();
    Render::Renderer::EndFrame();

    int fps = Config::Settings.performance.fpsLimit;
    if (fps <= 0)
      fps = 144; // Fallback

    auto frameTimeTarget = std::chrono::milliseconds(1000 / fps);
    auto timeTaken = std::chrono::steady_clock::now() - frameStart;
    if (timeTaken < frameTimeTarget) {
      std::this_thread::sleep_for(frameTimeTarget - timeTaken);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
