#include "imgui_manager.h"
#include "../overlay/overlay.h"
#include "renderer.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>


namespace Render {
bool ImGuiManager::Init() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Настраиваем шрифт — увеличенный размер для лучшей читаемости
  ImFontConfig fontConfig;
  fontConfig.SizePixels = 15.0f;
  io.Fonts->AddFontDefault(&fontConfig);

  // Современная тёмная тема как база
  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  // Базовые скругления и отступы
  style.WindowRounding = 16.0f;
  style.ChildRounding = 12.0f;
  style.FrameRounding = 8.0f;
  style.PopupRounding = 12.0f;
  style.ScrollbarRounding = 8.0f;
  style.GrabRounding = 8.0f;
  style.TabRounding = 8.0f;

  style.WindowBorderSize = 0.0f;
  style.ChildBorderSize = 0.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 0.0f;

  style.FramePadding = ImVec2(10.0f, 8.0f);
  style.ItemSpacing = ImVec2(12.0f, 12.0f);
  style.ItemInnerSpacing = ImVec2(10.0f, 8.0f);
  style.IndentSpacing = 16.0f;
  style.ScrollbarSize = 8.0f;

  // Улучшенная цветовая схема (Midnight-подобная)
  colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.06f, 0.98f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.96f);
  colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.14f, 0.50f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.80f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.18f, 0.90f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.35f, 0.30f, 0.75f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.42f, 0.37f, 0.82f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.24f, 0.65f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.35f, 0.30f, 0.75f, 0.35f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.42f, 0.37f, 0.82f, 0.45f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.42f, 0.37f, 0.82f, 0.65f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.40f, 0.85f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.50f, 0.95f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.40f, 0.85f, 1.00f);
  colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.17f, 0.60f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.30f, 0.75f, 0.70f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.80f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.22f, 0.80f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.35f, 0.90f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.30f, 0.75f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.12f, 0.80f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.30f, 0.75f, 0.60f);
  colors[ImGuiCol_TabActive] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.45f, 0.40f, 0.85f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.55f, 0.50f, 0.95f, 1.00f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.40f, 0.85f, 0.80f);

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
