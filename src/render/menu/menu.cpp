#include "menu.h"
#include "../overlay/overlay.h"
#include "config/settings.h"
#include "features/chams/chams.h"
#include "features/esp/esp_config.h"
#include "features/feature_manager.h"
#include "menu_theme.h"
#include "tab_legit.h"
#include "tab_misc.h"
#include "tab_settings.h"
#include "tab_visuals.h"
#include "tab_world.h"
#include "../renderer/imgui_manager.h"
#include "ui_components.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cfloat>
#include <imgui.h>
#include <imgui_internal.h>
#include <windows.h>

namespace Render {

bool Menu::isOpen = false;
bool Menu::shouldClose = false;

namespace {

enum class MenuSection : int {
  Legitbot = 0,
  Visuals = 1,
  World = 2,
  Misc = 3,
  Settings = 4,
};

struct PreviewRenderCallbackData {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  bool teammate = false;
};

static MenuSection s_currentSection = MenuSection::Visuals;
static int s_lastAppliedTheme = -1;
static int s_previewAffinity = 0;
static bool s_previewExpanded = true;
static std::array<int, 5> s_subTabs = {0, 0, 0, 0, 0};

constexpr float kFrameRadius = 8.0f;
constexpr float kPanelGap = 12.0f;
constexpr float kSidebarWidth = 170.0f;
constexpr float kPreviewWidth = 250.0f;
constexpr float kHeaderHeight = 94.0f;
constexpr float kPreviewTabWidth = 90.0f;
constexpr float kPreviewTabHeight = 22.0f;
constexpr float kPreviewGridStep = 24.0f;

void RestoreOverlayTransparency() {
  HWND hwnd = Render::Overlay::GetWindowHandle();
  if (!hwnd) {
    return;
  }

  long ex = GetWindowLong(hwnd, GWL_EXSTYLE);
  SetWindowLong(hwnd, GWL_EXSTYLE, ex | WS_EX_TRANSPARENT);
}

int SectionIndex(MenuSection section) {
  return static_cast<int>(section);
}

const char *GetThemeName(int theme) {
  switch (theme) {
  case 1:
    return "Blood";
  case 2:
    return "Cyber";
  case 3:
    return "Lavender";
  case 4:
    return "Gold";
  case 5:
    return "Mono";
  case 6:
    return "Toxic";
  default:
    return "Midnight";
  }
}

const char *GetSectionLabel(MenuSection section) {
  switch (section) {
  case MenuSection::Legitbot:
    return "Legitbot";
  case MenuSection::Visuals:
    return "Visuals";
  case MenuSection::World:
    return "World";
  case MenuSection::Misc:
    return "Misc";
  default:
    return "Settings";
  }
}

const char *GetSectionDescription(MenuSection section) {
  switch (section) {
  case MenuSection::Legitbot:
    return "Combat controls";
  case MenuSection::Visuals:
    return "Player ESP";
  case MenuSection::World:
    return "Radar and sound";
  case MenuSection::Misc:
    return "Overlay tools";
  default:
    return "Configs and system";
  }
}

const char *GetPreviewTitle(MenuSection section) {
  switch (section) {
  case MenuSection::World:
    return "World Preview";
  default:
    return "ESP Preview";
  }
}

bool ShouldShowPreview(MenuSection section) {
  return section == MenuSection::Visuals || section == MenuSection::World;
}

void GetSubTabs(MenuSection section, const char *const *&labels, int &count) {
  static const char *kLegit[] = {"AIMBOT", "TRIGGER", "RCS"};
  static const char *kVisuals[] = {"ESP", "EFFECTS"};
  static const char *kWorld[] = {"RADAR", "SOUND"};
  static const char *kMisc[] = {"CROSSHAIR"};
  static const char *kSettings[] = {"CONFIGS", "SYSTEM"};

  switch (section) {
  case MenuSection::Legitbot:
    labels = kLegit;
    count = 3;
    break;
  case MenuSection::Visuals:
    labels = kVisuals;
    count = 2;
    break;
  case MenuSection::World:
    labels = kWorld;
    count = 2;
    break;
  case MenuSection::Misc:
    labels = kMisc;
    count = 1;
    break;
  default:
    labels = kSettings;
    count = 2;
    break;
  }
}

void DrawWireBackdrop(ImDrawList *drawList, const ImVec2 &pos,
                      const ImVec2 &size) {
  static constexpr ImVec2 kPoints[] = {
      {0.08f, 0.11f}, {0.22f, 0.06f}, {0.39f, 0.14f}, {0.58f, 0.05f},
      {0.76f, 0.10f}, {0.90f, 0.04f}, {0.14f, 0.33f}, {0.28f, 0.24f},
      {0.46f, 0.31f}, {0.65f, 0.22f}, {0.84f, 0.27f}, {0.94f, 0.38f},
      {0.11f, 0.56f}, {0.24f, 0.47f}, {0.43f, 0.55f}, {0.60f, 0.47f},
      {0.79f, 0.58f}, {0.92f, 0.49f}, {0.07f, 0.82f}, {0.27f, 0.75f},
      {0.48f, 0.86f}, {0.66f, 0.72f}, {0.82f, 0.84f}, {0.94f, 0.76f},
  };
  static constexpr int kEdges[][2] = {
      {0, 1},  {1, 2},  {2, 3},  {3, 4},  {4, 5},  {0, 6},  {1, 7},
      {2, 8},  {3, 9},  {4, 10}, {5, 11}, {6, 7},  {7, 8},  {8, 9},
      {9, 10}, {10, 11}, {6, 12}, {7, 13}, {8, 14}, {9, 15}, {10, 16},
      {11, 17}, {12, 13}, {13, 14}, {14, 15}, {15, 16}, {16, 17}, {12, 18},
      {13, 19}, {14, 20}, {15, 21}, {16, 22}, {17, 23}, {18, 19}, {19, 20},
      {20, 21}, {21, 22}, {22, 23},
  };

  const ImU32 lineColor = ImGui::GetColorU32(ImVec4(0.22f, 0.29f, 0.38f, 0.15f));
  for (const auto &edge : kEdges) {
    const ImVec2 a(pos.x + size.x * kPoints[edge[0]].x,
                   pos.y + size.y * kPoints[edge[0]].y);
    const ImVec2 b(pos.x + size.x * kPoints[edge[1]].x,
                   pos.y + size.y * kPoints[edge[1]].y);
    drawList->AddLine(a, b, lineColor, 1.0f);
  }
}

void DrawAppFrame(ImDrawList *drawList, const ImVec2 &pos, const ImVec2 &size,
                  const ImVec4 &accent) {
  const ImU32 outer = ImGui::GetColorU32(ImVec4(0.07f, 0.07f, 0.08f, 0.985f));
  const ImU32 sidebar = ImGui::GetColorU32(ImVec4(0.06f, 0.06f, 0.07f, 1.0f));
  const ImU32 border = ImGui::GetColorU32(ImVec4(0.18f, 0.19f, 0.22f, 1.0f));
  const ImU32 accentBar =
      ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.95f));

  drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), outer,
                          kFrameRadius);
  drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), border,
                    kFrameRadius, 0, 1.0f);
  drawList->AddRectFilled(pos,
                          ImVec2(pos.x + kSidebarWidth + 10.0f, pos.y + size.y),
                          sidebar, kFrameRadius,
                          ImDrawFlags_RoundCornersTopLeft |
                              ImDrawFlags_RoundCornersBottomLeft);
  drawList->AddLine(ImVec2(pos.x + kSidebarWidth + 10.0f, pos.y + 1.0f),
                    ImVec2(pos.x + kSidebarWidth + 10.0f, pos.y + size.y - 1.0f),
                    border, 1.0f);
  drawList->AddRectFilled(ImVec2(pos.x + 1.0f, pos.y + 1.0f),
                          ImVec2(pos.x + 4.0f, pos.y + size.y - 1.0f), accentBar,
                          kFrameRadius,
                          ImDrawFlags_RoundCornersTopLeft |
                              ImDrawFlags_RoundCornersBottomLeft);

  DrawWireBackdrop(drawList, ImVec2(pos.x + kSidebarWidth + 20.0f, pos.y + 6.0f),
                   ImVec2(size.x - kSidebarWidth - 30.0f, size.y - 12.0f));
}

void DrawSidebarBrand() {
  if (ImFont *font = Render::ImGuiManager::GetSemiboldFont()) {
    ImGui::PushFont(font);
  }
  ImGui::TextColored(ImVec4(0.22f, 0.53f, 1.0f, 1.0f), "G");
  ImGui::SameLine(0.0f, 6.0f);
  ImGui::TextColored(ImVec4(0.94f, 0.96f, 0.99f, 1.0f), "ENESIS");
  if (Render::ImGuiManager::GetSemiboldFont()) {
    ImGui::PopFont();
  }
}

