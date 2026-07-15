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
  static void UpdateAll(const FeatureFrame &frame);
  static void RenderAll(const FeatureFrame &frame, Render::DrawList &drawList);
  static void EnsureFeatureInitialized(std::string_view name);
  static void EnsureAllInitialized();
  // Destroys feature-owned resources while their rendering dependencies are
  // still alive. Factories remain registered, so a new application lifecycle
  // may initialize them again.
  static void ShutdownAll();
  static void SetEnabled(std::string_view name, bool enabled);
  static void ForEachInitialized(const std::function<void(IFeature &)> &visitor);
  static IFeature* GetFeature(std::string_view name);

private:
  struct FeatureSlot {
    std::string name;
    FeatureFactory factory;
    std::unique_ptr<IFeature> instance;
    bool failed = false;
    FeatureSlot(std::string_view n, FeatureFactory f)
        : name(n), factory(std::move(f)), instance(nullptr) {}
  };
  static bool CreateInstance(FeatureSlot &slot);
  static bool SetSlotEnabled(FeatureSlot &slot, bool enabled);
  static void Quarantine(FeatureSlot &slot, std::string_view phase,
                         const char *message = nullptr);
  static std::vector<FeatureSlot> featureSlots;
};
} // namespace Features
