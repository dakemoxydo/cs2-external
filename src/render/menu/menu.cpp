#include "menu.h"
#include "../overlay/overlay.h"
#include "config/settings.h"
#include "features/esp/esp_config.h"
#include "menu_theme.h"
#include "tab_legit.h"
#include "tab_misc.h"
#include "tab_settings.h"
#include "tab_visuals.h"
#include "ui_components.h"
#include <algorithm>
#include <imgui.h>
#include <windows.h>

namespace Render {

bool Menu::isOpen = false;
bool Menu::shouldClose = false;

static int s_currentTab = 0;
static int s_lastAppliedTheme = -1;
static int s_previewAffinity = 0;

namespace {

void RestoreOverlayTransparency() {
  HWND hwnd = Render::Overlay::GetWindowHandle();
  if (!hwnd) {
    return;
  }

  long ex = GetWindowLong(hwnd, GWL_EXSTYLE);
  SetWindowLong(hwnd, GWL_EXSTYLE, ex | WS_EX_TRANSPARENT);
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

const char *GetTabLabel(int tab) {
  switch (tab) {
  case 0:
    return "Visuals";
  case 1:
    return "Legit";
  case 2:
    return "Misc";
  default:
    return "Settings";
  }
}

const char *GetTabHint(int tab) {
  switch (tab) {
  case 0:
    return "ESP, radar, sound";
  case 1:
    return "Aim, trigger, recoil";
  case 2:
    return "Crosshair, alerts";
  default:
    return "Theme, profiles, perf";
  }
}

void DrawAppFrame(ImDrawList *drawList, const ImVec2 &pos, const ImVec2 &size,
                  const ImVec4 &accent) {
  const ImU32 outer = ImGui::GetColorU32(ImVec4(0.06f, 0.07f, 0.09f, 0.98f));
  const ImU32 border = ImGui::GetColorU32(ImVec4(0.16f, 0.18f, 0.21f, 1.0f));
  const ImU32 glow = ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.08f));
  const ImU32 glowSoft = ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.03f));

  drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), outer, 10.0f);
  drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), border, 10.0f, 0, 1.0f);
  drawList->AddCircleFilled(ImVec2(pos.x + size.x * 0.33f, pos.y + 110.0f), 160.0f, glowSoft, 72);
  drawList->AddCircleFilled(ImVec2(pos.x + size.x * 0.74f, pos.y + size.y - 120.0f), 220.0f, glowSoft, 72);
  drawList->AddRectFilledMultiColor(
      ImVec2(pos.x + 1.0f, pos.y + 1.0f), ImVec2(pos.x + size.x - 1.0f, pos.y + 90.0f),
      ImGui::GetColorU32(ImVec4(0.10f, 0.12f, 0.16f, 0.65f)),
      ImGui::GetColorU32(ImVec4(0.10f, 0.12f, 0.16f, 0.18f)),
      ImGui::GetColorU32(ImVec4(0.06f, 0.07f, 0.09f, 0.04f)),
      ImGui::GetColorU32(ImVec4(0.06f, 0.07f, 0.09f, 0.35f)));
  drawList->AddCircleFilled(ImVec2(pos.x + size.x - 150.0f, pos.y + 70.0f), 90.0f, glow, 64);
}