void DrawSidebarGroupLabel(const char *label) {
  ImGui::Dummy(ImVec2(0.0f, 10.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.42f, 0.48f, 1.0f));
  ImGui::TextUnformatted(label);
  ImGui::PopStyleColor();
  ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

bool DrawSidebarEntry(MenuSection section) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  const ImGuiStyle &style = ImGui::GetStyle();
  const bool selected = s_currentSection == section;
  const char *label = GetSectionLabel(section);
  const char *hint = GetSectionDescription(section);
  const ImGuiID id = window->GetID(label);
  const ImVec2 pos = window->DC.CursorPos;
  const float height = 48.0f;
  const ImRect bb(pos,
                  ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + height));

  ImGui::ItemSize(bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, id)) {
    return false;
  }

  bool hovered = false;
  bool held = false;
  const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];
  const ImU32 bg = ImGui::GetColorU32(
      selected ? ImVec4(0.15f, 0.16f, 0.19f, 0.95f)
               : hovered ? ImVec4(0.12f, 0.13f, 0.15f, 0.85f)
                         : ImVec4(0.00f, 0.00f, 0.00f, 0.0f));
  window->DrawList->AddRectFilled(bb.Min, bb.Max, bg, 4.0f);
  ImGui::RenderNavHighlight(bb, id);
  if (selected) {
    window->DrawList->AddRectFilled(
        ImVec2(bb.Min.x - 2.0f, bb.Min.y + 4.0f),
        ImVec2(bb.Min.x + 1.5f, bb.Max.y - 4.0f), ImGui::GetColorU32(accent),
        1.0f);
  }

  const ImVec4 labelColor =
      selected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.72f, 0.74f, 0.78f, 0.96f);
  const ImVec4 hintColor = selected ? ImVec4(0.53f, 0.69f, 1.0f, 0.85f)
                                    : ImVec4(0.45f, 0.47f, 0.52f, 0.95f);
  window->DrawList->AddText(ImVec2(bb.Min.x + 12.0f, bb.Min.y + 8.0f),
                            ImGui::GetColorU32(labelColor), label);
  window->DrawList->AddText(ImVec2(bb.Min.x + 12.0f, bb.Min.y + 25.0f),
                            ImGui::GetColorU32(hintColor), hint);
  return pressed;
}

void DrawSidebar(const Config::GlobalSettings &settings) {
  ImGui::BeginChild("GenesisSidebar", ImVec2(kSidebarWidth, 0.0f), false,
                    ImGuiWindowFlags_None);
  DrawSidebarBrand();
  ImGui::Dummy(ImVec2(0.0f, 18.0f));

  DrawSidebarGroupLabel("COMBAT");
  if (DrawSidebarEntry(MenuSection::Legitbot)) {
    s_currentSection = MenuSection::Legitbot;
  }

  DrawSidebarGroupLabel("VISUALS");
  if (DrawSidebarEntry(MenuSection::Visuals)) {
    s_currentSection = MenuSection::Visuals;
  }
  if (DrawSidebarEntry(MenuSection::World)) {
    s_currentSection = MenuSection::World;
  }

  DrawSidebarGroupLabel("OTHER");
  if (DrawSidebarEntry(MenuSection::Misc)) {
    s_currentSection = MenuSection::Misc;
  }
  if (DrawSidebarEntry(MenuSection::Settings)) {
    s_currentSection = MenuSection::Settings;
  }

  ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 86.0f);
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 8.0f));
  ImGui::TextDisabled("Theme");
  ImGui::Text("%s", GetThemeName(settings.misc.menuTheme));
  ImGui::Dummy(ImVec2(0.0f, 4.0f));
  ImGui::TextDisabled("Menu  INSERT");
  ImGui::TextDisabled("Exit  END");
  ImGui::EndChild();
}

