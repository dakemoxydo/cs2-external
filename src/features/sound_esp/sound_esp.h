#pragma once
#include "../feature_base.h"
#include "../../core/sdk/structs.h"
#include "../../render/draw/draw_list.h"
#include <unordered_map>
#include <vector>

namespace Features {

struct SoundRing {
  SDK::Vector3 worldPos;
  float color[4];
  float maxRadius;
  double startTime;
};

class SoundEsp : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override;
  std::string_view GetName() override { return "SoundEsp"; }
  void RenderUI() override;

private:
  std::vector<SoundRing> m_rings;
  // Keyed by pawnHandle for stable identity across entity address reuse
  std::unordered_map<uint32_t, bool> m_prevOnGround;
};

} // namespace Features
