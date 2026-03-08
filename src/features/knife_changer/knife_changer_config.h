#pragma once

namespace Features {

struct KnifeChangerConfig {
  bool enabled = false;
  int knifeModel = 507; // default: Karambit
  int paintKit = 0;
  float wear = 0.001f;
  int seed = 0;
};

} // namespace Features