void DrawPreviewTabs() {
  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];
  const ImVec4 muted = style.Colors[ImGuiCol_TextDisabled];
  const char *labels[] = {"ENEMIES", "TEAM"};

  for (int i = 0; i < 2; ++i) {
    if (i != 0) {
      ImGui::SameLine(0.0f, 8.0f);
    }
    if (ImGui::InvisibleButton(labels[i],
                               ImVec2(kPreviewTabWidth, kPreviewTabHeight))) {
      s_previewAffinity = i;
    }

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max,
                            ImGui::GetColorU32(ImVec4(0.13f, 0.14f, 0.17f, 1.0f)),
                            3.0f);
    drawList->AddRect(min, max,
                      ImGui::GetColorU32(ImVec4(0.21f, 0.23f, 0.27f, 1.0f)),
                      3.0f, 0, 1.0f);
    if (i == s_previewAffinity) {
      drawList->AddLine(ImVec2(min.x, max.y + 3.0f),
                        ImVec2(max.x, max.y + 3.0f), ImGui::GetColorU32(accent),
                        2.0f);
    }
    const ImVec2 textSize = ImGui::CalcTextSize(labels[i]);
    const ImVec2 textPos(min.x + (kPreviewTabWidth - textSize.x) * 0.5f,
                         min.y + (kPreviewTabHeight - textSize.y) * 0.5f - 1.0f);
    drawList->AddText(
        textPos,
        ImGui::GetColorU32(i == s_previewAffinity ? ImVec4(0.95f, 0.97f, 1.0f, 1.0f)
                                                  : muted),
        labels[i]);
  }
}

void DrawBracketBox(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max,
                    const ImVec4 &accent) {
  const float len = 18.0f;
  const ImU32 col = ImGui::GetColorU32(accent);

  drawList->AddLine(min, ImVec2(min.x + len, min.y), col, 1.6f);
  drawList->AddLine(min, ImVec2(min.x, min.y + len), col, 1.6f);
  drawList->AddLine(ImVec2(max.x - len, min.y), ImVec2(max.x, min.y), col, 1.6f);
  drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + len), col, 1.6f);
  drawList->AddLine(ImVec2(min.x, max.y - len), ImVec2(min.x, max.y), col, 1.6f);
  drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + len, max.y), col, 1.6f);
  drawList->AddLine(ImVec2(max.x - len, max.y), max, col, 1.6f);
  drawList->AddLine(ImVec2(max.x, max.y - len), max, col, 1.6f);
}

void DrawPreviewFigure(ImDrawList *drawList, const ImVec2 &center, float scale,
                       const ImVec4 &accent) {
  const ImU32 body = ImGui::GetColorU32(ImVec4(0.84f, 0.88f, 0.96f, 0.75f));
  const ImU32 glow = ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.22f));
  const ImU32 line = ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.85f));

  drawList->AddCircleFilled(ImVec2(center.x, center.y - scale * 92.0f),
                            scale * 22.0f, glow, 24);
  drawList->AddCircleFilled(ImVec2(center.x, center.y - scale * 94.0f),
                            scale * 18.0f, body, 24);
  drawList->AddRectFilled(ImVec2(center.x - scale * 22.0f, center.y - scale * 64.0f),
                          ImVec2(center.x + scale * 22.0f, center.y + scale * 26.0f),
                          body, 16.0f);
  drawList->AddLine(ImVec2(center.x - scale * 18.0f, center.y - scale * 42.0f),
                    ImVec2(center.x - scale * 52.0f, center.y + scale * 10.0f), line,
                    3.0f);
  drawList->AddLine(ImVec2(center.x + scale * 18.0f, center.y - scale * 42.0f),
                    ImVec2(center.x + scale * 56.0f, center.y + scale * 8.0f), line,
                    3.0f);
  drawList->AddLine(ImVec2(center.x - scale * 8.0f, center.y + scale * 26.0f),
                    ImVec2(center.x - scale * 24.0f, center.y + scale * 98.0f), line,
                    3.0f);
  drawList->AddLine(ImVec2(center.x + scale * 8.0f, center.y + scale * 26.0f),
                    ImVec2(center.x + scale * 26.0f, center.y + scale * 98.0f), line,
                    3.0f);
  drawList->AddCircleFilled(ImVec2(center.x - scale * 64.0f, center.y + scale * 34.0f),
                            scale * 58.0f, glow, 36);
}