void DrawTopBar(const Config::GlobalSettings &settings, const ImVec2 &contentSize) {
  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];
  const ImVec4 muted = style.Colors[ImGuiCol_TextDisabled];

  ImGui::BeginChild("MenuTopBar", ImVec2(0, 64), false, ImGuiWindowFlags_NoScrollbar);
  ImGui::SetCursorPos(ImVec2(8.0f, 4.0f));
  ImGui::TextColored(ImVec4(1, 1, 1, 1), "Visual Control");
  ImGui::SetCursorPos(ImVec2(8.0f, 24.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, muted);
  ImGui::TextUnformatted("Clean layout with live in-game style preview");
  ImGui::PopStyleColor();

  ImGui::SetCursorPos(ImVec2(contentSize.x - 272.0f, 10.0f));
  UI::StatChip("Theme", GetThemeName(settings.misc.menuTheme));
  ImGui::SameLine();
  UI::StatChip("Pacing", settings.performance.vsyncEnabled ? "VSync" : "Manual");

  ImGui::SetCursorPos(ImVec2(contentSize.x - 36.0f, 16.0f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.21f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.23f, 0.27f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.30f, 0.35f, 1.0f));
  if (ImGui::Button("X", ImVec2(28.0f, 28.0f))) {
    Menu::isOpen = false;
    RestoreOverlayTransparency();
  }
  ImGui::PopStyleColor(3);

  ImGui::SetCursorPos(ImVec2(0.0f, 63.0f));
  ImGui::Separator();
  ImGui::EndChild();
}

void DrawPreviewTabs() {
  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];
  const ImVec4 muted = style.Colors[ImGuiCol_TextDisabled];
  const char *labels[] = {"ENEMIES", "TEAMMATES"};

  for (int i = 0; i < 2; ++i) {
    if (i != 0) {
      ImGui::SameLine(0.0f, 20.0f);
    }
    if (ImGui::InvisibleButton(labels[i], ImVec2(84.0f, 20.0f))) {
      s_previewAffinity = i;
    }

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddText(min, ImGui::GetColorU32(i == s_previewAffinity ? accent : muted), labels[i]);
    if (i == s_previewAffinity) {
      drawList->AddLine(ImVec2(min.x, max.y + 4.0f), ImVec2(max.x, max.y + 4.0f),
                        ImGui::GetColorU32(accent), 2.0f);
    }
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

  drawList->AddCircleFilled(ImVec2(center.x, center.y - scale * 92.0f), scale * 22.0f, glow, 24);
  drawList->AddCircleFilled(ImVec2(center.x, center.y - scale * 94.0f), scale * 18.0f, body, 24);
  drawList->AddRectFilled(ImVec2(center.x - scale * 22.0f, center.y - scale * 64.0f),
                          ImVec2(center.x + scale * 22.0f, center.y + scale * 26.0f),
                          body, 16.0f);
  drawList->AddLine(ImVec2(center.x - scale * 18.0f, center.y - scale * 42.0f),
                    ImVec2(center.x - scale * 52.0f, center.y + scale * 10.0f), line, 3.0f);
  drawList->AddLine(ImVec2(center.x + scale * 18.0f, center.y - scale * 42.0f),
                    ImVec2(center.x + scale * 56.0f, center.y + scale * 8.0f), line, 3.0f);
  drawList->AddLine(ImVec2(center.x - scale * 8.0f, center.y + scale * 26.0f),
                    ImVec2(center.x - scale * 24.0f, center.y + scale * 98.0f), line, 3.0f);
  drawList->AddLine(ImVec2(center.x + scale * 8.0f, center.y + scale * 26.0f),
                    ImVec2(center.x + scale * 26.0f, center.y + scale * 98.0f), line, 3.0f);
  drawList->AddCircleFilled(ImVec2(center.x - scale * 64.0f, center.y + scale * 34.0f),
                            scale * 58.0f, glow, 36);
}

void DrawRectBox(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max,
                 const ImVec4 &color) {
  const ImU32 col = ImGui::GetColorU32(color);
  drawList->AddRect(min, max, col, 0.0f, 0, 1.8f);
  drawList->AddRect(ImVec2(min.x - 1.0f, min.y - 1.0f), ImVec2(max.x + 1.0f, max.y + 1.0f),
                    IM_COL32(0, 0, 0, 160), 0.0f, 0, 1.0f);
}

