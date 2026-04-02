#include "menu.h"
#include "../overlay/overlay.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "core/sdk/updater.h"
#include "features/skinchanger/skinchanger.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>
#include <future>

#include "ui_components.h"

// ─── Custom Widgets
static void ApplyTheme(int theme) {
  ImGuiStyle *style = &ImGui::GetStyle();
  ImVec4 *colors = style->Colors;

  // Улучшенная стилизация — более крупные скругления, отступы, современный вид
  style->WindowRounding = 16.0f;
  style->ChildRounding = 12.0f;
  style->FrameRounding = 8.0f;
  style->PopupRounding = 12.0f;
  style->ScrollbarRounding = 8.0f;
  style->GrabRounding = 8.0f;
  style->TabRounding = 8.0f;

  style->WindowBorderSize = 0.0f; // Убираем рамку для чистого вида
  style->ChildBorderSize = 0.0f;
  style->FrameBorderSize = 0.0f;
  style->PopupBorderSize = 0.0f;

  style->FramePadding = ImVec2(10.0f, 8.0f);
  style->ItemSpacing = ImVec2(12.0f, 12.0f);
  style->ItemInnerSpacing = ImVec2(10.0f, 8.0f);
  style->IndentSpacing = 16.0f;
  style->ScrollbarSize = 8.0f;

  if (theme == 0) { // Midnight — улучшенная
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
  } else if (theme == 1) { // Blood — улучшенная
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.04f, 0.04f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.08f, 0.08f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.08f, 0.08f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.10f, 0.10f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.65f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.75f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.65f, 0.15f, 0.15f, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.20f, 0.20f, 0.45f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.75f, 0.20f, 0.20f, 0.65f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.75f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.88f, 0.88f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.08f, 0.08f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.65f, 0.15f, 0.15f, 0.70f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.04f, 0.04f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.10f, 0.10f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.15f, 0.15f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.65f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.06f, 0.06f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.65f, 0.15f, 0.15f, 0.60f);
    colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.75f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.85f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.75f, 0.20f, 0.20f, 0.80f);
  } else if (theme == 2) { // Cyber — улучшенная
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.06f, 0.06f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.08f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.16f, 0.16f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.22f, 0.22f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.08f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.04f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.08f, 0.55f, 0.55f, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.65f, 0.65f, 0.45f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.65f, 0.65f, 0.65f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.15f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.25f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.15f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.88f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.08f, 0.20f, 0.20f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.08f, 0.55f, 0.55f, 0.70f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.06f, 0.06f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.10f, 0.18f, 0.18f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15f, 0.30f, 0.30f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.08f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.06f, 0.08f, 0.08f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.08f, 0.55f, 0.55f, 0.60f);
    colors[ImGuiCol_TabActive] = ImVec4(0.08f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.15f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.12f, 0.65f, 0.65f, 0.80f);
  } else if (theme == 3) { // Lavender — улучшенная
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.06f, 0.10f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.09f, 0.13f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.15f, 0.13f, 0.18f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.11f, 0.15f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.15f, 0.22f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.19f, 0.28f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.50f, 0.40f, 0.80f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.50f, 0.88f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.30f, 0.70f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.50f, 0.40f, 0.80f, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.50f, 0.88f, 0.45f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.50f, 0.88f, 0.65f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.50f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 0.60f, 0.95f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.50f, 0.90f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.94f, 0.92f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.42f, 0.50f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.13f, 0.18f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.50f, 0.40f, 0.80f, 0.70f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.06f, 0.10f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.16f, 0.22f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.25f, 0.35f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.40f, 0.80f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.07f, 0.11f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.50f, 0.40f, 0.80f, 0.60f);
    colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.50f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.60f, 0.95f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.50f, 0.88f, 0.80f);
  } else if (theme == 4) { // Gold/Luxury — улучшенная
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.14f, 0.08f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.13f, 0.11f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.16f, 0.13f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.70f, 0.55f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.65f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.45f, 0.10f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.70f, 0.55f, 0.15f, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.80f, 0.65f, 0.25f, 0.45f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.80f, 0.65f, 0.25f, 0.65f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.70f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.80f, 0.40f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.70f, 0.30f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.92f, 0.88f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.48f, 0.45f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.14f, 0.08f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.55f, 0.15f, 0.70f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.22f, 0.18f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.70f, 0.55f, 0.15f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.06f, 0.06f, 0.06f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.70f, 0.55f, 0.15f, 0.60f);
    colors[ImGuiCol_TabActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.80f, 0.70f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.90f, 0.80f, 0.40f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.80f, 0.65f, 0.25f, 0.80f);
  } else if (theme == 5) { // Monochrome — улучшенная
    colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.18f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.13f, 0.13f, 0.13f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.35f, 0.35f, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.55f, 0.55f, 0.55f, 0.45f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.55f, 0.55f, 0.65f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.18f, 0.18f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.55f, 0.55f, 0.55f, 0.70f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.04f, 0.04f, 0.04f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.55f, 0.55f, 0.55f, 0.60f);
    colors[ImGuiCol_TabActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.55f, 0.55f, 0.55f, 0.80f);
  } else if (theme == 6) { // Toxic — улучшенная
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.07f, 0.06f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.28f, 0.18f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.13f, 0.10f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.22f, 0.15f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.32f, 0.20f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.35f, 0.75f, 0.08f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.85f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.65f, 0.04f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.75f, 0.08f, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.85f, 0.15f, 0.45f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.85f, 0.15f, 0.65f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.85f, 0.15f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.95f, 0.25f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.85f, 0.15f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.50f, 0.45f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.28f, 0.18f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.75f, 0.08f, 0.70f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.12f, 0.18f, 0.12f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.20f, 0.30f, 0.20f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.75f, 0.08f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.06f, 0.05f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.75f, 0.08f, 0.60f);
    colors[ImGuiCol_TabActive] = ImVec4(0.08f, 0.10f, 0.08f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.45f, 0.85f, 0.15f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.55f, 0.95f, 0.25f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.85f, 0.15f, 0.80f);
  }
}

