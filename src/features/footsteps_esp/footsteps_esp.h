#pragma once
#include "../feature_base.h"
#include "../../core/sdk/structs.h"
#include "../../render/draw/draw_list.h"
#include <unordered_map>
#include <vector>

namespace Features {

struct FootstepRing {
  SDK::Vector3 worldPos;
  float color[4];
  float maxRadius;
  double startTime;
};

class FootstepsEsp : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  std::string_view GetName() override { return "FootstepsEsp"; }
  void RenderUI() override;

private:
  std::vector<FootstepRing> m_rings;
  std::unordered_map<uintptr_t, bool> m_prevOnGround;
};

} // namespace Features
