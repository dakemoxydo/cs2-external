#pragma once
#include <string>

namespace Render {
class DrawList; // Forward declaration
}

namespace Features {
class IFeature {
public:
  virtual ~IFeature() = default;

  virtual void Update() = 0;
  virtual void Render(Render::DrawList &drawList) = 0;
  virtual const char *GetName() = 0;

  virtual void OnEnable() {}
  virtual void OnDisable() {}

  // Метод для UI в меню
  virtual void RenderUI() {}

  bool IsEnabled() const { return enabled; }
  void SetEnabled(bool state) {
    if (enabled != state) {
      enabled = state;
      if (enabled)
        OnEnable();
      else
        OnDisable();
    }
  }

protected:
  bool enabled = false;
};
} // namespace Features
