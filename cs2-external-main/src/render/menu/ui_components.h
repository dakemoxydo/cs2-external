#pragma once
#include <imgui.h>

namespace UI {
bool BeginCard(const char *title, ImVec2 size = ImVec2(0, 0));
void EndCard();
bool SettingToggle(const char *label, bool *value);
bool SettingColor(const char *label, float *color);
bool SettingHotkey(const char *label, int &keyTarget);
} // namespace UI