void RenderPreviewModelCallback(const ImDrawList *, const ImDrawCmd *cmd) {
  auto *data = static_cast<PreviewRenderCallbackData *>(cmd->UserCallbackData);
  if (!data) {
    return;
  }

  if (auto *feature = Features::FeatureManager::GetFeature("Chams")) {
    if (auto *chams = dynamic_cast<Features::Chams *>(feature)) {
      chams->RenderPreview(data->x, data->y, data->width, data->height,
                           data->teammate);
    }
  }
}

void DestroyPreviewCallbackData(const ImDrawList *, const ImDrawCmd *cmd) {
  delete static_cast<PreviewRenderCallbackData *>(cmd->UserCallbackData);
}

void DrawRectBox(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max,
                 const ImVec4 &color) {
  const ImU32 col = ImGui::GetColorU32(color);
  drawList->AddRect(min, max, col, 0.0f, 0, 1.8f);
  drawList->AddRect(ImVec2(min.x - 1.0f, min.y - 1.0f),
                    ImVec2(max.x + 1.0f, max.y + 1.0f), IM_COL32(0, 0, 0, 160),
                    0.0f, 0, 1.0f);
}

void DrawFilledBox(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max,
                   const ImVec4 &color, float alpha) {
  drawList->AddRectFilled(
      min, max, ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, alpha)), 0.0f);
  DrawRectBox(drawList, min, max, color);
}

void DrawPreviewEspOverlay(ImDrawList *drawList, const Config::GlobalSettings &settings,
                           const ImVec2 &center, float scale) {
  const bool teammate = s_previewAffinity == 1;
  const float *colSrc = teammate ? settings.esp.teamColor : settings.esp.boxColor;
  const ImVec4 boxColor(colSrc[0], colSrc[1], colSrc[2], colSrc[3]);
  const ImVec2 boxMin(center.x - 68.0f * scale, center.y - 132.0f * scale);
  const ImVec2 boxMax(center.x + 68.0f * scale, center.y + 102.0f * scale);

  if (settings.esp.showBox) {
    switch (settings.esp.boxStyle) {
    case Features::BoxStyle::Corners:
      DrawBracketBox(drawList, boxMin, boxMax, boxColor);
      break;
    case Features::BoxStyle::Filled:
      DrawFilledBox(drawList, boxMin, boxMax, boxColor, settings.esp.fillBoxAlpha);
      break;
    default:
      DrawRectBox(drawList, boxMin, boxMax, boxColor);
      break;
    }
  }

  if (settings.esp.showHealth) {
    const float hpRatio = 0.76f;
    const ImVec2 barMin(boxMin.x - 10.0f, boxMin.y);
    const ImVec2 barMax(boxMin.x - 6.0f, boxMax.y);
    drawList->AddRectFilled(barMin, barMax, IM_COL32(18, 20, 24, 220), 2.0f);
    drawList->AddRectFilled(
        ImVec2(barMin.x, barMax.y - (barMax.y - barMin.y) * hpRatio), barMax,
        IM_COL32(126, 241, 68, 255), 2.0f);
  }

  if (settings.esp.showName) {
    drawList->AddText(ImVec2(center.x - 42.0f, boxMin.y - 24.0f),
                      IM_COL32(255, 255, 255, 255),
                      teammate ? "teammate_01" : "enemy_player");
  }

  if (settings.esp.showDistance) {
    drawList->AddText(ImVec2(center.x - 16.0f, boxMax.y + 10.0f),
                      IM_COL32(138, 143, 152, 255),
                      teammate ? "24 m" : "18 m");
  }

  if (settings.esp.showWeapon) {
    drawList->AddRectFilled(ImVec2(center.x - 36.0f, boxMax.y + 30.0f),
                            ImVec2(center.x + 36.0f, boxMax.y + 52.0f),
                            IM_COL32(24, 28, 34, 242), 4.0f);
    drawList->AddRect(ImVec2(center.x - 36.0f, boxMax.y + 30.0f),
                      ImVec2(center.x + 36.0f, boxMax.y + 52.0f),
                      ImGui::GetColorU32(boxColor), 4.0f, 0, 1.0f);
    drawList->AddText(ImVec2(center.x - 15.0f, boxMax.y + 35.0f),
                      IM_COL32(255, 255, 255, 235),
                      teammate ? "m4a1" : "ak47");
  }

  if (settings.esp.showSnapLines) {
    const float *snap = settings.esp.snapLineColor;
    drawList->AddLine(ImVec2(center.x, boxMax.y + 72.0f), ImVec2(center.x, boxMax.y),
                      ImGui::GetColorU32(
                          ImVec4(snap[0], snap[1], snap[2], snap[3])),
                      1.2f);
  }
}

