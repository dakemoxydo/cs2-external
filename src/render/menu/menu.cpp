#include "menu.h"
#include "../overlay/overlay.h"
#include "config/config_manager.h"
#include "config/settings.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>

// ─── Key picker
// ──────────────────────────────────────────────────────────────
// Returns button label given VK code
static const char *KeyLabel(int vk) {
  switch (vk) {
  case 0x01:
    return "LMB";
  case 0x02:
    return "RMB";
  case 0x04:
    return "MMB";
  case 0x05:
    return "Mouse4"; // XButton1
  case 0x06:
    return "Mouse5"; // XButton2
  case 0x10:
    return "Shift";
  case 0x11:
    return "Ctrl";
  case 0x12:
    return "Alt";
  case VK_INSERT:
    return "Insert";
  case VK_END:
    return "End";
  case 'Z':
    return "Z";
  case 'X':
    return "X";
  case 'C':
    return "C";
  default: {
    static char buf[8];
    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
  }
  }
}

// Draws a "pick key" inline button. Returns true when a new key was selected.
static bool HotkeyPicker(const char *label, int &keyTarget) {
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
          0x01,    0x02,      0x04,   0x05,    0x06,      VK_SHIFT, VK_CONTROL,
          VK_MENU, VK_INSERT, VK_END, VK_HOME, VK_DELETE, VK_LEFT,  VK_RIGHT,
          VK_UP,   VK_DOWN,   VK_F1,  VK_F2,   VK_F3,     VK_F4,    VK_F5,
          VK_F6,   VK_F7,     VK_F8,  VK_F9,   VK_F10,    VK_F11,   VK_F12,
          '0',     '1',       '2',    '3',     '4',       '5',      '6',
          '7',     '8',       '9',    'Z',     'X',       'C',      'V',
          'F',     'G',       'H',    'R',     'T',       'B',
      };
      for (int v : candidateVKs) {
        if (GetAsyncKeyState(v) & 0x8000) {
          keyTarget = v;
          s_listening = nullptr;
          changed = true;
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

// ─── Custom Widgets
static void ApplyTheme(int theme) {
  ImGuiStyle *style = &ImGui::GetStyle();
  ImVec4 *colors = style->Colors;

  style->WindowRounding = 12.0f;
  style->ChildRounding = 8.0f;
  style->FrameRounding = 6.0f;
  style->PopupRounding = 8.0f;
  style->ScrollbarRounding = 6.0f;
  style->GrabRounding = 6.0f;
  style->TabRounding = 6.0f;

  style->WindowBorderSize = 1.0f;
  style->ChildBorderSize = 0.0f; // Cleaner look
  style->FrameBorderSize = 0.0f;
  style->PopupBorderSize = 1.0f;

  style->FramePadding = ImVec2(8.0f, 6.0f);
  style->ItemSpacing = ImVec2(10.0f, 10.0f);
  style->ItemInnerSpacing = ImVec2(8.0f, 6.0f);

  if (theme == 0) { // Midnight
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.40f, 0.35f, 0.80f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.40f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.30f, 0.70f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.40f, 0.35f, 0.80f, 0.50f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.40f, 0.85f, 0.60f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.40f, 0.85f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.45f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.55f, 0.95f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.45f, 0.90f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
  } else if (theme == 1) { // Blood
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.70f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.70f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.85f, 0.85f, 1.00f);
  } else if (theme == 2) { // Cyber
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.10f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.10f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.05f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.10f, 0.60f, 0.60f, 0.50f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.30f, 0.30f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.95f, 0.95f, 1.00f);
  } else if (theme == 3) { // Lavender
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.08f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.13f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.18f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.55f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.65f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.35f, 0.75f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.55f, 0.45f, 0.85f, 0.50f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.65f, 0.55f, 0.90f, 0.60f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.65f, 0.55f, 0.90f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.65f, 0.55f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.75f, 0.65f, 0.95f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.55f, 0.95f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.90f, 0.95f, 1.00f);
  } else if (theme == 4) { // Gold/Luxury
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.16f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.16f, 0.14f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.18f, 0.15f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.75f, 0.60f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.85f, 0.70f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.50f, 0.15f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.75f, 0.60f, 0.20f, 0.50f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.70f, 0.30f, 0.60f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.85f, 0.70f, 0.30f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.85f, 0.75f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.85f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.75f, 0.35f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.90f, 0.85f, 1.00f);
  } else if (theme == 5) { // Monochrome
    colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.40f, 0.50f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.60f, 0.60f, 0.60f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.60f, 0.60f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
  } else if (theme == 6) { // Toxic
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.09f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.30f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.15f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.25f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.35f, 0.22f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.40f, 0.80f, 0.10f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.90f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.70f, 0.05f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.40f, 0.80f, 0.10f, 0.50f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.90f, 0.20f, 0.60f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.90f, 0.20f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.90f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 1.00f, 0.30f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.90f, 0.20f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
  }
}

