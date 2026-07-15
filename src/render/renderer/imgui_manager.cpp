#include "imgui_manager.h"
#include "../overlay/overlay.h"
#include "renderer.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include <imgui.h>

namespace Render {
namespace {
bool s_initialized = false;
bool s_contextCreated = false;
bool s_win32Initialized = false;
bool s_dx11Initialized = false;
ImFont *s_regularFont = nullptr;
ImFont *s_semiboldFont = nullptr;

constexpr const char *kSegoeRegular = "C:\\Windows\\Fonts\\segoeui.ttf";
constexpr const char *kSegoeSemibold = "C:\\Windows\\Fonts\\segoeuib.ttf";
} // namespace

bool ImGuiManager::Init() {
  if (ImGui::GetCurrentContext()) {
    Shutdown();
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  s_contextCreated = true;
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImFontConfig regularConfig;
  regularConfig.FontDataOwnedByAtlas = false;
  s_regularFont = io.Fonts->AddFontFromFileTTF(kSegoeRegular, 16.0f, &regularConfig);

  ImFontConfig semiboldConfig;
  semiboldConfig.FontDataOwnedByAtlas = false;
  s_semiboldFont = io.Fonts->AddFontFromFileTTF(kSegoeSemibold, 16.0f, &semiboldConfig);

  if (!s_regularFont) {
    ImFontConfig fallbackConfig;
    fallbackConfig.SizePixels = 16.0f;
    s_regularFont = io.Fonts->AddFontDefault(&fallbackConfig);
  }
  if (!s_semiboldFont) {
    s_semiboldFont = s_regularFont;
  }
  io.FontDefault = s_regularFont;

  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

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

  if (!ImGui_ImplWin32_Init(Overlay::GetWindowHandle())) {
    Shutdown();
    return false;
  }
  s_win32Initialized = true;
  if (!ImGui_ImplDX11_Init(Renderer::GetDevice(), Renderer::GetContext())) {
    Shutdown();
    return false;
  }
  s_dx11Initialized = true;

  s_initialized = true;
  return true;
}

void ImGuiManager::Shutdown() {
  if (!s_contextCreated && !s_win32Initialized && !s_dx11Initialized &&
      !s_initialized) {
    return;
  }

  if (s_dx11Initialized) ImGui_ImplDX11_Shutdown();
  if (s_win32Initialized) ImGui_ImplWin32_Shutdown();
  if (s_contextCreated && ImGui::GetCurrentContext()) {
    ImGui::DestroyContext();
  }
  s_regularFont = nullptr;
  s_semiboldFont = nullptr;
  s_dx11Initialized = false;
  s_win32Initialized = false;
  s_contextCreated = false;
  s_initialized = false;
}

void ImGuiManager::NewFrame() {
  if (!s_initialized) {
    return;
  }
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void ImGuiManager::Render() {
  if (!s_initialized) {
    return;
  }
  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

ImFont *ImGuiManager::GetRegularFont() { return s_regularFont; }
ImFont *ImGuiManager::GetSemiboldFont() { return s_semiboldFont; }

} // namespace Render