void DrawPreviewViewport(const Config::GlobalSettings &settings) {
  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];
  const ImVec2 panelPos = ImGui::GetCursorScreenPos();
  const ImVec2 panelSize = ImGui::GetContentRegionAvail();
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  drawList->AddRectFilled(panelPos,
                          ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
                          ImGui::GetColorU32(ImVec4(0.14f, 0.14f, 0.16f, 1.0f)), 0.0f);
  drawList->AddRect(panelPos,
                    ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
                    ImGui::GetColorU32(ImVec4(0.24f, 0.25f, 0.28f, 1.0f)), 0.0f, 0,
                    1.0f);

  for (float x = panelPos.x + 18.0f; x < panelPos.x + panelSize.x;
       x += kPreviewGridStep) {
    drawList->AddLine(
        ImVec2(x, panelPos.y + 48.0f),
        ImVec2(x, panelPos.y + panelSize.y - 18.0f),
        ImGui::GetColorU32(ImVec4(0.14f, 0.16f, 0.20f, 0.20f)), 1.0f);
  }
  for (float y = panelPos.y + 64.0f; y < panelPos.y + panelSize.y;
       y += kPreviewGridStep) {
    drawList->AddLine(
        ImVec2(panelPos.x + 18.0f, y),
        ImVec2(panelPos.x + panelSize.x - 18.0f, y),
        ImGui::GetColorU32(ImVec4(0.14f, 0.16f, 0.20f, 0.18f)), 1.0f);
  }

  ImGui::SetCursorPos(
      ImVec2(ImGui::GetCursorPosX() + 16.0f, ImGui::GetCursorPosY() + 12.0f));
  DrawPreviewTabs();

  const ImVec2 center(panelPos.x + panelSize.x * 0.50f, panelPos.y + panelSize.y * 0.58f);
  const float scale = (std::min)(panelSize.x, panelSize.y) / 340.0f;
  const ImVec2 previewMin(panelPos.x + 24.0f, panelPos.y + 62.0f);
  const ImVec2 previewMax(panelPos.x + panelSize.x - 24.0f,
                          panelPos.y + panelSize.y - 26.0f);

  bool hasModelPreview = false;
  if (auto *feature = Features::FeatureManager::GetFeature("Chams")) {
    if (auto *chams = dynamic_cast<Features::Chams *>(feature)) {
      hasModelPreview = chams->CanRenderPreview();
    }
  }

  if (hasModelPreview) {
    auto *callbackData = new PreviewRenderCallbackData{
        previewMin.x, previewMin.y, previewMax.x - previewMin.x,
        previewMax.y - previewMin.y, s_previewAffinity == 1};
    drawList->AddCallback(RenderPreviewModelCallback, callbackData);
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    drawList->AddCallback(DestroyPreviewCallbackData, callbackData);
  } else {
    DrawPreviewFigure(drawList, center, scale, accent);
  }

  DrawPreviewEspOverlay(drawList, settings, center, scale);
}

