#pragma once
#include "features/feature_base.h"
#include <chrono>
#include <random>

namespace Features {
class Triggerbot : public IFeature {
public:
  void Update(const FeatureFrame &frame) override;
  void Render(const FeatureFrame &frame, Render::DrawList &drawList) override;
  void OnDisable() override;
  std::string_view GetName() override { return "Triggerbot"; }
  void RenderUI() override;

private:
  enum class State { Idle, TargetFound, Waiting, Shooting, Cooldown };
  void ResetState();
  int RandRange(int lo, int hi);

  std::mt19937 rng_{std::random_device{}()};
  State state_ = State::Idle;
  std::chrono::steady_clock::time_point timer_{};
  int delayMs_ = 0;
};
} // namespace Features
