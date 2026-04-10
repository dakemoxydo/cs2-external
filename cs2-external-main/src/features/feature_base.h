#pragma once
#include <string_view>

namespace Render {
class DrawList;
}

namespace Features {
class IFeature {
public:
  virtual ~IFeature() = default;

  virtual void Update() = 0;
  virtual void Render(Render::DrawList &drawList) = 0;
  virtual std::string_view GetName() = 0;

  virtual void Initialize() {}
  virtual void OnEnable() {}
  virtual void OnDisable() {}

  // Метод для UI в меню
  virtual void RenderUI() {}

  bool IsEnabled() const { return enabled; }
  bool IsInitialized() const { return initialized; }
  void SetEnabled(bool state) {
    if (enabled != state) {
      if (state && !initialized) {
        Initialize();
        initialized = true;
      }
      enabled = state;
      if (enabled)
        OnEnable();
      else
        OnDisable();
    }
  }

protected:
  bool enabled = false;
  bool initialized = false;
};
} // namespace Features
