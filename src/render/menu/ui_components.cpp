#include "ui_components.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <windows.h>

static const char *KeyLabel(int vk) {
  switch (vk) {
  case 0x01: return "LMB";
  case 0x02: return "RMB";
  case 0x04: return "MMB";
  case 0x05: return "Mouse4"; // XButton1
  case 0x06: return "Mouse5"; // XButton2
  case VK_LSHIFT: return "Shift";
  case VK_RSHIFT: return "Shift";
  case VK_LCONTROL: return "Ctrl";
  case VK_RCONTROL: return "Ctrl";
  case VK_LMENU: return "Alt";
  case VK_RMENU: return "Alt";
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

namespace {

void DrawNavIcon(ImDrawList *drawList, const ImVec2 &center, const ImVec4 &color,
                 int iconKind) {
  const ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
  switch (iconKind) {
  case 0:
    drawList->AddCircle(center, 7.0f, col, 20, 1.4f);
    drawList->AddLine(ImVec2(center.x - 6.0f, center.y + 6.0f),
                      ImVec2(center.x + 6.0f, center.y - 6.0f), col, 1.4f);
    break;
  case 1:
    drawList->AddRect(ImVec2(center.x - 6.0f, center.y - 5.0f),
                      ImVec2(center.x + 6.0f, center.y + 5.0f), col, 3.0f, 0, 1.4f);
    drawList->AddLine(ImVec2(center.x, center.y - 7.0f),
                      ImVec2(center.x, center.y + 7.0f), col, 1.4f);
    break;
  case 2:
    drawList->AddRect(ImVec2(center.x - 7.0f, center.y - 7.0f),
                      ImVec2(center.x + 7.0f, center.y + 7.0f), col, 3.0f, 0, 1.4f);
    drawList->AddLine(ImVec2(center.x - 4.0f, center.y),
                      ImVec2(center.x + 4.0f, center.y), col, 1.4f);
    drawList->AddLine(ImVec2(center.x, center.y - 4.0f),
                      ImVec2(center.x, center.y + 4.0f), col, 1.4f);
    break;
  default:
    drawList->AddRect(ImVec2(center.x - 6.5f, center.y - 6.5f),
                      ImVec2(center.x + 6.5f, center.y + 6.5f), col, 3.0f, 0, 1.4f);
    drawList->AddCircle(center, 2.0f, col, 12, 1.4f);
    break;
  }
}

int IconKindForTitle(const char *title) {
  if (!title) return 0;
  if (strcmp(title, "Players") == 0) return 0;
  if (strcmp(title, "Combat") == 0) return 1;
  if (strcmp(title, "Utility") == 0) return 2;
  return 3;
}

} // namespace

bool NavTile(const char *title, const char *description, bool selected, float height) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(title);

  ImVec2 pos = window->DC.CursorPos;
  ImVec2 avail = ImGui::GetContentRegionAvail();
  ImRect bb(pos, ImVec2(pos.x + avail.x, pos.y + height));
  ImGui::ItemSize(bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, id)) return false;

  bool hovered = false;
  bool held = false;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  ImVec4 accent = style.Colors[ImGuiCol_Button];
  ImVec4 accentHover = style.Colors[ImGuiCol_ButtonHovered];
  ImVec4 text = style.Colors[ImGuiCol_Text];
  ImVec4 muted = style.Colors[ImGuiCol_TextDisabled];

  float anim = selected ? 1.0f : 0.0f;
  float hoverAnim = hovered ? 1.0f : 0.0f;
  ImGuiStorage *storage = ImGui::GetStateStorage();
  ImGuiID animId = id ^ 0x91E10D4F;
  ImGuiID hoverId = id ^ 0x41A0F1B1;
  float currentAnim = storage->GetFloat(animId, anim);
  float currentHover = storage->GetFloat(hoverId, hoverAnim);
  float dt = g.IO.DeltaTime * 12.0f;
  currentAnim = ImLerp(currentAnim, anim, std::clamp(dt, 0.0f, 1.0f));
  currentHover = ImLerp(currentHover, hoverAnim, std::clamp(dt, 0.0f, 1.0f));
  storage->SetFloat(animId, currentAnim);
  storage->SetFloat(hoverId, currentHover);

  ImU32 bg = ImGui::GetColorU32(ImVec4(
      style.Colors[ImGuiCol_ChildBg].x * 0.92f + accent.x * (0.04f + 0.12f * currentAnim + 0.05f * currentHover),
      style.Colors[ImGuiCol_ChildBg].y * 0.92f + accent.y * (0.04f + 0.12f * currentAnim + 0.05f * currentHover),
      style.Colors[ImGuiCol_ChildBg].z * 0.92f + accent.z * (0.04f + 0.12f * currentAnim + 0.05f * currentHover),
      1.0f));

  ImVec2 pad(12.0f, 10.0f);
  window->DrawList->AddRectFilled(bb.Min, bb.Max, bg, 9.0f);
  window->DrawList->AddRect(
      bb.Min, bb.Max,
      ImGui::GetColorU32(ImVec4(style.Colors[ImGuiCol_Border].x,
                                style.Colors[ImGuiCol_Border].y,
                                style.Colors[ImGuiCol_Border].z,
                                selected ? 0.95f : 0.75f)),
      9.0f, 0, 1.0f);
  if (selected || hovered) {
    window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.20f + 0.20f * currentAnim)), 9.0f, 0, 1.0f);
  }

  window->DrawList->AddRectFilled(
      ImVec2(bb.Min.x + 1.0f, bb.Min.y + 6.0f),
      ImVec2(bb.Min.x + 4.0f, bb.Max.y - 6.0f),
      ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.20f + 0.55f * currentAnim)), 1.0f);

  const ImVec2 iconCenter(bb.Min.x + 22.0f, (bb.Min.y + bb.Max.y) * 0.5f);
  DrawNavIcon(window->DrawList, iconCenter,
              selected ? accentHover : ImVec4(muted.x, muted.y, muted.z, 0.95f),
              IconKindForTitle(title));

  ImGui::PushStyleColor(ImGuiCol_Text, selected ? ImVec4(text.x, text.y, text.z, 1.0f) : ImVec4(text.x, text.y, text.z, 0.92f));
  ImGui::SetCursorScreenPos(ImVec2(bb.Min.x + 40.0f, bb.Min.y + pad.y - 1.0f));
  ImGui::TextUnformatted(title);
  if (description && description[0] != '\0') {
    ImGui::PushStyleColor(ImGuiCol_Text, muted);
    ImGui::SetCursorScreenPos(ImVec2(bb.Min.x + 40.0f, bb.Min.y + pad.y + 20.0f));
    ImGui::TextWrapped("%s", description);
    ImGui::PopStyleColor();
  }
  ImGui::PopStyleColor();

  return pressed;
}

