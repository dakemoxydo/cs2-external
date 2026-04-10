#include "menu.h"
#include "../overlay/overlay.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include "features/feature_manager.h"
#include "menu_theme.h"
#include "tab_legit.h"
#include "tab_visuals.h"
#include "tab_misc.h"
#include "tab_settings.h"
#include "ui_components.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <windows.h>

namespace Render {

bool Menu::isOpen = false;
bool Menu::shouldClose = false;

static int s_currentTab = 0;
static int s_lastAppliedTheme = -1;

void Menu::Render() {
    if (!isOpen) return;

    long ex = GetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE);
    if (ex & WS_EX_TRANSPARENT)
        SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE, ex & ~WS_EX_TRANSPARENT);

    if (Config::Settings.misc.menuTheme != s_lastAppliedTheme) {
        ApplyMenuTheme(Config::Settings.misc.menuTheme);
        s_lastAppliedTheme = Config::Settings.misc.menuTheme;
    }

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 accent = style.Colors[ImGuiCol_Button];
    ImVec4 accentHover = style.Colors[ImGuiCol_ButtonHovered];

    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::Begin("CS2 External###MainWindow", &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::PopStyleVar(2);

        // ─── КАСТОМНЫЙ ЗАГОЛОВОК ───
        ImGui::BeginChild("Header", ImVec2(0, 48), false, ImGuiWindowFlags_NoScrollbar);
        {
            ImGuiStyle &headerStyle = ImGui::GetStyle();
            ImVec2 headerPos = ImGui::GetCursorScreenPos();
            ImGuiWindow *window = ImGui::GetCurrentWindow();

            window->DrawList->AddRectFilled(
                headerPos,
                ImVec2(headerPos.x + ImGui::GetWindowWidth(), headerPos.y + 48),
                ImGui::GetColorU32(ImVec4(
                    style.Colors[ImGuiCol_ChildBg].x * 0.85f + accent.x * 0.08f,
                    style.Colors[ImGuiCol_ChildBg].y * 0.85f + accent.y * 0.08f,
                    style.Colors[ImGuiCol_ChildBg].z * 0.85f + accent.z * 0.08f,
                    1.0f)),
                0.0f);

            window->DrawList->AddLine(
                ImVec2(headerPos.x, headerPos.y + 47),
                ImVec2(headerPos.x + ImGui::GetWindowWidth(), headerPos.y + 47),
                ImGui::GetColorU32(accent), 2.0f);

            ImGui::SetCursorPos(ImVec2(16, 12));
            ImGui::TextColored(accentHover, "CS2");
            ImGui::SameLine(0, 2);
            ImGui::TextColored(style.Colors[ImGuiCol_Text], "External");

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

        // ─── SIDEBAR ───
        ImGui::BeginChild("Sidebar", ImVec2(180, 0), true);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::Dummy(ImVec2(0, 12));

        const char *tabNames[] = {"LEGIT", "VISUALS", "MISC", "SETTINGS"};
        for (int i = 0; i < 4; i++) {
            bool isSelected = (s_currentTab == i);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, accent);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accentHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, accent);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            if (ImGui::Selectable(tabNames[i], isSelected, 0, ImVec2(0, 40)))
                s_currentTab = i;
            if (isSelected) ImGui::PopStyleColor(4);
        }

        ImGui::PopStyleVar(3);
        ImGui::EndChild();

        ImGui::SameLine(0, 10);

        // ─── CONTENT ───
        ImGui::BeginChild("Content", ImVec2(0, 0), false, 0);
        ImGui::Dummy(ImVec2(0, 5));

        switch (s_currentTab) {
            case 0: RenderTabLegit(); break;
            case 1: RenderTabVisuals(); break;
            case 2: RenderTabMisc(); break;
            case 3: RenderTabSettings(); break;
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
        SetWindowLong(Render::Overlay::GetWindowHandle(), GWL_EXSTYLE, ex | WS_EX_TRANSPARENT);
    }
}

} // namespace Render
