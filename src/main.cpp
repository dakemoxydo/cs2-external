#include "config/config_manager.h"
#include "core/game/game_manager.h"
#include "core/process/module.h"
#include "core/process/process.h"
#include "features/feature_manager.h"
#include "input/input_manager.h"
#include "render/draw/draw_list.h"
#include "render/menu/menu.h"
#include "render/overlay/overlay.h"
#include "render/renderer/imgui_manager.h"
#include "render/renderer/renderer.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "core/sdk/updater.h"

int main() {
  std::cout << "========================================\n";
  std::cout << "[INFO] Initializing CS2 External...\n";
  std::cout << "========================================\n";

  if (!SDK::Updater::UpdateOffsets()) {
    std::cout << "[WARNING] Offset update failed. Fallback to default/0.\n";
  }

  std::cout << "[DEBUG] Attaching to cs2.exe..." << std::endl;
  if (!Core::Process::Attach(L"cs2.exe")) {
    std::cerr << "[ERROR] Could not attach to cs2.exe. Waiting for game..."
              << std::endl;
  } else {
    std::cout << "[DEBUG] Attached successfully! PID: "
              << Core::Process::GetProcessId() << std::endl;
  }

  std::cout << "[DEBUG] Creating Overlay..." << std::endl;
  if (!Render::Overlay::Create(1920, 1080)) {
    std::cerr << "[ERROR] Could not create overlay" << std::endl;
    return 1;
  }

  std::cout << "[DEBUG] Initializing Renderer..." << std::endl;
  if (!Render::Renderer::Init(Render::Overlay::GetWindowHandle())) {
    std::cerr << "[ERROR] Could not init renderer" << std::endl;
    return 1;
  }

  std::cout << "[DEBUG] Initializing ImGui..." << std::endl;
  Render::ImGuiManager::Init();

  std::cout << "[DEBUG] Registering Features..." << std::endl;
  Features::FeatureManager::RegisterAll();

  std::cout << "[DEBUG] Loading Settings..." << std::endl;
  Config::ConfigManager::Load("default.json");

  std::cout << "========================================" << std::endl;
  std::cout << "[INFO] Setup complete. Cheat is RUNNING." << std::endl;
  std::cout << "[INFO] Press [INSERT] to toggle Menu." << std::endl;
  std::cout << "[INFO] Press [END] to Panic/Unload." << std::endl;
  std::cout << "========================================" << std::endl;

  bool isRunning = true;
  bool insertPressed = false;

  while (isRunning) {
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
      std::cout << "[INFO] Unload signal received. Shutting down..."
                << std::endl;
      isRunning = false;
      break;
    }

    if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
      if (!insertPressed) {
        Render::Menu::Toggle();
        std::cout << "[DEBUG] Menu toggled: "
                  << (Render::Menu::IsOpen() ? "OPEN" : "CLOSED") << std::endl;
        insertPressed = true;
      }
    } else {
      insertPressed = false;
    }

    Input::InputManager::Poll();

    // Auto-attach logic if cs2.exe wasn't open on launch or crashed
    if (Core::Process::GetProcessId() == 0) {
      static auto lastAttach = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now() - lastAttach)
              .count() >= 5) {
        std::cout << "[DEBUG] Attempting to attach to cs2.exe..." << std::endl;
        if (Core::Process::Attach(L"cs2.exe")) {
          std::cout << "[DEBUG] Attached successfully! PID: "
                    << Core::Process::GetProcessId() << std::endl;
        } else {
          std::cout << "[DEBUG] Waiting for game..." << std::endl;
        }
        lastAttach = std::chrono::steady_clock::now();
      }
    } else {
      static auto lastMainDebug = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now() - lastMainDebug)
              .count() >= 5) {
        uintptr_t clientBase = Core::GameManager::GetClientBase();
        auto plrs = Core::GameManager::GetPlayers();
        std::cout << "[DEBUG-MAIN] Process: ATTACHED | client.dll: 0x"
                  << std::hex << clientBase << std::dec
                  << " | Entities: " << plrs.size() << std::endl;
        lastMainDebug = std::chrono::steady_clock::now();
      }
    }

    Core::GameManager::Update();
    Features::FeatureManager::UpdateAll();

    Render::Renderer::BeginFrame();
    Render::ImGuiManager::NewFrame();

    Render::Menu::Render();

    Render::DrawList drawList;
    Features::FeatureManager::RenderAll(drawList);

    Render::ImGuiManager::Render();
    Render::Renderer::EndFrame();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  std::cout << "[DEBUG] Shutting down systems..." << std::endl;
  Render::ImGuiManager::Shutdown();
  Render::Renderer::Shutdown();
  Render::Overlay::Destroy();
  Core::Process::Detach();

  std::cout << "[INFO] Unloaded successfully." << std::endl;

  return 0;
}