void DrawFilledBox(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max,
                   const ImVec4 &color, float alpha) {
  drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, alpha)),
                          0.0f);
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
    drawList->AddRectFilled(ImVec2(barMin.x, barMax.y - (barMax.y - barMin.y) * hpRatio), barMax,
                            IM_COL32(70, 225, 120, 255), 2.0f);
  }

  if (settings.esp.showName) {
    drawList->AddText(ImVec2(center.x - 42.0f, boxMin.y - 24.0f), IM_COL32(255, 255, 255, 255),
                      teammate ? "teammate_01" : "enemy_player");
  }

  if (settings.esp.showDistance) {
    drawList->AddText(ImVec2(center.x - 16.0f, boxMax.y + 10.0f), IM_COL32(138, 143, 152, 255),
                      teammate ? "24 m" : "1128 m");
  }

  if (settings.esp.showWeapon) {
    drawList->AddRectFilled(ImVec2(center.x - 36.0f, boxMax.y + 30.0f),
                            ImVec2(center.x + 36.0f, boxMax.y + 52.0f),
                            IM_COL32(24, 28, 34, 242), 6.0f);
    drawList->AddRect(ImVec2(center.x - 36.0f, boxMax.y + 30.0f),
                      ImVec2(center.x + 36.0f, boxMax.y + 52.0f),
                      ImGui::GetColorU32(boxColor), 6.0f, 0, 1.0f);
    drawList->AddText(ImVec2(center.x - 15.0f, boxMax.y + 35.0f),
                      IM_COL32(255, 255, 255, 235), teammate ? "m4a1" : "ak47");
  }

  if (settings.esp.showSnapLines) {
    const float *snap = settings.esp.snapLineColor;
    drawList->AddLine(ImVec2(center.x, boxMax.y + 72.0f), ImVec2(center.x, boxMax.y),
                      ImGui::GetColorU32(ImVec4(snap[0], snap[1], snap[2], snap[3])), 1.2f);
  }
}

void DrawPreviewPanel(const Config::GlobalSettings &settings) {
  ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 accent = style.Colors[ImGuiCol_ButtonActive];
  const ImVec4 muted = style.Colors[ImGuiCol_TextDisabled];
  const ImVec2 panelPos = ImGui::GetCursorScreenPos();
  const ImVec2 panelSize = ImGui::GetContentRegionAvail();
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  drawList->AddRectFilled(panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
                          ImGui::GetColorU32(ImVec4(0.10f, 0.11f, 0.14f, 1.0f)), 10.0f);
  drawList->AddRect(panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
                    ImGui::GetColorU32(ImVec4(0.16f, 0.18f, 0.21f, 1.0f)), 10.0f, 0, 1.0f);
  drawList->AddRectFilledMultiColor(
      panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
      ImGui::GetColorU32(ImVec4(0.05f, 0.08f, 0.12f, 0.15f)),
      ImGui::GetColorU32(ImVec4(0.05f, 0.08f, 0.12f, 0.04f)),
      ImGui::GetColorU32(ImVec4(0.00f, 0.00f, 0.00f, 0.08f)),
      ImGui::GetColorU32(ImVec4(0.00f, 0.00f, 0.00f, 0.14f)));

  for (float x = panelPos.x + 18.0f; x < panelPos.x + panelSize.x; x += 24.0f) {
    drawList->AddLine(ImVec2(x, panelPos.y + 48.0f), ImVec2(x, panelPos.y + panelSize.y - 18.0f),
                      ImGui::GetColorU32(ImVec4(0.14f, 0.16f, 0.20f, 0.22f)), 1.0f);
  }
  for (float y = panelPos.y + 64.0f; y < panelPos.y + panelSize.y; y += 24.0f) {
    drawList->AddLine(ImVec2(panelPos.x + 18.0f, y), ImVec2(panelPos.x + panelSize.x - 18.0f, y),
                      ImGui::GetColorU32(ImVec4(0.14f, 0.16f, 0.20f, 0.18f)), 1.0f);
  }

  ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 18.0f, ImGui::GetCursorPosY() + 14.0f));
  DrawPreviewTabs();

  const ImVec2 center(panelPos.x + panelSize.x * 0.50f, panelPos.y + panelSize.y * 0.58f);
  const float scale = (std::min)(panelSize.x, panelSize.y) / 340.0f;
  const ImVec2 boxMin(center.x - 70.0f * scale, center.y - 130.0f * scale);
  const ImVec2 boxMax(center.x + 70.0f * scale, center.y + 104.0f * scale);
  DrawPreviewFigure(drawList, center, scale, accent);

  drawList->AddText(ImVec2(boxMin.x - 56.0f, boxMin.y + 10.0f),
                    ImGui::GetColorU32(ImVec4(1.0f, 0.60f, 0.35f, 1.0f)), "BOMB");
  drawList->AddText(ImVec2(boxMax.x + 12.0f, boxMin.y + 36.0f),
                    ImGui::GetColorU32(ImVec4(0.00f, 0.83f, 1.00f, 1.0f)), "PLANTING");
  DrawPreviewEspOverlay(drawList, settings, center, scale);

  drawList->AddText(ImVec2(panelPos.x + 22.0f, panelPos.y + panelSize.y - 34.0f),
                    ImGui::GetColorU32(muted), "ESP PREVIEW");
}