// Toggles moved to UI namespace

// ─── Config UI state
static char s_configName[64] = "default";
static std::vector<std::string> s_configList;
static int s_configSelected = 0;
static bool s_configDirty = false;
static int s_currentTab = 0; // 0=Legit, 1=Visuals, 2=Skins, 3=Misc, 4=Settings

// ─── Offset update state
static bool s_offsetUpdatePending = false;
static bool s_offsetUpdateSuccess = false;
static std::string s_offsetUpdateMessage = "";
static std::future<bool> s_offsetUpdateFuture;

namespace Render {
bool Menu::isOpen = false;
bool Menu::shouldClose = false;

void Menu::Render() {
  if (!isOpen)
    return;

  long ex = GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
  if (ex & WS_EX_TRANSPARENT)
    SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE,
                  ex & ~WS_EX_TRANSPARENT);

  ApplyTheme(Config::Settings.misc.menuTheme);

  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 accent = style.Colors[ImGuiCol_Button];
  ImVec4 accentHover = style.Colors[ImGuiCol_ButtonHovered];

  ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);

  char title[64];
  snprintf(title, sizeof(title), "CS2 External%s###MainWindow",
           s_configDirty ? " *" : "");

  // Улучшенное окно — без стандартного заголовка, с кастомным оформлением
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  if (ImGui::Begin(title, &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
    ImGui::PopStyleVar(2);

    // ─── КАСТОМНЫЙ ЗАГОЛОВОК ───
    ImGui::BeginChild("Header", ImVec2(0, 48), false, ImGuiWindowFlags_NoScrollbar);
    {
      ImGuiStyle &headerStyle = ImGui::GetStyle();
      ImVec2 headerPos = ImGui::GetCursorScreenPos();
      ImGuiWindow *window = ImGui::GetCurrentWindow();

      // Фоновая полоса заголовка с акцентным градиентом
      window->DrawList->AddRectFilled(
          headerPos,
          ImVec2(headerPos.x + ImGui::GetWindowWidth(), headerPos.y + 48),
          ImGui::GetColorU32(ImVec4(
              style.Colors[ImGuiCol_ChildBg].x * 0.85f + accent.x * 0.08f,
              style.Colors[ImGuiCol_ChildBg].y * 0.85f + accent.y * 0.08f,
              style.Colors[ImGuiCol_ChildBg].z * 0.85f + accent.z * 0.08f,
              1.0f)),
          0.0f);

      // Акцентная линия снизу
      window->DrawList->AddLine(
          ImVec2(headerPos.x, headerPos.y + 47),
          ImVec2(headerPos.x + ImGui::GetWindowWidth(), headerPos.y + 47),
          ImGui::GetColorU32(accent), 2.0f);

      // Логотип/название
      ImGui::SetCursorPos(ImVec2(16, 12));
      ImGui::TextColored(accentHover, "CS2");
      ImGui::SameLine(0, 2);
      ImGui::TextColored(style.Colors[ImGuiCol_Text], "External");

      // Кнопка закрытия
      ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 40, 10));
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 0.8f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
      if (ImGui::Button("X", ImVec2(28, 28))) {
        isOpen = false;
        Config::Settings.menuIsOpen = false;
        long ex2 = GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
        SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE, ex2 | WS_EX_TRANSPARENT);
      }
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);
    }
    ImGui::EndChild();

    // ─── ОСНОВНОЙ LAYOUT ───
    // ─── SIDEBAR ───
    ImGui::BeginChild("Sidebar", ImVec2(180, 0), true);

    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

    ImGui::Dummy(ImVec2(0, 12)); // Top padding

    // Навигационные элементы
    const char *tabNames[] = {"LEGIT", "VISUALS", "SKINS", "MISC", "SETTINGS"};
    for (int i = 0; i < 5; i++) {
      bool isSelected = (s_currentTab == i);
      if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Button, accent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accentHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, accent);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
      }
      if (ImGui::Selectable(tabNames[i], isSelected, 0, ImVec2(0, 40)))
        s_currentTab = i;
      if (isSelected) {
        ImGui::PopStyleColor(4);
      }
    }

    ImGui::PopStyleVar(3);

    ImGui::EndChild();

    ImGui::SameLine(0, 10);

    // ─── CONTENT ───
    ImGui::BeginChild("Content", ImVec2(0, 0), false, 0);
    ImGui::Dummy(ImVec2(0, 5));

    if (s_currentTab == 0) { // ─── LEGIT
      ImGui::Columns(2, "LegitCols", false);

      if (UI::BeginCard("Aimbot")) {
        UI::SettingToggle("Enable Aimbot", &Config::Settings.aimbot.enabled);
        if (Config::Settings.aimbot.enabled) {
          ImGui::Spacing();
          UI::SettingHotkey("Hold Key Aimbot", Config::Settings.aimbot.hotkey);
          ImGui::Spacing();
          ImGui::SliderFloat("FOV", &Config::Settings.aimbot.fov, 1.0f, 30.0f, "%.1f deg");
          ImGui::SliderFloat("Smooth", &Config::Settings.aimbot.smooth, 1.0f, 20.0f, "%.1f");
          ImGui::InputFloat("Sensitivity (In-Game)", &Config::Settings.aimbot.sensitivity, 0.01f, 0.1f, "%.3f");
          ImGui::SliderFloat("Jitter", &Config::Settings.aimbot.jitter, 0.0f, 0.15f, "%.3f");

          ImGui::Separator();
          const char *bones[] = {"Pelvis", "Chest", "Neck", "Head"};
          const int bVals[] = {0, 4, 5, 6};
          int sel = 3;
          for (int i = 0; i < 4; i++)
            if (bVals[i] == Config::Settings.aimbot.targetBone) {
              sel = i;
              break;
            }
          if (ImGui::Combo("Target Bone", &sel, bones, 4))
            Config::Settings.aimbot.targetBone = bVals[sel];

          UI::SettingToggle("Target Lock", &Config::Settings.aimbot.targetLock);
          UI::SettingToggle("Visible Only", &Config::Settings.aimbot.visibleOnly);
          UI::SettingToggle("Team Check Aimbot", &Config::Settings.aimbot.teamCheck);
          UI::SettingToggle("Only Scoped", &Config::Settings.aimbot.onlyScoped);
          UI::SettingToggle("Show FOV Circle", &Config::Settings.aimbot.showFov);

          ImGui::Spacing();
          ImGui::Separator();
          ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered], "Recoil Control System (Standalone)");
          UI::SettingToggle("Enable RCS System", &Config::Settings.rcs.enabled);
          if (Config::Settings.rcs.enabled) {
            ImGui::SliderFloat("Pitch Strength", &Config::Settings.rcs.pitchStrength, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Yaw Strength", &Config::Settings.rcs.yawStrength, 0.0f, 2.0f, "%.2f");
            ImGui::SliderInt("Start at Bullet", &Config::Settings.rcs.startBullet, 1, 10);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
            ImGui::TextWrapped("Note: RCS is disabled while Aimbot keys are held to prevent conflict.");
            ImGui::PopStyleColor();
          }
        }
      }
      UI::EndCard();

      ImGui::NextColumn();

      if (UI::BeginCard("Triggerbot")) {
        UI::SettingToggle("Enable Triggerbot", &Config::Settings.triggerbot.enabled);
        if (Config::Settings.triggerbot.enabled) {
          ImGui::Spacing();
          UI::SettingHotkey("Hold Key Trigger", Config::Settings.triggerbot.hotkey);
          ImGui::Spacing();
          ImGui::SliderInt("Min Delay (ms)", &Config::Settings.triggerbot.delayMin, 0, 150);
          ImGui::SliderInt("Max Delay (ms)", &Config::Settings.triggerbot.delayMax, Config::Settings.triggerbot.delayMin, 300);
          UI::SettingToggle("Team Check Trigger", &Config::Settings.triggerbot.teamCheck);
        }
      }
      UI::EndCard();

      ImGui::Columns(1);
    } else if (s_currentTab == 2) { // ─── VISUALS
      ImGui::Columns(2, "VisCols", false);

      if (UI::BeginCard("Player ESP")) {
        UI::SettingToggle("Enable ESP", &Config::Settings.esp.enabled);
        UI::SettingToggle("Team ESP", &Config::Settings.esp.showTeammates);

        ImGui::Spacing();
        UI::SettingToggle("Draw Box", &Config::Settings.esp.showBox);
        UI::SettingColor("##box_color", Config::Settings.esp.boxColor);
        if (Config::Settings.esp.showBox) {
          const char *boxStyles[] = {"Rect", "Corners", "Filled"};
          int bsInt = static_cast<int>(Config::Settings.esp.boxStyle);
          if (ImGui::Combo("Box Style", &bsInt, boxStyles, 3))
            Config::Settings.esp.boxStyle = static_cast<Features::BoxStyle>(bsInt);
          if (Config::Settings.esp.boxStyle == Features::BoxStyle::Filled)
            ImGui::SliderFloat("Fill Alpha", &Config::Settings.esp.fillBoxAlpha, 0.02f, 0.5f, "%.2f");
        }

        UI::SettingToggle("Draw Health Bar", &Config::Settings.esp.showHealth);
        if (Config::Settings.esp.showHealth) {
          const char *hpStyles[] = {"Side", "Bottom"};
          int hsInt = static_cast<int>(Config::Settings.esp.healthBarStyle);
          if (ImGui::Combo("Bar Style", &hsInt, hpStyles, 2))
            Config::Settings.esp.healthBarStyle = static_cast<Features::HealthBarStyle>(hsInt);
        }

        UI::SettingToggle("Draw Name", &Config::Settings.esp.showName);
        UI::SettingColor("##name_color", Config::Settings.esp.nameColor);

        UI::SettingToggle("Draw Weapon", &Config::Settings.esp.showWeapon);
        UI::SettingColor("##weap_color", Config::Settings.esp.weaponColor);

        UI::SettingToggle("Draw Distance", &Config::Settings.esp.showDistance);

        UI::SettingToggle("Snap Lines", &Config::Settings.esp.showSnapLines);
        UI::SettingColor("##snap_color", Config::Settings.esp.snapLineColor);

        UI::SettingToggle("Skeleton", &Config::Settings.esp.showBones);
        UI::SettingColor("##bone_color", Config::Settings.esp.boneColor);
        if (Config::Settings.esp.showBones) {
          UI::SettingToggle("Skeleton Outline", &Config::Settings.esp.skeletonOutline);
          UI::SettingColor("##skel_out_color", Config::Settings.esp.skeletonOutlineColor);
        }
      }
      UI::EndCard();

      ImGui::NextColumn();

      if (UI::BeginCard("Radar Overlay", ImVec2(0, ImGui::GetWindowHeight() * 0.65f))) {
        UI::SettingToggle("Enable Radar", &Config::Settings.radar.enabled);
        if (Config::Settings.radar.enabled) {
          UI::SettingToggle("Rotate Map", &Config::Settings.radar.rotate);
          UI::SettingToggle("Show Teammates Radar", &Config::Settings.radar.showTeammates);
          UI::SettingToggle("Visible Check Radar", &Config::Settings.radar.visibleCheck);

          const char *maps[] = {"Custom", "Mirage", "Dust2", "Inferno", "Nuke"};
          ImGui::Combo("Map", &Config::Settings.radar.mapIndex, maps, 5);
          ImGui::SliderFloat("Zoom", &Config::Settings.radar.zoom, 0.1f, 3.0f, "%.2f");
          ImGui::SliderFloat("Point Size", &Config::Settings.radar.pointSize, 2.0f, 8.0f, "%.1f");
        }
      }
      UI::EndCard();

      if (UI::BeginCard("World Options")) {
        UI::SettingToggle("Enable Bomb Timer", &Config::Settings.bomb.enabled);
      }
      UI::EndCard();

      ImGui::Columns(1);
    } else if (s_currentTab == 1) { // ─── SKINS
      if (UI::BeginCard("Skin Changer")) {
        UI::SettingToggle("Enable Skinchanger", &Config::Settings.skinchanger.enabled);
        if (Config::Settings.skinchanger.enabled) {
          ImGui::Spacing();
          UI::SettingToggle("Auto Apply Skins", &Config::Settings.skinchanger.autoApply);
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
          
          // Knife settings
          UI::SettingToggle("Enable Knife Skin", &Config::Settings.skinchanger.knifeEnabled);
          if (Config::Settings.skinchanger.knifeEnabled) {
            ImGui::SliderInt("Knife Paint Kit", &Config::Settings.skinchanger.knifeSkin.paintKit, 0, 1000);
            ImGui::SliderInt("Knife Seed", &Config::Settings.skinchanger.knifeSkin.seed, 0, 255);
            ImGui::SliderFloat("Knife Wear", &Config::Settings.skinchanger.knifeSkin.wear, 0.0f, 1.0f, "%.4f");
            ImGui::SliderInt("Knife StatTrak", &Config::Settings.skinchanger.knifeSkin.statTrak, -1, 10000);
          }
          
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
          
          // Gloves settings
          UI::SettingToggle("Enable Gloves Skin", &Config::Settings.skinchanger.glovesEnabled);
          if (Config::Settings.skinchanger.glovesEnabled) {
            ImGui::SliderInt("Gloves Paint Kit", &Config::Settings.skinchanger.glovesSkin.paintKit, 0, 10000);
            ImGui::SliderInt("Gloves Seed", &Config::Settings.skinchanger.glovesSkin.seed, 0, 255);
            ImGui::SliderFloat("Gloves Wear", &Config::Settings.skinchanger.glovesSkin.wear, 0.0f, 1.0f, "%.4f");
          }
          
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
          
          // Apply button
          if (ImGui::Button("Apply Skins Now", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
            Config::Settings.skinchanger.applySkins = true;
          }
        }
      }
      UI::EndCard();
    } else if (s_currentTab == 3) { // ─── MISC
      if (UI::BeginCard("Miscellaneous")) {
        UI::SettingToggle("AWP Crosshair", &Config::Settings.misc.awpCrosshair);
        UI::SettingColor("##awp_cross_color", Config::Settings.misc.crosshairColor);

        if (Config::Settings.misc.awpCrosshair) {
          const char *styles[] = {"Dot", "Cross", "Circle", "All"};
          ImGui::Combo("Style", &Config::Settings.misc.crosshairStyle, styles, 4);
          ImGui::SliderFloat("Size", &Config::Settings.misc.crosshairSize, 2.0f, 25.0f, "%.0f px");
          ImGui::SliderFloat("Thickness", &Config::Settings.misc.crosshairThickness, 0.5f, 4.0f, "%.1f");
          UI::SettingToggle("Center Gap", &Config::Settings.misc.crosshairGap);
        }
      }
      UI::EndCard();
    } else if (s_currentTab == 4) { // ─── SETTINGS
      ImGui::Columns(2, "SetCols", false);

      if (UI::BeginCard("Configuration")) {
        const char *themes[] = {"Midnight", "Blood",      "Cyber", "Lavender",
                                "Gold",     "Monochrome", "Toxic"};
        if (ImGui::Combo("Theme Preset", &Config::Settings.misc.menuTheme, themes, 7)) {
            Config::ConfigManager::ApplySettings();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ─── Offset Update Section ────────────────────────────────────────────
        ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_Text], "Offsets Management");
        ImGui::Spacing();

        if (ImGui::Button("Update Offsets from GitHub", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
          s_offsetUpdatePending = true;
          s_offsetUpdateSuccess = false;
          s_offsetUpdateMessage = "Updating...";
          s_offsetUpdateFuture = SDK::Updater::ForceUpdateOffsets();
        }

        if (s_offsetUpdatePending && s_offsetUpdateFuture.valid() && 
            s_offsetUpdateFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          s_offsetUpdateSuccess = s_offsetUpdateFuture.get();
          s_offsetUpdateMessage = s_offsetUpdateSuccess ? "Offsets updated successfully!" : "Failed to update offsets.";
        }

        if (!s_offsetUpdateMessage.empty()) {
          ImGui::Spacing();
          ImVec4 msgColor = s_offsetUpdateSuccess
                                ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f)
                                : ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
          ImGui::TextColored(msgColor, "%s", s_offsetUpdateMessage.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::InputText("Name", s_configName, sizeof(s_configName));
        if (ImGui::Button("Save Config", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
          Config::ConfigManager::Save(s_configName);
          s_configList = Config::ConfigManager::ListConfigs();
        }

        ImGui::Spacing();
        if (ImGui::Button("Refresh List", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
          s_configList = Config::ConfigManager::ListConfigs();

        ImGui::BeginChild("List", ImVec2(0, 120), true);
        for (int i = 0; i < (int)s_configList.size(); i++) {
          if (ImGui::Selectable(s_configList[i].c_str(), i == s_configSelected))
            s_configSelected = i;
        }
        ImGui::EndChild();

        if (!s_configList.empty()) {
          if (ImGui::Button("Load Selected", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            strncpy_s(s_configName, s_configList[s_configSelected].c_str(), sizeof(s_configName) - 1);
            Config::ConfigManager::Load(s_configList[s_configSelected]);
          }
        }
      }
      UI::EndCard();

      ImGui::NextColumn();

      if (UI::BeginCard("Performance & Debug")) {
        UI::SettingToggle("Enable VSync", &Config::Settings.performance.vsyncEnabled);
        ImGui::SliderInt("FPS Limit", &Config::Settings.performance.fpsLimit, 10, 500, "%d FPS");
        ImGui::SliderInt("UPS Limit (Max)", &Config::Settings.performance.upsLimit, 10, 500, "%d UPS");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Show current FPS/UPS when VSync is active
        if (Config::Settings.performance.vsyncEnabled) {
          ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "VSync Active - FPS/UPS synced to display");
          ImGui::Spacing();
        }

        UI::SettingToggle("Enable Debug Overlay", &Config::Settings.debug.enabled);
        UI::SettingToggle("Developer Mode", &Config::Settings.debug.devMode);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.0f, 0.0f, 1));
        if (ImGui::Button("UNLOAD / EXIT", ImVec2(ImGui::GetContentRegionAvail().x, 35)))
          shouldClose = true;
        ImGui::PopStyleColor(3);
      }
      UI::EndCard();
      ImGui::Columns(1);
    }

    ImGui::EndChild(); // Content
  } else {
    ImGui::PopStyleVar(2);
  }
  ImGui::End();
}

bool Menu::IsOpen() { return isOpen; }
bool Menu::ShouldClose() { return shouldClose; }
void Menu::Toggle() {
  isOpen = !isOpen;
  Config::Settings.menuIsOpen = isOpen;
  if (!isOpen) {
    long ex = GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
    SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE,
                  ex | WS_EX_TRANSPARENT);
  }
}

} // namespace Render