void StatChip(const char *label, const char *value) {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 accent = style.Colors[ImGuiCol_Button];
  ImVec2 textSize = ImGui::CalcTextSize(value);
  float width = std::max(120.0f, textSize.x + 28.0f);

  ImGui::BeginGroup();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(
      style.Colors[ImGuiCol_ChildBg].x * 0.90f + accent.x * 0.04f,
      style.Colors[ImGuiCol_ChildBg].y * 0.90f + accent.y * 0.04f,
      style.Colors[ImGuiCol_ChildBg].z * 0.90f + accent.z * 0.04f,
      1.0f));
  ImGui::BeginChild(label, ImVec2(width, 38.0f), true, ImGuiWindowFlags_NoScrollbar);
  ImGui::SetCursorPos(ImVec2(12.0f, 7.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
  ImGui::TextUnformatted(label);
  ImGui::PopStyleColor();
  ImGui::SetCursorPos(ImVec2(12.0f, 20.0f));
  ImGui::TextUnformatted(value);
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::EndGroup();
}

bool ColorRow(const char *label, float *color) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::SameLine();
  ImGui::PushID(label);
  bool changed = SettingColor("##color", color);
  ImGui::PopID();
  return changed;
}

bool BeginCard(const char *title, ImVec2 size) {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 accent = style.Colors[ImGuiCol_Button];
  ImVec4 accentHover = style.Colors[ImGuiCol_ButtonHovered];

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(
      style.Colors[ImGuiCol_ChildBg].x * 0.85f + accent.x * 0.02f,
      style.Colors[ImGuiCol_ChildBg].y * 0.85f + accent.y * 0.02f,
      style.Colors[ImGuiCol_ChildBg].z * 0.85f + accent.z * 0.02f,
      style.Colors[ImGuiCol_ChildBg].w
  ));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));

  bool ret = ImGui::BeginChild(title, size, true, ImGuiWindowFlags_NoScrollbar);

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();

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

