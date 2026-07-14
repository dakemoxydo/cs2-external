#pragma once
#include "features/feature_base.h"

namespace Features {

class Chams : public IFeature {
public:
  ~Chams() override;

  void Initialize() override;
  bool Warmup();
  void OnDisable() override;
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  bool CanRenderPreview() const;
  bool RenderPreview(float x, float y, float width, float height,
                     bool teammate);
  std::string_view GetName() override { return "Chams"; }
  void RenderUI() override;

private:
  class Impl;
  Impl *m_impl = nullptr;
};

} // namespace Features
