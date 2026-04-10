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
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 accent = style.Colors[ImGuiCol_Button];
  ImVec4 accentHover = style.Colors[ImGuiCol_ButtonHovered];

  // Фоновый градиент-эффект через полупрозрачный оверлей
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(
      style.Colors[ImGuiCol_ChildBg].x * 0.85f + accent.x * 0.02f,
      style.Colors[ImGuiCol_ChildBg].y * 0.85f + accent.y * 0.02f,
      style.Colors[ImGuiCol_ChildBg].z * 0.85f + accent.z * 0.02f,
      style.Colors[ImGuiCol_ChildBg].w
  ));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));

  bool ret = ImGui::BeginChild(title, size, true);

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();

  // Заголовок карточки с акцентной полоской слева
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  ImVec2 pos = window->DC.CursorPos;
  window->DrawList->AddRectFilled(
      ImVec2(pos.x, pos.y + 2.0f),
      ImVec2(pos.x + 3.0f, pos.y + ImGui::GetTextLineHeight() + 6.0f),
      ImGui::GetColorU32(accent), 1.5f);

  ImGui::SetCursorPosX(pos.x + 10.0f);
  ImGui::TextColored(ImVec4(
      accentHover.x * 0.7f + style.Colors[ImGuiCol_Text].x * 0.3f,
      accentHover.y * 0.7f + style.Colors[ImGuiCol_Text].y * 0.3f,
      accentHover.z * 0.7f + style.Colors[ImGuiCol_Text].z * 0.3f,
      1.0f), title);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  return ret;
}

void EndCard() {
  ImGui::Spacing();
  ImGui::EndChild();
}

bool SettingToggle(const char *label, bool *v) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

  // Увеличенный размер переключателя
  float height = 22.0f;
  float width = height * 1.8f;
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
  float t_anim;
  ImGuiStorage* storage = ImGui::GetStateStorage();
  bool found;
  t_anim = storage->GetFloat(id, t);
  found = storage->GetInt(id, 0) != 0;
  if (!found) {
    t_anim = t;
    storage->SetFloat(id, t_anim);
    storage->SetInt(id, 1);
  }

  // Более плавная анимация
  float ANIM_SPEED = g.IO.DeltaTime * 10.0f;
  if (t_anim != t) {
    t_anim += (*v ? 1.0f : -1.0f) * ANIM_SPEED;
    if (t_anim < 0.0f) t_anim = 0.0f;
    if (t_anim > 1.0f) t_anim = 1.0f;
    storage->SetFloat(id, t_anim);
  }

  ImVec4 col_off = style.Colors[ImGuiCol_FrameBg];
  ImVec4 col_on = style.Colors[ImGuiCol_Button];
  if (hovered) {
    col_off = style.Colors[ImGuiCol_FrameBgHovered];
    col_on = style.Colors[ImGuiCol_ButtonHovered];
  }

  // Фон с тенью
  ImU32 bg_color = ImGui::GetColorU32(ImLerp(col_off, col_on, t_anim));
  window->DrawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bg_color, height * 0.5f);

  // Обводка при наведении
  if (hovered) {
    window->DrawList->AddRect(pos, ImVec2(pos.x + width, pos.y + height),
        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.15f)), height * 0.5f, 0, 1.0f);
  }

  // Круглый переключатель с тенью
  float circle_x = pos.x + radius + t_anim * (width - radius * 2.0f);
  float circle_y = pos.y + radius;
  float circle_radius = radius - 2.0f;

  // Тень кружка
  window->DrawList->AddCircleFilled(
      ImVec2(circle_x + 1.0f, circle_y + 1.5f),
      circle_radius, IM_COL32(0, 0, 0, 60));

  // Основной кружок
  window->DrawList->AddCircleFilled(
      ImVec2(circle_x, circle_y),
      circle_radius, IM_COL32(255, 255, 255, 255));

  // Внутренний градиент на кружке
  window->DrawList->AddCircleFilled(
      ImVec2(circle_x, circle_y - 1.0f),
      circle_radius * 0.6f, IM_COL32(255, 255, 255, 40));

  if (label_size.x > 0.0f) {
    ImGui::RenderText(ImVec2(pos.x + width + style.ItemInnerSpacing.x, pos.y + style.FramePadding.y), label);
  }

  return pressed;
}

bool SettingColor(const char *label, float *color) {
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 1.8f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
  bool ret = ImGui::ColorEdit4(
      label, color,
      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf);
  ImGui::PopStyleVar(2);
  if (ret) Config::ConfigManager::ApplySettings();
  return ret;
}

bool SettingHotkey(const char* label, int& keyTarget) {
  static int *s_listening = nullptr;
  bool changed = false;

  ImGuiStyle &style = ImGui::GetStyle();

  char btnLabel[64];
  snprintf(btnLabel, sizeof(btnLabel), "%s", label);

  bool listening = (s_listening == &keyTarget);

  // Метка
  ImGui::TextUnformatted(btnLabel);
  ImGui::SameLine();

  if (listening) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(
        style.Colors[ImGuiCol_Button].x * 0.5f + 0.8f * 0.5f,
        style.Colors[ImGuiCol_Button].y * 0.5f + 0.3f * 0.5f,
        style.Colors[ImGuiCol_Button].z * 0.5f + 0.0f * 0.5f,
        1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.35f, 0.05f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("... Press key ...", ImVec2(160, 0))) {
      s_listening = nullptr;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

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
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    char keyLabel[32];
    snprintf(keyLabel, sizeof(keyLabel), "[ %s ]", KeyLabel(keyTarget));
    if (ImGui::Button(keyLabel, ImVec2(100, 0)))
      s_listening = &keyTarget;
    ImGui::PopStyleVar();
  }
  return changed;
}

} // namespace UI
