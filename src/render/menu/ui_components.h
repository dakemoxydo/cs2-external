#pragma once
#include <imgui.h>

namespace UI {
bool BeginCard(const char *title, ImVec2 size = ImVec2(0, 0));
void EndCard();
void SectionHeader(const char *title, const char *description = nullptr);
void HelpText(const char *text);
bool NavTile(const char *title, const char *description, bool selected, float height = 64.0f);
void StatChip(const char *label, const char *value);
bool ColorRow(const char *label, float *color);
bool SettingToggle(const char *label, bool *value);
bool SettingColor(const char *label, float *color);
bool SettingHotkey(const char *label, int &keyTarget);
} // namespace UI