void RenderSettingsColumn(int currentTab) {
  switch (currentTab) {
  case 0:
    RenderTabVisuals();
    break;
  case 1:
    RenderTabLegit();
    break;
  case 2:
    RenderTabMisc();
    break;
  default:
    RenderTabSettings();
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
  const ImVec4 muted = style.Colors[ImGuiCol_TextDisabled];

  ImGui::SetNextWindowSize(ImVec2(1340.0f, 820.0f), ImGuiCond_FirstUseEver);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 18));

  if (ImGui::Begin("CS2 External###MainWindow", &isOpen,
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                       ImGuiWindowFlags_NoScrollbar)) {
    ImGui::PopStyleVar();

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    DrawAppFrame(ImGui::GetWindowDrawList(), windowPos, windowSize, accent);
    DrawTopBar(settings, ImGui::GetContentRegionAvail());

    const float fullWidth = ImGui::GetContentRegionAvail().x;
    const float navWidth = fullWidth * 0.15f;
    const bool showPreview = s_currentTab == 0;
    const float previewWidth = showPreview ? fullWidth * 0.50f : 0.0f;
    const float settingsWidth =
        fullWidth - navWidth - (showPreview ? previewWidth + 24.0f : 12.0f);
    const float contentHeight = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("NavPanel", ImVec2(navWidth, contentHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::TextColored(ImVec4(1, 1, 1, 1), "Navigation");
    ImGui::Dummy(ImVec2(0, 8));

    for (int i = 0; i < 4; ++i) {
      if (UI::NavTile(GetTabLabel(i), GetTabHint(i), s_currentTab == i, 52.0f)) {
        s_currentTab = i;
      }
      ImGui::Dummy(ImVec2(0, 6));
    }

    ImGui::Dummy(ImVec2(0, 14));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 12));
    ImGui::TextColored(muted, "Shortcuts");
    ImGui::TextUnformatted("INSERT");
    ImGui::TextUnformatted("END");
    ImGui::EndChild();

    if (showPreview) {
      ImGui::SameLine(0.0f, 12.0f);
      ImGui::BeginChild("PreviewPanel", ImVec2(previewWidth, contentHeight), false,
                        ImGuiWindowFlags_NoScrollbar);
      DrawPreviewPanel(settings);
      ImGui::EndChild();
      ImGui::SameLine(0.0f, 12.0f);
    } else {
      ImGui::SameLine(0.0f, 12.0f);
    }

    ImGui::BeginChild("SettingsPanel", ImVec2(settingsWidth, contentHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", GetTabLabel(s_currentTab));
    ImGui::SameLine();
    ImGui::TextColored(muted, "| %s", GetTabHint(s_currentTab));
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 8));
    RenderSettingsColumn(s_currentTab);
    ImGui::EndChild();
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