static bool DrawToggle(const char *label, bool *v) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

  float height = ImGui::GetFrameHeight();
  float width = height * 1.55f;
  float radius = height * 0.50f;

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(
      pos,
      ImVec2(pos.x + width +
                 (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x
                                      : 0.0f),
             pos.y + height));

  ImGui::ItemSize(total_bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(total_bb, id))
    return false;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
  if (pressed) {
    *v = !(*v);
    ImGui::MarkItemEdited(id);
  }

  // Animation float in state storage
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
    if (*t_anim < 0.0f)
      *t_anim = 0.0f;
    if (*t_anim > 1.0f)
      *t_anim = 1.0f;
  }

  // Colors
  ImU32 col_bg;
  ImVec4 col_off = style.Colors[ImGuiCol_FrameBg];
  ImVec4 col_on = style.Colors[ImGuiCol_Button];
  if (hovered) {
    col_off = style.Colors[ImGuiCol_FrameBgHovered];
    col_on = style.Colors[ImGuiCol_ButtonHovered];
  }

  ImU32 bg_color = ImGui::GetColorU32(ImLerp(col_off, col_on, *t_anim));

  window->DrawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                                  bg_color, height * 0.5f);
  window->DrawList->AddCircleFilled(
      ImVec2(pos.x + radius + (*t_anim) * (width - radius * 2.0f),
             pos.y + radius),
      radius - 1.5f, IM_COL32(255, 255, 255, 255));

  if (label_size.x > 0.0f) {
    ImGui::RenderText(ImVec2(pos.x + width + style.ItemInnerSpacing.x,
                             pos.y + style.FramePadding.y),
                      label);
  }

  return pressed;
}

static bool ColorEditInline(const char *label, float *color) {
  ImGui::SameLine(ImGui::GetContentRegionAvail().x -
                  ImGui::GetFrameHeight() * 1.5f);
  return ImGui::ColorEdit4(
      label, color,
      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf);
}