void DrawPreviewShell(MenuSection section, const Config::GlobalSettings &settings) {
  ImGui::BeginChild("PreviewShell", ImVec2(kPreviewWidth, 0.0f), false,
                    ImGuiWindowFlags_None);
  if (ImFont *font = Render::ImGuiManager::GetSemiboldFont()) {
    ImGui::PushFont(font);
    ImGui::TextColored(ImVec4(0.96f, 0.97f, 1.0f, 1.0f), "%s",
                       GetPreviewTitle(section));
    ImGui::PopFont();
  } else {
    ImGui::TextUnformatted(GetPreviewTitle(section));
  }
  ImGui::SameLine();
  ImGui::TextDisabled("live preview");
  ImGui::Dummy(ImVec2(0.0f, 8.0f));
  DrawPreviewViewport(settings);
  ImGui::EndChild();
}

void DrawSubTabBar(MenuSection section) {
  const char *const *labels = nullptr;
  int count = 0;
  GetSubTabs(section, labels, count);
  const int sectionIndex = SectionIndex(section);

  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];

  for (int i = 0; i < count; ++i) {
    if (i != 0) {
      ImGui::SameLine(0.0f, 20.0f);
      ImGui::TextDisabled("|");
      ImGui::SameLine(0.0f, 20.0f);
    }

    const ImVec2 tabSize(104.0f, 28.0f);
    if (ImGui::InvisibleButton(labels[i], tabSize)) {
      s_subTabs[sectionIndex] = i;
    }
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const bool active = s_subTabs[sectionIndex] == i;
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(min.x, min.y + 5.0f),
        ImGui::GetColorU32(active ? ImVec4(0.95f, 0.97f, 1.0f, 1.0f)
                                  : ImVec4(0.52f, 0.54f, 0.58f, 1.0f)),
        labels[i]);
    if (active) {
      ImGui::GetWindowDrawList()->AddLine(
          ImVec2(min.x, max.y + 2.0f), ImVec2(max.x - 10.0f, max.y + 2.0f),
          ImGui::GetColorU32(accent), 2.0f);
    }
  }
}

void DrawContentHeader(MenuSection section, const Config::GlobalSettings &settings) {
  ImGui::BeginChild("ContentHeader", ImVec2(0.0f, kHeaderHeight), false,
                    ImGuiWindowFlags_None);

  if (ImFont *font = Render::ImGuiManager::GetSemiboldFont()) {
    ImGui::PushFont(font);
    ImGui::TextColored(ImVec4(0.95f, 0.97f, 1.0f, 1.0f), "%s",
                       GetSectionLabel(section));
    ImGui::PopFont();
  } else {
    ImGui::TextUnformatted(GetSectionLabel(section));
  }

  ImGui::SameLine();
  ImGui::TextDisabled("%s", GetSectionDescription(section));

  const char *pacing = settings.performance.vsyncEnabled ? "VSync" : "Manual";
  const float closeWidth = 28.0f;
  const float statusWidth = ImGui::CalcTextSize(GetThemeName(settings.misc.menuTheme)).x +
                            ImGui::CalcTextSize(pacing).x + 170.0f +
                            (ShouldShowPreview(section) ? 92.0f : 0.0f);
  ImGui::SetCursorPosX(
      (std::max)(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - statusWidth));
  ImGui::TextDisabled("Theme");
  ImGui::SameLine(0.0f, 6.0f);
  ImGui::Text("%s", GetThemeName(settings.misc.menuTheme));
  ImGui::SameLine(0.0f, 14.0f);
  ImGui::TextDisabled("Pacing");
  ImGui::SameLine(0.0f, 6.0f);
  ImGui::Text("%s", pacing);

  if (ShouldShowPreview(section)) {
    ImGui::SameLine(0.0f, 16.0f);
    if (ImGui::Button(s_previewExpanded ? "Hide Preview" : "Show Preview")) {
      s_previewExpanded = !s_previewExpanded;
    }
  }

  ImGui::SameLine(0.0f, 14.0f);
  if (ImGui::Button("X", ImVec2(closeWidth, 24.0f))) {
    Menu::isOpen = false;
    RestoreOverlayTransparency();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Close menu (Insert)");
  }

  ImGui::Dummy(ImVec2(0.0f, 12.0f));
  DrawSubTabBar(section);
  ImGui::SetCursorPosY(kHeaderHeight - 1.0f);
  ImGui::Separator();
  ImGui::EndChild();
}

