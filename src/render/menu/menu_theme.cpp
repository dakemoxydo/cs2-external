#include "menu_theme.h"
#include <imgui.h>

namespace Render {

namespace {

ImVec4 AccentForTheme(int theme) {
  switch (theme) {
  case 1:
    return ImVec4(0.95f, 0.40f, 0.40f, 1.0f);
  case 2:
    return ImVec4(0.00f, 0.83f, 1.00f, 1.0f);
  case 3:
    return ImVec4(0.62f, 0.66f, 1.00f, 1.0f);
  case 4:
    return ImVec4(0.93f, 0.73f, 0.25f, 1.0f);
  case 5:
    return ImVec4(0.78f, 0.82f, 0.90f, 1.0f);
  case 6:
    return ImVec4(0.45f, 0.95f, 0.55f, 1.0f);
  default:
    return ImVec4(0.29f, 0.62f, 1.00f, 1.0f);
  }
}

} // namespace

void ApplyMenuTheme(int theme) {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;
  const ImVec4 accent = AccentForTheme(theme);
  const ImVec4 accentSoft = ImVec4(accent.x, accent.y, accent.z, 0.25f);
  const ImVec4 accentMid = ImVec4(accent.x, accent.y, accent.z, 0.55f);

  style.WindowPadding = ImVec2(18.0f, 18.0f);
  style.FramePadding = ImVec2(10.0f, 8.0f);
  style.CellPadding = ImVec2(8.0f, 8.0f);
  style.ItemSpacing = ImVec2(12.0f, 12.0f);
  style.ItemInnerSpacing = ImVec2(8.0f, 8.0f);
  style.ScrollbarSize = 10.0f;
  style.GrabMinSize = 24.0f;

  style.WindowRounding = 10.0f;
  style.ChildRounding = 10.0f;
  style.PopupRounding = 10.0f;
  style.FrameRounding = 8.0f;
  style.ScrollbarRounding = 8.0f;
  style.GrabRounding = 999.0f;
  style.TabRounding = 8.0f;

  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;

  colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.54f, 0.56f, 0.60f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.09f, 0.98f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.14f, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.13f, 0.15f, 0.19f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);

  colors[ImGuiCol_Button] = accentSoft;
  colors[ImGuiCol_ButtonHovered] = accentMid;
  colors[ImGuiCol_ButtonActive] = accent;
  colors[ImGuiCol_Header] = accentSoft;
  colors[ImGuiCol_HeaderHovered] = accentMid;
  colors[ImGuiCol_HeaderActive] = accent;
  colors[ImGuiCol_CheckMark] = accent;
  colors[ImGuiCol_SliderGrab] = accentMid;
  colors[ImGuiCol_SliderGrabActive] = accent;
  colors[ImGuiCol_Separator] = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = accentMid;
  colors[ImGuiCol_SeparatorActive] = accent;
  colors[ImGuiCol_ResizeGrip] = accentSoft;
  colors[ImGuiCol_ResizeGripHovered] = accentMid;
  colors[ImGuiCol_ResizeGripActive] = accent;
  colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
  colors[ImGuiCol_TabHovered] = accentSoft;
  colors[ImGuiCol_TabActive] = ImVec4(0.13f, 0.15f, 0.19f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = accent;
  colors[ImGuiCol_PlotHistogramHovered] = accent;
  colors[ImGuiCol_NavHighlight] = accentMid;

  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = accentMid;
}

} // namespace Render
