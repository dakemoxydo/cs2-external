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
  float waveHeight;
  float startTime = 0.0f;
};

class SoundEsp : public IFeature {
public:
  void Update(const FeatureFrame &frame) override;
  void Render(const FeatureFrame &frame, Render::DrawList &drawList) override;
  std::string_view GetName() override { return "SoundEsp"; }
  void RenderUI() override;

private:
  void ResetState();

  std::vector<SoundRing> m_rings;
  uint64_t m_lastAudioEventId = 0;
  uintptr_t m_lastObservedLocalPawn = 0;
};

} // namespace Features
