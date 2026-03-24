#pragma once

namespace Render {
class DrawList {
public:
  void DrawLine(float x1, float y1, float x2, float y2, float color[4],
                float thickness = 1.0f);
  void DrawBox(float x, float y, float w, float h, float color[4],
               float thickness = 1.0f);
  void DrawFilledRect(float x, float y, float w, float h, float color[4]);
  void DrawCornerBox(float x, float y, float w, float h, float color[4],
                     float thickness = 1.5f);
  void AddText(float x, float y, const char *text, float color[4]);
  void DrawCircle(float x, float y, float radius, float color[4],
                  int segments = 12, float thickness = 1.0f);
};
} // namespace Render
