#include "draw_list.h"
#include <imgui.h>

namespace Render {

namespace {
// Returns the ImGui background draw list, or nullptr if no ImGui frame/context
// is currently active. Callers must guard against nullptr to avoid a crash when
// drawing happens outside an active ImGui frame (e.g. during device-lost
// recovery).
ImDrawList *GetDL() { return ImGui::GetBackgroundDrawList(); }
} // namespace

void DrawList::DrawLine(float x1, float y1, float x2, float y2, float color[4],
                         float thickness) {
  ImDrawList *dl = GetDL();
  if (!dl) return;
  dl->AddLine(
      ImVec2(x1, y1), ImVec2(x2, y2),
      ImGui::ColorConvertFloat4ToU32(
          ImVec4(color[0], color[1], color[2], color[3])),
      thickness);
}

void DrawList::DrawBox(float x, float y, float w, float h, float color[4],
                         float thickness) {
  ImDrawList *dl = GetDL();
  if (!dl) return;
  dl->AddRect(
      ImVec2(x, y), ImVec2(x + w, y + h),
      ImGui::ColorConvertFloat4ToU32(
          ImVec4(color[0], color[1], color[2], color[3])),
      0.0f, 0, thickness);
}

void DrawList::DrawFilledRect(float x, float y, float w, float h,
                               float color[4]) {
  ImDrawList *dl = GetDL();
  if (!dl) return;
  dl->AddRectFilled(
      ImVec2(x, y), ImVec2(x + w, y + h),
      ImGui::ColorConvertFloat4ToU32(
          ImVec4(color[0], color[1], color[2], color[3])));
}

void DrawList::DrawCornerBox(float x, float y, float w, float h, float color[4],
                              float thickness) {
  ImDrawList *dl = GetDL();
  if (!dl) return;
  ImU32 c = ImGui::ColorConvertFloat4ToU32(
      ImVec4(color[0], color[1], color[2], color[3]));
  float cx = w * 0.22f;
  float cy = h * 0.12f;
  // Top-left
  dl->AddLine(ImVec2(x, y), ImVec2(x + cx, y), c, thickness);
  dl->AddLine(ImVec2(x, y), ImVec2(x, y + cy), c, thickness);
  // Top-right
  dl->AddLine(ImVec2(x + w, y), ImVec2(x + w - cx, y), c, thickness);
  dl->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + cy), c, thickness);
  // Bottom-left
  dl->AddLine(ImVec2(x, y + h), ImVec2(x + cx, y + h), c, thickness);
  dl->AddLine(ImVec2(x, y + h), ImVec2(x, y + h - cy), c, thickness);
  // Bottom-right
  dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w - cx, y + h), c, thickness);
  dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - cy), c, thickness);
}

void DrawList::AddText(float x, float y, const char *text, float color[4]) {
  ImDrawList *dl = GetDL();
  if (!dl) return;
  dl->AddText(
      ImVec2(x, y),
      ImGui::ColorConvertFloat4ToU32(
          ImVec4(color[0], color[1], color[2], color[3])),
      text);
}

void DrawList::DrawCircle(float x, float y, float radius, float color[4],
                           int segments, float thickness) {
  ImDrawList *dl = GetDL();
  if (!dl) return;
  dl->AddCircle(
      ImVec2(x, y), radius,
      ImGui::ColorConvertFloat4ToU32(
          ImVec4(color[0], color[1], color[2], color[3])),
      segments, thickness);
}

} // namespace Render