void RenderSectionContent(MenuSection section) {
  const int subTab = s_subTabs[SectionIndex(section)];
  switch (section) {
  case MenuSection::Legitbot:
    RenderTabLegit(subTab);
    break;
  case MenuSection::Visuals:
    RenderTabVisuals(subTab);
    break;
  case MenuSection::World:
    RenderTabWorld(subTab);
    break;
  case MenuSection::Misc:
    RenderTabMisc(subTab);
    break;
  case MenuSection::Settings:
  default:
    RenderTabSettings(subTab);
    break;
  }
}

} // namespace

void Menu::Render() {
  if (!isOpen) {
    return;
  }

  Config::GlobalSettings settings = Config::CopySettings();
  if (settings.misc.menuTheme < 0 || settings.misc.menuTheme > 6) {
    Config::MutateSettingsVoid([](auto &state) { state.misc.menuTheme = 0; });
    settings.misc.menuTheme = 0;
  }

  HWND overlayWindow = Render::Overlay::GetWindowHandle();
  if (overlayWindow) {
    long ex = GetWindowLong(overlayWindow, GWL_EXSTYLE);
    if (ex & WS_EX_TRANSPARENT) {
      SetWindowLong(overlayWindow, GWL_EXSTYLE, ex & ~WS_EX_TRANSPARENT);
    }
  }

  if (settings.misc.menuTheme != s_lastAppliedTheme) {
    ApplyMenuTheme(settings.misc.menuTheme);
    s_lastAppliedTheme = settings.misc.menuTheme;
  }

  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];

  const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
  const float maxWidth = (std::max)(1.0f, displaySize.x - 24.0f);
  const float maxHeight = (std::max)(1.0f, displaySize.y - 24.0f);
  const float minWidth = (std::min)(720.0f, maxWidth);
  const float minHeight = (std::min)(500.0f, maxHeight);
  ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, minHeight),
                                      ImVec2(maxWidth, maxHeight));
  ImGui::SetNextWindowSize(ImVec2((std::min)(1360.0f, maxWidth),
                                  (std::min)(820.0f, maxHeight)),
                           ImGuiCond_FirstUseEver);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 14.0f));

  if (ImGui::Begin("GenesisMenu###MainWindow", &isOpen,
                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
    ImGui::PopStyleVar();

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    DrawAppFrame(ImGui::GetWindowDrawList(), windowPos, windowSize, accent);

    DrawSidebar(settings);
    ImGui::SameLine(0.0f, kPanelGap);

    const bool showPreview = ShouldShowPreview(s_currentSection) && s_previewExpanded;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const bool previewFits = availableWidth >= 640.0f + kPreviewWidth + kPanelGap;
    const bool showPreviewNow = showPreview && previewFits;
    const float mainWidth = showPreviewNow
                                ? availableWidth - kPreviewWidth - kPanelGap
                                : availableWidth;

    ImGui::BeginChild("MainColumn", ImVec2(mainWidth, 0.0f), false,
                      ImGuiWindowFlags_None);
    DrawContentHeader(s_currentSection, settings);
    RenderSectionContent(s_currentSection);
    ImGui::EndChild();

    if (showPreviewNow) {
      ImGui::SameLine(0.0f, kPanelGap);
      DrawPreviewShell(s_currentSection, settings);
    }
  } else {
    ImGui::PopStyleVar();
  }

  ImGui::End();

  if (!isOpen) {
    RestoreOverlayTransparency();
  }
}

bool Menu::IsOpen() { return isOpen; }
bool Menu::ShouldClose() { return shouldClose; }

void Menu::Toggle() {
  isOpen = !isOpen;
  if (!isOpen) {
    RestoreOverlayTransparency();
  } else {
    shouldClose = false;
  }
}

} // namespace Render
