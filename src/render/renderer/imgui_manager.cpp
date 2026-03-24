#include "imgui_manager.h"
#include "../overlay/overlay.h"
#include "renderer.h"
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <iostream>


namespace Render {
bool ImGuiManager::Init() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();

  if (!ImGui_ImplWin32_Init(Overlay::GetWindowHandle()))
    return false;
  if (!ImGui_ImplDX11_Init(Renderer::GetDevice(), Renderer::GetContext()))
    return false;

  return true;
}

void ImGuiManager::Shutdown() {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiManager::NewFrame() {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void ImGuiManager::Render() {
  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
} // namespace Render
