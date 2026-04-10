#include "menu.h"
#include "../overlay/overlay.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "features/feature_manager.h"
#include "menu_theme.h"
#include "tab_legit.h"
#include "tab_misc.h"
#include "tab_settings.h"
#include "tab_visuals.h"
#include "ui_components.h"
#include <imgui.h>
#include <windows.h>

namespace Render {

bool Menu::isOpen = false;
bool Menu::shouldClose = false;

static int s_currentTab = 0;
static int s_lastAppliedTheme = -1;

namespace {

void RestoreOverlayTransparency() {
    HWND hwnd = Render::Overlay::GetWindowHandle();
    if (!hwnd) {
        return;
    }

    long ex = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, ex | WS_EX_TRANSPARENT);
}

}

void Menu::Render() {
    if (!isOpen) {
        return;
    }

    if (Config::Settings.misc.menuTheme < 0 || Config::Settings.misc.menuTheme > 6) {
        Config::Settings.misc.menuTheme = 0;
    }

    HWND overlayWindow = Render::Overlay::GetWindowHandle();
    if (overlayWindow) {
        long ex = GetWindowLong(overlayWindow, GWL_EXSTYLE);
        if (ex & WS_EX_TRANSPARENT) {
            SetWindowLong(overlayWindow, GWL_EXSTYLE, ex & ~WS_EX_TRANSPARENT);
        }
    }

    if (Config::Settings.misc.menuTheme != s_lastAppliedTheme) {
        ApplyMenuTheme(Config::Settings.misc.menuTheme);
        s_lastAppliedTheme = Config::Settings.misc.menuTheme;
    }

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 accent = style.Colors[ImGuiCol_Button];
    ImVec4 accentHover = style.Colors[ImGuiCol_ButtonHovered];
    ImVec4 muted = style.Colors[ImGuiCol_TextDisabled];

    const char *tabNames[] = {"Legit", "Visuals", "Misc", "Settings"};
    const char *tabHints[] = {
        "Aim, trigger, and recoil controls.",
        "ESP, radar, bomb, and spatial overlays.",
        "Crosshair and utility helpers.",
        "Profiles, offsets, performance, and shutdown."
    };
    const char *contentTitles[] = {
        "Legit Control",
        "Visual Intelligence",
        "Utility Tools",
        "System Configuration"
    };
    const char *contentDescriptions[] = {
        "Tune target acquisition, recoil handling, and reaction timing.",
        "Organize player information, world alerts, and overlay visibility.",
        "Keep small helpers isolated from combat and visuals settings.",
        "Manage configs, offset refresh, performance limits, and exit behavior."
    };

    ImGui::SetNextWindowSize(ImVec2(1120.0f, 720.0f), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("CS2 External###MainWindow", &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::PopStyleVar(2);

        ImGui::BeginChild("Header", ImVec2(0, 72), false, ImGuiWindowFlags_NoScrollbar);
        {
            ImVec2 headerPos = ImGui::GetCursorScreenPos();
            float headerWidth = ImGui::GetWindowWidth();
            ImDrawList *drawList = ImGui::GetWindowDrawList();

            drawList->AddRectFilled(
                headerPos,
                ImVec2(headerPos.x + headerWidth, headerPos.y + 72.0f),
                ImGui::GetColorU32(ImVec4(
                    style.Colors[ImGuiCol_ChildBg].x * 0.92f + accent.x * 0.05f,
                    style.Colors[ImGuiCol_ChildBg].y * 0.92f + accent.y * 0.05f,
                    style.Colors[ImGuiCol_ChildBg].z * 0.92f + accent.z * 0.05f,
                    1.0f)));
            drawList->AddLine(
                ImVec2(headerPos.x, headerPos.y + 71.0f),
                ImVec2(headerPos.x + headerWidth, headerPos.y + 71.0f),
                ImGui::GetColorU32(accent),
                2.0f);

            ImGui::SetCursorPos(ImVec2(18.0f, 10.0f));
            ImGui::TextColored(accentHover, "CS2 Overlay");

            ImGui::SetCursorPos(ImVec2(18.0f, 36.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, muted);
            ImGui::TextUnformatted("External control surface for combat logic, visuals, configs, and runtime diagnostics.");
            ImGui::PopStyleColor();

            const char *statusText = Config::Settings.performance.vsyncEnabled ? "VSync linked" : "Manual FPS";
            ImVec2 statusTextSize = ImGui::CalcTextSize(statusText);
            ImVec2 pillMin(headerWidth - 196.0f, 18.0f);
            ImVec2 pillMax(pillMin.x + statusTextSize.x + 26.0f, pillMin.y + 24.0f);
            drawList->AddRectFilled(pillMin, pillMax, ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.18f)), 10.0f);
            drawList->AddRect(pillMin, pillMax, ImGui::GetColorU32(ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.40f)), 10.0f, 0, 1.0f);
            drawList->AddText(ImVec2(pillMin.x + 12.0f, pillMin.y + 4.0f), ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), statusText);

            ImGui::SetCursorPos(ImVec2(headerWidth - 46.0f, 18.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.14f, 0.14f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
            if (ImGui::Button("X", ImVec2(28.0f, 28.0f))) {
                isOpen = false;
                Config::Settings.menuIsOpen = false;
                RestoreOverlayTransparency();
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
        }
        ImGui::EndChild();

        ImGui::BeginChild("Sidebar", ImVec2(220.0f, 0), true);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

        ImGui::Dummy(ImVec2(0, 8));
        ImGui::TextColored(accentHover, "Navigation");
        ImGui::PushStyleColor(ImGuiCol_Text, muted);
        ImGui::TextWrapped("Tabs are grouped by intent so related options live together.");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 8));

        for (int i = 0; i < 4; ++i) {
            bool selected = s_currentTab == i;
            ImGui::PushStyleColor(ImGuiCol_Header, selected ? ImVec4(accent.x, accent.y, accent.z, 0.34f) : ImVec4(1, 1, 1, 0.03f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, selected ? ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.44f) : ImVec4(accent.x, accent.y, accent.z, 0.14f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(accent.x, accent.y, accent.z, 0.52f));
            if (ImGui::Selectable(tabNames[i], selected, 0, ImVec2(0, 48))) {
                s_currentTab = i;
            }
            ImGui::PopStyleColor(3);

            ImGui::PushStyleColor(ImGuiCol_Text, muted);
            ImGui::TextWrapped("%s", tabHints[i]);
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));
        }

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::TextColored(accentHover, "Shortcuts");
        ImGui::PushStyleColor(ImGuiCol_Text, muted);
        ImGui::TextUnformatted("INSERT  - Toggle menu");
        ImGui::TextUnformatted("END     - Exit overlay");
        ImGui::PopStyleColor();

        ImGui::PopStyleVar(3);
        ImGui::EndChild();

        ImGui::SameLine(0, 12.0f);

        ImGui::BeginChild("Content", ImVec2(0, 0), false);
        ImGui::Dummy(ImVec2(0, 8));
        UI::SectionHeader(contentTitles[s_currentTab], contentDescriptions[s_currentTab]);

        switch (s_currentTab) {
        case 0: RenderTabLegit(); break;
        case 1: RenderTabVisuals(); break;
        case 2: RenderTabMisc(); break;
        case 3: RenderTabSettings(); break;
        default: break;
        }

        ImGui::EndChild();
    } else {
        ImGui::PopStyleVar(2);
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
    Config::Settings.menuIsOpen = isOpen;
    if (!isOpen) {
        RestoreOverlayTransparency();
    }
}

} // namespace Render