void SectionHeader(const char *title, const char *description) {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 accent = style.Colors[ImGuiCol_ButtonHovered];

  ImGui::Spacing();
  ImGui::TextColored(accent, "%s", title);
  if (description && description[0] != '\0') {
    ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(description);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
  }
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

void HelpText(const char *text) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
  ImGui::PushTextWrapPos(0.0f);
  ImGui::TextUnformatted(text);
  ImGui::PopTextWrapPos();
  ImGui::PopStyleColor();
}

bool SettingToggle(const char *label, bool *v) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

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

  ImU32 bg_color = ImGui::GetColorU32(ImLerp(col_off, col_on, t_anim));
  window->DrawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bg_color, height * 0.5f);
  window->DrawList->AddRect(
      pos, ImVec2(pos.x + width, pos.y + height),
      ImGui::GetColorU32(ImVec4(style.Colors[ImGuiCol_Border].x,
                                style.Colors[ImGuiCol_Border].y,
                                style.Colors[ImGuiCol_Border].z, 1.0f)),
      height * 0.5f, 0, 1.0f);

  if (hovered) {
    window->DrawList->AddRect(pos, ImVec2(pos.x + width, pos.y + height),
        ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.15f)), height * 0.5f, 0, 1.0f);
  }

  float circle_x = pos.x + radius + t_anim * (width - radius * 2.0f);
  float circle_y = pos.y + radius;
  float circle_radius = radius - 2.0f;

  window->DrawList->AddCircleFilled(
      ImVec2(circle_x + 1.0f, circle_y + 1.5f),
      circle_radius, IM_COL32(0, 0, 0, 60));

  window->DrawList->AddCircleFilled(
      ImVec2(circle_x, circle_y),
      circle_radius, IM_COL32(255, 255, 255, 255));

  window->DrawList->AddCircleFilled(
      ImVec2(circle_x, circle_y - 1.0f),
      circle_radius * 0.6f, IM_COL32(255, 255, 255, 40));

  if (label_size.x > 0.0f) {
    ImGui::RenderText(ImVec2(pos.x + width + style.ItemInnerSpacing.x, pos.y + style.FramePadding.y), label);
  }

  return pressed;
}

bool SettingColor(const char *label, float *color) {
  float pickerSize = ImGui::GetFrameHeight() * 1.8f;
  float cursorX = ImGui::GetCursorPosX();
  float contentWidth = ImGui::GetContentRegionAvail().x;
  float targetX = contentWidth - pickerSize;
  if (targetX > cursorX) {
    ImGui::SetCursorPosX(targetX);
  }
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
  bool ret = ImGui::ColorEdit4(
      label, color,
      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf);
  ImGui::PopStyleVar(2);
  return ret;
}

bool SettingHotkey(const char* label, int& keyTarget) {
  static int s_listeningId = -1;
  bool changed = false;

  ImGuiStyle &style = ImGui::GetStyle();

  char btnLabel[64];
  snprintf(btnLabel, sizeof(btnLabel), "%s", label);

  ImGuiID widgetId = ImGui::GetID(label);
  int myId = static_cast<int>(widgetId & 0x7FFFFFFF);

  bool listening = (s_listeningId == myId);

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
      s_listeningId = -1;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // NOTE: Direct GetAsyncKeyState() here is intentional and safe.
    // This is a UI-only hotkey picker — it needs live polling of all
    // VK codes, which is outside the scope of InputManager::keyStates[]
    // (that array only contains feature-relevant keys). Runs on render
    // thread, no cross-thread races.
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
      s_listeningId = -1;
    } else {
      static const int candidateVKs[] = {
          0x01, 0x02, 0x04, 0x05, 0x06, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU, VK_INSERT, VK_END, VK_HOME, VK_DELETE,
          VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9,
          VK_F10, VK_F11, VK_F12, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'Z', 'X', 'C', 'V', 'F',
          'G', 'H', 'R', 'T', 'B',
      };
      for (int v : candidateVKs) {
        if (GetAsyncKeyState(v) & 0x8000) {
          keyTarget = v;
          s_listeningId = -1;
          changed = true;
          break;
        }
      }
    }
  } else {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    char keyLabel[32];
    snprintf(keyLabel, sizeof(keyLabel), "[ %s ]", KeyLabel(keyTarget));
    if (ImGui::Button(keyLabel, ImVec2(100, 0)))
      s_listeningId = myId;
    ImGui::PopStyleVar();
  }
  return changed;
}

} // namespace UI
