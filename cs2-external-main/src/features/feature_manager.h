#pragma once
#include "feature_base.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Render {
class DrawList;
}

namespace Features {
class FeatureManager {
public:
  using FeatureFactory = std::function<std::unique_ptr<IFeature>()>;

  static void RegisterFeature(std::string_view name, FeatureFactory factory);
  static void RegisterAll();
  static void UpdateAll();
  static void RenderAll(Render::DrawList &drawList);
  static void EnsureFeatureInitialized(std::string_view name);
  static void EnsureAllInitialized();
  static IFeature* GetFeature(std::string_view name);

public:
  struct FeatureSlot {
    std::string name;
    FeatureFactory factory;
    std::unique_ptr<IFeature> instance;
    FeatureSlot(std::string_view n, FeatureFactory f) : name(n), factory(std::move(f)), instance(nullptr) {}
  };
  static std::vector<FeatureSlot> featureSlots;
};
} // namespace Features
