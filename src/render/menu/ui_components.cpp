#include "ui_components.h"
#include "config/config_manager.h"
#include <imgui_internal.h>
#include <cstdio>
#include <windows.h>

static const char *KeyLabel(int vk) {
  switch (vk) {
  case 0x01: return "LMB";
  case 0x02: return "RMB";
  case 0x04: return "MMB";
  case 0x05: return "Mouse4"; // XButton1
  case 0x06: return "Mouse5"; // XButton2
  case 0x10: return "Shift";
  case 0x11: return "Ctrl";
  case 0x12: return "Alt";
  case VK_INSERT: return "Insert";
  case VK_END: return "End";
  case 'Z': return "Z";
  case 'X': return "X";
  case 'C': return "C";
  default: {
    static char buf[8];
    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
  }
  }
}

namespace UI {

bool BeginCard(const char *title, ImVec2 size) {
  bool ret = ImGui::BeginChild(title, size, true);
  ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered], title);
  ImGui::Separator();
  return ret;
}

void EndCard() {
  ImGui::EndChild();
}

bool SettingToggle(const char *label, bool *v) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

  float height = ImGui::GetFrameHeight();
  float width = height * 1.55f;
  float radius = height * 0.50f;

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(
      pos, ImVec2(pos.x + width + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
                  pos.y + height));

  ImGui::ItemSize(total_bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(total_bb, id)) return false;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
  if (pressed) {
    *v = !(*v);
    ImGui::MarkItemEdited(id);
    Config::ConfigManager::ApplySettings();
  }

  float t = *v ? 1.0f : 0.0f;
  float *t_anim = (float *)ImGui::GetStateStorage()->GetVoidPtr(id);
  if (!t_anim) {
    t_anim = (float *)ImGui::MemAlloc(sizeof(float));
    *t_anim = t;
    ImGui::GetStateStorage()->SetVoidPtr(id, t_anim);
  }

  float ANIM_SPEED = g.IO.DeltaTime * 12.0f;
  if (*t_anim != t) {
    *t_anim += (*v ? 1.0f : -1.0f) * ANIM_SPEED;
    if (*t_anim < 0.0f) *t_anim = 0.0f;
    if (*t_anim > 1.0f) *t_anim = 1.0f;
  }

  ImU32 col_bg;
  ImVec4 col_off = style.Colors[ImGuiCol_FrameBg];
  ImVec4 col_on = style.Colors[ImGuiCol_Button];
  if (hovered) {
    col_off = style.Colors[ImGuiCol_FrameBgHovered];
    col_on = style.Colors[ImGuiCol_ButtonHovered];
  }

  ImU32 bg_color = ImGui::GetColorU32(ImLerp(col_off, col_on, *t_anim));
  window->DrawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bg_color, height * 0.5f);
  window->DrawList->AddCircleFilled(
      ImVec2(pos.x + radius + (*t_anim) * (width - radius * 2.0f), pos.y + radius),
      radius - 1.5f, IM_COL32(255, 255, 255, 255));

  if (label_size.x > 0.0f) {
    ImGui::RenderText(ImVec2(pos.x + width + style.ItemInnerSpacing.x, pos.y + style.FramePadding.y), label);
  }

  return pressed;
}

bool SettingColor(const char *label, float *color) {
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 1.5f);
  bool ret = ImGui::ColorEdit4(
      label, color,
      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf);
  if (ret) Config::ConfigManager::ApplySettings();
  return ret;
}

bool SettingHotkey(const char* label, int& keyTarget) {
  static int *s_listening = nullptr;
  bool changed = false;

  char btnLabel[64];
  snprintf(btnLabel, sizeof(btnLabel), "%s  [%s]", label, KeyLabel(keyTarget));

  bool listening = (s_listening == &keyTarget);

  if (listening) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.0f, 1.0f));
    if (ImGui::Button("... Press key ...", ImVec2(180, 0))) {
      s_listening = nullptr;
    }
    ImGui::PopStyleColor();

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
      s_listening = nullptr;
    } else {
      static const int candidateVKs[] = {
          0x01, 0x02, 0x04, 0x05, 0x06, VK_SHIFT, VK_CONTROL, VK_MENU, VK_INSERT, VK_END, VK_HOME, VK_DELETE, 
          VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, 
          VK_F10, VK_F11, VK_F12, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'Z', 'X', 'C', 'V', 'F', 
          'G', 'H', 'R', 'T', 'B',
      };
      for (int v : candidateVKs) {
        if (GetAsyncKeyState(v) & 0x8000) {
          keyTarget = v;
          s_listening = nullptr;
          changed = true;
          Config::ConfigManager::ApplySettings();
          break;
        }
      }
    }
  } else {
    if (ImGui::Button(btnLabel, ImVec2(180, 0)))
      s_listening = &keyTarget;
  }
  return changed;
}

} // namespace UI