// ─── Config UI state
static char s_configName[64] = "default";
static std::vector<std::string> s_configList;
static int s_configSelected = 0;
static bool s_configDirty = false;
static int s_currentTab = 0; // 0=Legit, 1=Visuals, 2=Misc, 3=Skins, 4=Settings

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

  ImGui::SetNextWindowSize(ImVec2(850, 620), ImGuiCond_FirstUseEver);

  char title[64];
  snprintf(title, sizeof(title), "Antigravity External%s###MainWindow",
           s_configDirty ? " *" : "");

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  if (ImGui::Begin(title, &isOpen, ImGuiWindowFlags_NoCollapse)) {
    ImGui::PopStyleVar();

    // ─── VERTICAL LAYOUT ───
    ImGui::BeginChild("Sidebar", ImVec2(160, 0), true);

    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));

    ImGui::Dummy(ImVec2(0, 10)); // Top padding
    if (ImGui::Selectable("LEGIT", s_currentTab == 0, 0, ImVec2(0, 35)))
      s_currentTab = 0;
    if (ImGui::Selectable("VISUALS", s_currentTab == 1, 0, ImVec2(0, 35)))
      s_currentTab = 1;
    if (ImGui::Selectable("MISC", s_currentTab == 2, 0, ImVec2(0, 35)))
      s_currentTab = 2;
    if (ImGui::Selectable("SKINS", s_currentTab == 3, 0, ImVec2(0, 35)))
      s_currentTab = 3;
    if (ImGui::Selectable("SETTINGS", s_currentTab == 4, 0, ImVec2(0, 35)))
      s_currentTab = 4;

    ImGui::PopStyleVar(2);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.0f, 0.0f, 1));
    if (ImGui::Button("UNLOAD", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
      shouldClose = true;
    ImGui::PopStyleColor(3);

    ImGui::EndChild();

    ImGui::SameLine(0, 10);

    // ─── CONTENT ───
    ImGui::BeginChild("Content", ImVec2(0, 0), false, 0);
    ImGui::Dummy(ImVec2(0, 5));

    if (s_currentTab == 0) { // ─── LEGIT
      ImGui::Columns(2, "LegitCols", false);

      ImGui::BeginChild("AimbotCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Aimbot");
      ImGui::Separator();
      if (DrawToggle("Enable Aimbot", &Config::Settings.aimbot.enabled))
        Config::ConfigManager::ApplySettings();
      if (Config::Settings.aimbot.enabled) {
        ImGui::Spacing();
        HotkeyPicker("Hold Key Aimbot", Config::Settings.aimbot.hotkey);
        ImGui::Spacing();
        ImGui::SliderFloat("FOV", &Config::Settings.aimbot.fov, 1.0f, 30.0f,
                           "%.1f deg");
        ImGui::SliderFloat("Smooth", &Config::Settings.aimbot.smooth, 1.0f,
                           20.0f, "%.1f");
        ImGui::SliderFloat("Sensitivity", &Config::Settings.aimbot.sensitivity,
                           0.1f, 10.0f, "%.1f");
        ImGui::SliderFloat("Jitter", &Config::Settings.aimbot.jitter, 0.0f,
                           0.15f, "%.3f");

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

        DrawToggle("Target Lock", &Config::Settings.aimbot.targetLock);
        DrawToggle("Visible Only", &Config::Settings.aimbot.visibleOnly);
        DrawToggle("Team Check Aimbot", &Config::Settings.aimbot.teamCheck);
        DrawToggle("Only Scoped", &Config::Settings.aimbot.onlyScoped);
        DrawToggle("Show FOV Circle", &Config::Settings.aimbot.showFov);
      }
      ImGui::EndChild();

      ImGui::NextColumn();

      ImGui::BeginChild("TriggerbotCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Triggerbot");
      ImGui::Separator();
      if (DrawToggle("Enable Triggerbot", &Config::Settings.triggerbot.enabled))
        Config::ConfigManager::ApplySettings();
      if (Config::Settings.triggerbot.enabled) {
        ImGui::Spacing();
        HotkeyPicker("Hold Key Trigger", Config::Settings.triggerbot.hotkey);
        ImGui::Spacing();
        ImGui::SliderInt("Min Delay (ms)",
                         &Config::Settings.triggerbot.delayMin, 0, 150);
        ImGui::SliderInt("Max Delay (ms)",
                         &Config::Settings.triggerbot.delayMax,
                         Config::Settings.triggerbot.delayMin, 300);
        DrawToggle("Team Check Trigger",
                   &Config::Settings.triggerbot.teamCheck);
      }
      ImGui::EndChild();

      ImGui::Columns(1);
    } else if (s_currentTab == 1) { // ─── VISUALS
      ImGui::Columns(2, "VisCols", false);

      ImGui::BeginChild("ESPCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Player ESP");
      ImGui::Separator();
      if (DrawToggle("Enable ESP", &Config::Settings.esp.enabled))
        Config::ConfigManager::ApplySettings();
      DrawToggle("Team ESP", &Config::Settings.esp.showTeammates);

      ImGui::Spacing();
      DrawToggle("Draw Box", &Config::Settings.esp.showBox);
      ColorEditInline("##box_color", Config::Settings.esp.boxColor);
      if (Config::Settings.esp.showBox) {
        const char *boxStyles[] = {"Rect", "Corners", "Filled"};
        int bsInt = static_cast<int>(Config::Settings.esp.boxStyle);
        if (ImGui::Combo("Box Style", &bsInt, boxStyles, 3))
          Config::Settings.esp.boxStyle =
              static_cast<Features::BoxStyle>(bsInt);
        if (Config::Settings.esp.boxStyle == Features::BoxStyle::Filled)
          ImGui::SliderFloat("Fill Alpha", &Config::Settings.esp.fillBoxAlpha,
                             0.02f, 0.5f, "%.2f");
      }

      DrawToggle("Draw Health Bar", &Config::Settings.esp.showHealth);
      if (Config::Settings.esp.showHealth) {
        const char *hpStyles[] = {"Side", "Bottom"};
        int hsInt = static_cast<int>(Config::Settings.esp.healthBarStyle);
        if (ImGui::Combo("Bar Style", &hsInt, hpStyles, 2))
          Config::Settings.esp.healthBarStyle =
              static_cast<Features::HealthBarStyle>(hsInt);
      }

      DrawToggle("Draw Name", &Config::Settings.esp.showName);
      ColorEditInline("##name_color", Config::Settings.esp.nameColor);

      DrawToggle("Draw Weapon", &Config::Settings.esp.showWeapon);
      ColorEditInline("##weap_color", Config::Settings.esp.weaponColor);

      DrawToggle("Draw Distance", &Config::Settings.esp.showDistance);

      DrawToggle("Snap Lines", &Config::Settings.esp.showSnapLines);
      ColorEditInline("##snap_color", Config::Settings.esp.snapLineColor);

      DrawToggle("Skeleton", &Config::Settings.esp.showBones);
      ColorEditInline("##bone_color", Config::Settings.esp.boneColor);
      if (Config::Settings.esp.showBones) {
        DrawToggle("Skeleton Outline", &Config::Settings.esp.skeletonOutline);
        ColorEditInline("##skel_out_color",
                        Config::Settings.esp.skeletonOutlineColor);
      }

      ImGui::EndChild();
      ImGui::NextColumn();

      ImGui::BeginChild("RadarCard",
                        ImVec2(0, ImGui::GetWindowHeight() * 0.65f), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Radar Overlay");
      ImGui::Separator();
      if (DrawToggle("Enable Radar", &Config::Settings.radar.enabled))
        Config::ConfigManager::ApplySettings();
      if (Config::Settings.radar.enabled) {
        DrawToggle("Rotate Map", &Config::Settings.radar.rotate);
        DrawToggle("Show Teammates Radar",
                   &Config::Settings.radar.showTeammates);
        DrawToggle("Visible Check Radar", &Config::Settings.radar.visibleCheck);

        const char *maps[] = {"Custom", "Mirage", "Dust2", "Inferno", "Nuke"};
        ImGui::Combo("Map", &Config::Settings.radar.mapIndex, maps, 5);
        ImGui::SliderFloat("Zoom", &Config::Settings.radar.zoom, 0.1f, 3.0f,
                           "%.2f");
        ImGui::SliderFloat("Point Size", &Config::Settings.radar.pointSize,
                           2.0f, 8.0f, "%.1f");
      }
      ImGui::EndChild();

      ImGui::BeginChild("BombCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "World Options");
      ImGui::Separator();
      if (DrawToggle("Enable Bomb Timer", &Config::Settings.bomb.enabled))
        Config::ConfigManager::ApplySettings();
      ImGui::EndChild();

      ImGui::Columns(1);
    } else if (s_currentTab == 2) { // ─── MISC
      ImGui::BeginChild("MiscCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Miscellaneous");
      ImGui::Separator();

      if (DrawToggle("AWP Crosshair", &Config::Settings.misc.awpCrosshair))
        Config::ConfigManager::ApplySettings();
      ColorEditInline("##awp_cross_color",
                      Config::Settings.misc.crosshairColor);

      if (Config::Settings.misc.awpCrosshair) {
        const char *styles[] = {"Dot", "Cross", "Circle", "All"};
        ImGui::Combo("Style", &Config::Settings.misc.crosshairStyle, styles, 4);
        ImGui::SliderFloat("Size", &Config::Settings.misc.crosshairSize, 2.0f,
                           25.0f, "%.0f px");
        ImGui::SliderFloat("Thickness",
                           &Config::Settings.misc.crosshairThickness, 0.5f,
                           4.0f, "%.1f");
        DrawToggle("Center Gap", &Config::Settings.misc.crosshairGap);
      }
      ImGui::EndChild();
    } else if (s_currentTab == 3) { // ─── SKINS
      ImGui::BeginChild("KnifeCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Knife Changer");
      ImGui::Separator();

      if (DrawToggle("Enable Knife Changer",
                     &Config::Settings.knifeChanger.enabled))
        Config::ConfigManager::ApplySettings();

      if (Config::Settings.knifeChanger.enabled) {
        ImGui::Spacing();
        const char *knifeNames[] = {"Bayonet",        "Flip Knife", "Gut Knife",
                                    "Karambit",       "M9 Bayonet", "Huntsman",
                                    "Falchion",       "Bowie",      "Butterfly",
                                    "Shadow Daggers", "Paracord",   "Survival",
                                    "Ursus",          "Navaja",     "Stiletto",
                                    "Talon",          "Classic",    "Skeleton"};
        const int knifeIds[] = {500, 505, 506, 507, 508, 509, 512, 514, 515,
                                516, 519, 520, 521, 522, 523, 524, 525, 526};

        int sel = 3;
        for (int i = 0; i < 18; i++) {
          if (knifeIds[i] == Config::Settings.knifeChanger.knifeModel) {
            sel = i;
            break;
          }
        }
        if (ImGui::Combo("Knife Model", &sel, knifeNames, 18))
          Config::Settings.knifeChanger.knifeModel = knifeIds[sel];

        ImGui::SliderInt("Paint Kit ID",
                         &Config::Settings.knifeChanger.paintKit, 0, 1000);
        ImGui::SliderFloat("Wear", &Config::Settings.knifeChanger.wear, 0.0f,
                           1.0f, "%.3f");
        ImGui::SliderInt("Seed", &Config::Settings.knifeChanger.seed, 0, 1000);
      }
      ImGui::EndChild();
    } else if (s_currentTab == 4) { // ─── SETTINGS
      ImGui::Columns(2, "SetCols", false);

      ImGui::BeginChild("ConfigCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Configuration");
      ImGui::Separator();

      const char *themes[] = {"Midnight", "Blood",      "Cyber", "Lavender",
                              "Gold",     "Monochrome", "Toxic"};
      ImGui::Combo("Theme Preset", &Config::Settings.misc.menuTheme, themes, 7);

      ImGui::Spacing();
      ImGui::InputText("Name", s_configName, sizeof(s_configName));
      if (ImGui::Button("Save Config",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        Config::ConfigManager::Save(s_configName);
        s_configList = Config::ConfigManager::ListConfigs();
      }

      ImGui::Spacing();
      if (ImGui::Button("Refresh List",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        s_configList = Config::ConfigManager::ListConfigs();

      ImGui::BeginChild("List", ImVec2(0, 120), true);
      for (int i = 0; i < (int)s_configList.size(); i++) {
        if (ImGui::Selectable(s_configList[i].c_str(), i == s_configSelected))
          s_configSelected = i;
      }
      ImGui::EndChild();

      if (!s_configList.empty()) {
        if (ImGui::Button("Load Selected",
                          ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
          strncpy_s(s_configName, s_configList[s_configSelected].c_str(),
                    sizeof(s_configName) - 1);
          Config::ConfigManager::Load(s_configList[s_configSelected]);
        }
      }
      ImGui::EndChild();

      ImGui::NextColumn();

      ImGui::BeginChild("PerfCard", ImVec2(0, 0), true);
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
                         "Performance & Debug");
      ImGui::Separator();

      ImGui::SliderInt("FPS Limit", &Config::Settings.performance.fpsLimit, 10,
                       500, "%d FPS");
      ImGui::SliderInt("UPS Limit (Max)",
                       &Config::Settings.performance.upsLimit, 10, 500,
                       "%d UPS");

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (DrawToggle("Enable Debug Overlay", &Config::Settings.debug.enabled))
        Config::ConfigManager::ApplySettings();
      DrawToggle("Developer Mode", &Config::Settings.debug.devMode);

      ImGui::EndChild();
      ImGui::Columns(1);
    }

    ImGui::EndChild(); // Content
  } else {
    ImGui::PopStyleVar();
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
