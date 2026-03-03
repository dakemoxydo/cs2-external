#pragma once
#include "feature_base.h"
#include <memory>
#include <vector>

namespace Render {
class DrawList;
}

namespace Features {
class FeatureManager {
public:
  static void RegisterAll();
  static void UpdateAll();
  static void RenderAll(Render::DrawList &drawList);

public:
  static std::vector<std::unique_ptr<IFeature>> features;
};
} // namespace Features
