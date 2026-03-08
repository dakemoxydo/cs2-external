#pragma once
#include "features/feature_base.h"
#include <cstdint>
#include <unordered_map>

namespace Features {

constexpr int KNIFE_BAYONET = 500;
constexpr int KNIFE_FLIP = 505;
constexpr int KNIFE_GUT = 506;
constexpr int KNIFE_KARAMBIT = 507;
constexpr int KNIFE_M9_BAYONET = 508;
constexpr int KNIFE_HUNTSMAN = 509;
constexpr int KNIFE_FALCHION = 512;
constexpr int KNIFE_BOWIE = 514;
constexpr int KNIFE_BUTTERFLY = 515;
constexpr int KNIFE_SHADOW_DAGGERS = 516;
constexpr int KNIFE_PARACORD = 519;
constexpr int KNIFE_SURVIVAL = 520;
constexpr int KNIFE_URSUS = 521;
constexpr int KNIFE_NAVAJA = 522;
constexpr int KNIFE_STILETTO = 523;
constexpr int KNIFE_TALON = 524;
constexpr int KNIFE_CLASSIC = 525;
constexpr int KNIFE_SKELETON = 526;

class KnifeChanger : public IFeature {
public:
  void Update() override;
  void Render(Render::DrawList &drawList) override {
  } // empty — no render output
  const char *GetName() override { return "KnifeChanger"; }

private:
  void ApplyKnife();
  bool IsKnifeIndex(int defIndex) const;

  uintptr_t m_lastWeaponHandle = 0;
  bool m_pendingRestore = false;
  uint32_t m_pendingHandle = 0;

  int m_cachedModelIndex = -1;
  uint64_t ScanForModelHandle(int targetDefIndex);
  static std::unordered_map<int, uint64_t> s_modelHandleCache;
};

} // namespace Features
