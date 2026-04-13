#pragma once
#include "features/feature_base.h"
#include "core/sdk/structs.h"
#include <cstdint>
#include <vector>

namespace Features {

class Esp : public IFeature {
public:
  struct ActiveTracer {
    uint64_t shotId = 0;
    SDK::Vector3 start = {};
    SDK::Vector3 end = {};
    float createdAt = 0.0f;
    float lastUpdatedAt = 0.0f;
    float expiresAt = 0.0f;
    float impactFadeUntil = -1.0f;
    bool hasImpact = false;
    bool hitConfirmed = false;
  };

  void Update() override;
  void Render(Render::DrawList &drawList) override;
  std::string_view GetName() override { return "ESP"; }
  void RenderUI() override;

private:
  void ResetCombatVisuals();

  uintptr_t m_lastLocalPawn = 0;
  uint64_t m_lastShotEventId = 0;
  uint64_t m_lastTraceEventId = 0;
  uint64_t m_lastHitEventId = 0;
  float m_lastHitEventTime = -100.0f;
  std::vector<ActiveTracer> m_activeTracers;
};
} // namespace Features
