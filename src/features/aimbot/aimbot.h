#pragma once
#include "../feature_base.h"
#include <cstdint>
#include <random>

namespace Features {
class Aimbot : public IFeature {
public:
  void Update(const FeatureFrame &frame) override;
  void Render(const FeatureFrame &frame, Render::DrawList &drawList) override;
  void OnDisable() override;
  std::string_view GetName() override;
  void RenderUI() override;

private:
  void ResetState();
  void SendMouse(float dpitch, float dyaw, float sensitivity);

  uintptr_t lastTarget_ = 0;
  float dxRemainder_ = 0.0f;
  float dyRemainder_ = 0.0f;
  int jitterIndex_ = 0;
  float lastTotalDelta_ = 0.0f;
  uint64_t pauseUntil_ = 0;
  std::mt19937 rng_{std::random_device{}()};
  std::uniform_real_distribution<float> noiseDist_{-0.5f, 0.5f};
  std::uniform_int_distribution<int> microDelayDist_{0, 9};
};
} // namespace Features
