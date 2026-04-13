#pragma once

namespace Features {

struct ChamsConfig {
  bool enabled = false;
  bool showTeammates = false;
  bool wireframe = false;
  bool visibleCheck = true;

  int materialType = 1; // 0 = flat, 1 = shaded, 2 = rim

  float alpha = 1.0f;
  float wireAmount = 0.85f;
  float fillColor[4] = {0.22f, 0.92f, 0.42f, 1.00f};
  float hiddenColor[4] = {1.00f, 0.36f, 0.32f, 1.00f};
  float fillColorTeam[4] = {0.14f, 0.88f, 0.58f, 1.00f};
  float hiddenColorTeam[4] = {0.13f, 0.56f, 0.96f, 1.00f};
  float wireColor[4] = {1.00f, 1.00f, 1.00f, 0.85f};
};

} // namespace Features
