#include "esp.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/sdk/entity.h"
#include "esp_config.h"
#include "features/feature_frame.h"
#include "render/draw/draw_list.h"
#include "render/overlay/overlay.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <imgui.h>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Features {

void Esp::ResetCombatVisuals() {
  m_lastLocalPawn = 0;
  m_lastShotEventId = 0;
  m_lastTraceEventId = 0;
  m_lastHitEventId = 0;
  m_lastHitEventTime = -100.0f;
  m_activeTracers.clear();
}

struct EspSnapshot {
  bool enabled, showTeammates, showBox, showHealth, showName, showWeapon;
  bool showDistance, showBones, showSnapLines, showOffscreen;
  bool showBulletTracers, showHitmarker;
  bool frustumCullingEnabled, showHealthText, skeletonOutline;
  BoxStyle boxStyle;
  HealthBarStyle healthBarStyle;
  float fillBoxAlpha, skeletonMaxDistance;
  float bulletTracerThickness, bulletTracerLife, bulletTracerImpactRadius;
  float bulletTracerImpactThickness, hitmarkerLife;
  float teamColor[4], boxColor[4], nameColor[4], weaponColor[4];
  float distColor[4], snapLineColor[4], boneColor[4];
  float skeletonOutlineColor[4], offscreenColor[4], bulletTracerColor[4];
  float bulletTracerImpactColor[4], hitmarkerColor[4];
};

static SDK::Vector3 LerpVector(const SDK::Vector3 &a, const SDK::Vector3 &b,
                               float t) {
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

static float TracerImpactHold(float tracerLife) {
  return std::clamp(tracerLife * 0.35f, 0.10f, 0.28f);
}

static EspSnapshot SnapshotEsp(const EspConfig &E) {
  EspSnapshot s{};
  s = {E.enabled,
       E.showTeammates,
       E.showBox,
       E.showHealth,
       E.showName,
       E.showWeapon,
       E.showDistance,
       E.showBones,
       E.showSnapLines,
       E.showOffscreen,
       E.showBulletTracers,
       E.showHitmarker,
       E.frustumCullingEnabled,
       E.showHealthText,
       E.skeletonOutline,
       E.boxStyle,
       E.healthBarStyle,
       E.fillBoxAlpha,
       E.skeletonMaxDistance,
       E.bulletTracerThickness,
       E.bulletTracerLife,
       E.bulletTracerImpactRadius,
       E.bulletTracerImpactThickness,
       E.hitmarkerLife};
  std::copy(std::begin(E.teamColor), std::end(E.teamColor), s.teamColor);
  std::copy(std::begin(E.boxColor), std::end(E.boxColor), s.boxColor);
  std::copy(std::begin(E.nameColor), std::end(E.nameColor), s.nameColor);
  std::copy(std::begin(E.weaponColor), std::end(E.weaponColor), s.weaponColor);
  std::copy(std::begin(E.distColor), std::end(E.distColor), s.distColor);
  std::copy(std::begin(E.snapLineColor), std::end(E.snapLineColor),
            s.snapLineColor);
  std::copy(std::begin(E.boneColor), std::end(E.boneColor), s.boneColor);
  std::copy(std::begin(E.skeletonOutlineColor),
            std::end(E.skeletonOutlineColor), s.skeletonOutlineColor);
  std::copy(std::begin(E.offscreenColor), std::end(E.offscreenColor),
            s.offscreenColor);
  std::copy(std::begin(E.bulletTracerColor), std::end(E.bulletTracerColor),
            s.bulletTracerColor);
  std::copy(std::begin(E.bulletTracerImpactColor),
            std::end(E.bulletTracerImpactColor), s.bulletTracerImpactColor);
  std::copy(std::begin(E.hitmarkerColor), std::end(E.hitmarkerColor),
            s.hitmarkerColor);
  return s;
}

void Esp::Update(const FeatureFrame &frame) {
  const EspSnapshot s = SnapshotEsp(frame.settings.esp);
  const auto &snapshot = frame.game;
  if (!s.enabled || snapshot.localPawn == 0) {
    ResetCombatVisuals();
    return;
  }

  if (snapshot.localPawn != m_lastLocalPawn) {
    ResetCombatVisuals();
    m_lastLocalPawn = snapshot.localPawn;
  }

  const float now = snapshot.frameTimeSeconds;
  const float tracerLife = std::max(0.05f, s.bulletTracerLife);
  const float impactHold = TracerImpactHold(tracerLife);

  if (s.showBulletTracers) {
    for (const auto &shot : snapshot.shotEvents) {
      if (shot.id <= m_lastShotEventId) {
        continue;
      }

      ActiveTracer tracer{};
      tracer.shotId = shot.id;
      tracer.start = shot.start;
      tracer.end = shot.predictedEnd;
      tracer.createdAt = shot.timeSeconds;
      tracer.lastUpdatedAt = shot.timeSeconds;
      tracer.expiresAt = shot.timeSeconds + tracerLife;
      m_activeTracers.push_back(tracer);
      m_lastShotEventId = std::max(m_lastShotEventId, shot.id);
    }

    for (const auto &trace : snapshot.bulletTraceEvents) {
      if (trace.id <= m_lastTraceEventId) {
        continue;
      }

      ActiveTracer *targetTracer = nullptr;
      if (trace.shotId != 0) {
        for (auto &activeTracer : m_activeTracers) {
          if (activeTracer.shotId == trace.shotId) {
            targetTracer = &activeTracer;
            break;
          }
        }
      }

      if (targetTracer == nullptr) {
        ActiveTracer tracer{};
        tracer.shotId = trace.shotId;
        tracer.start = trace.start;
        tracer.end = trace.end;
        tracer.createdAt = trace.timeSeconds;
        tracer.lastUpdatedAt = trace.timeSeconds;
        tracer.expiresAt = trace.timeSeconds + tracerLife;
        tracer.hasImpact = trace.confirmedImpact;
        if (trace.confirmedImpact) {
          tracer.impactFadeUntil = trace.timeSeconds + impactHold;
        }
        m_activeTracers.push_back(tracer);
      } else {
        targetTracer->start = trace.start;
        targetTracer->end = trace.end;
        targetTracer->hasImpact = targetTracer->hasImpact || trace.confirmedImpact;
        targetTracer->lastUpdatedAt = trace.timeSeconds;
        if (trace.confirmedImpact) {
          targetTracer->impactFadeUntil =
              std::max(targetTracer->impactFadeUntil, trace.timeSeconds + impactHold);
        }
      }

      m_lastTraceEventId = std::max(m_lastTraceEventId, trace.id);
    }
  } else {
    m_activeTracers.clear();
  }

  if (s.showHitmarker) {
    for (const auto &hit : snapshot.hitEvents) {
      if (hit.id <= m_lastHitEventId) {
        continue;
      }

      m_lastHitEventTime = hit.timeSeconds;
      if (hit.shotId != 0) {
        for (auto &activeTracer : m_activeTracers) {
          if (activeTracer.shotId == hit.shotId) {
            activeTracer.hitConfirmed = true;
            activeTracer.lastUpdatedAt =
                std::max(activeTracer.lastUpdatedAt, hit.timeSeconds);
            activeTracer.impactFadeUntil =
                std::max(activeTracer.impactFadeUntil, hit.timeSeconds + impactHold);
            break;
          }
        }
      }
      m_lastHitEventId = std::max(m_lastHitEventId, hit.id);
    }
  } else {
    m_lastHitEventTime = -100.0f;
  }

  m_activeTracers.erase(
      std::remove_if(m_activeTracers.begin(), m_activeTracers.end(),
                     [now](const ActiveTracer &tracer) {
                       const float visibleUntil =
                           std::max(tracer.expiresAt, tracer.impactFadeUntil);
                       return visibleUntil > 0.0f && now >= visibleUntil;
                     }),
      m_activeTracers.end());

  if (m_activeTracers.size() > 64) {
    m_activeTracers.erase(m_activeTracers.begin(), m_activeTracers.end() - 64);
  }
}

static bool ResolveTracerScreenPoints(
    const SDK::Vector3 &start, const SDK::Vector3 &end,
    const SDK::Matrix4x4 &viewMatrix, int screenWidth, int screenHeight,
    SDK::Vector2 &screenStart, SDK::Vector2 &screenEnd) {
  bool hasStart = Core::Math::WorldToScreen(start, screenStart, viewMatrix,
                                            screenWidth, screenHeight);
  bool hasEnd = Core::Math::WorldToScreen(end, screenEnd, viewMatrix,
                                          screenWidth, screenHeight);

  if (!hasStart) {
    SDK::Vector3 visibleNear = end;
    SDK::Vector3 invisibleFar = start;
    SDK::Vector2 visiblePoint{};
    bool foundVisible = hasEnd;
    if (hasEnd) {
      visiblePoint = screenEnd;
    }

    for (int i = 0; i < 16; ++i) {
      const SDK::Vector3 mid = LerpVector(invisibleFar, visibleNear, 0.5f);
      SDK::Vector2 projected{};
      if (Core::Math::WorldToScreen(mid, projected, viewMatrix, screenWidth,
                                    screenHeight)) {
        visibleNear = mid;
        visiblePoint = projected;
        foundVisible = true;
      } else {
        invisibleFar = mid;
      }
    }

    if (foundVisible) {
      screenStart = visiblePoint;
      hasStart = true;
    }
  }

  if (!hasEnd) {
    SDK::Vector2 visiblePoint{};
    bool foundVisible = false;
    SDK::Vector3 nearPoint = start;
    SDK::Vector3 farPoint = end;

    for (int i = 0; i < 12; ++i) {
      SDK::Vector3 mid = LerpVector(nearPoint, farPoint, 0.5f);
      SDK::Vector2 projected{};
      if (Core::Math::WorldToScreen(mid, projected, viewMatrix, screenWidth,
                                    screenHeight)) {
        visiblePoint = projected;
        nearPoint = mid;
        foundVisible = true;
      } else {
        farPoint = mid;
      }
    }

    if (!foundVisible) {
      return false;
    }
    screenEnd = visiblePoint;
    hasEnd = true;
  }

  return hasStart && hasEnd;
}

static void DrawImpactRing(Render::DrawList &drawList,
                           const SDK::Vector3 &impactPosition,
                           const SDK::Matrix4x4 &viewMatrix, int screenWidth,
                           int screenHeight, const EspSnapshot &s,
                           float alphaRatio) {
  const int segments = 18;
  const float radius = std::max(1.0f, s.bulletTracerImpactRadius);
  float ringColor[4] = {s.bulletTracerImpactColor[0], s.bulletTracerImpactColor[1],
                        s.bulletTracerImpactColor[2],
                        s.bulletTracerImpactColor[3] * alphaRatio};

  auto drawPlaneRing = [&](bool verticalPlane) {
    SDK::Vector2 prev{};
    bool hasPrev = false;
    for (int i = 0; i <= segments; ++i) {
      const float t = (static_cast<float>(i) / static_cast<float>(segments)) *
                      6.2831853f;
      SDK::Vector3 point = impactPosition;
      if (verticalPlane) {
        point.y += std::cos(t) * radius;
        point.z += std::sin(t) * radius;
      } else {
        point.x += std::cos(t) * radius;
        point.y += std::sin(t) * radius;
      }

      SDK::Vector2 projected{};
      if (!Core::Math::WorldToScreen(point, projected, viewMatrix, screenWidth,
                                     screenHeight)) {
        hasPrev = false;
        continue;
      }

      if (hasPrev) {
        drawList.DrawLine(prev.x, prev.y, projected.x, projected.y, ringColor,
                          s.bulletTracerImpactThickness);
      }

      prev = projected;
      hasPrev = true;
    }
  };

  drawPlaneRing(false);
  drawPlaneRing(true);

  SDK::Vector2 impactScreen{};
  if (Core::Math::WorldToScreen(impactPosition, impactScreen, viewMatrix, screenWidth,
                                screenHeight)) {
    const float coreRadius = std::max(2.0f, s.bulletTracerImpactThickness + 1.5f);
    const float haloRadius =
        std::max(coreRadius + 3.0f, s.bulletTracerImpactRadius * 0.35f);
    float haloColor[4] = {s.bulletTracerImpactColor[0], s.bulletTracerImpactColor[1],
                          s.bulletTracerImpactColor[2],
                          s.bulletTracerImpactColor[3] * alphaRatio * 0.28f};
    float coreColor[4] = {s.bulletTracerImpactColor[0], s.bulletTracerImpactColor[1],
                          s.bulletTracerImpactColor[2],
                          s.bulletTracerImpactColor[3] * alphaRatio};
    drawList.DrawCircle(impactScreen.x, impactScreen.y, haloRadius, haloColor, 20,
                        coreRadius);
    drawList.DrawCircle(impactScreen.x, impactScreen.y, coreRadius, coreColor, 16,
                        std::max(1.0f, s.bulletTracerImpactThickness));
  }
}

static void DrawBulletTracers(
    Render::DrawList &drawList, const std::vector<Esp::ActiveTracer> &tracers,
    const Core::GameSnapshot &snapshot,
    const EspSnapshot &s, int screenWidth, int screenHeight) {
  if (!s.showBulletTracers) {
    return;
  }

  const float tracerLife = std::max(0.05f, s.bulletTracerLife);
  const float now = snapshot.frameTimeSeconds;
  for (const auto &tracer : tracers) {
    const float beamEndTime =
        tracer.expiresAt > 0.0f ? tracer.expiresAt : (tracer.createdAt + tracerLife);
    const float beamAlphaRatio =
        std::clamp((beamEndTime - now) / tracerLife, 0.0f, 1.0f);
    if (beamAlphaRatio <= 0.0f) {
      continue;
    }

    SDK::Vector2 screenStart{};
    SDK::Vector2 screenEnd{};
    if (!ResolveTracerScreenPoints(tracer.start, tracer.end, snapshot.viewMatrix,
                                   screenWidth, screenHeight, screenStart,
                                   screenEnd)) {
      continue;
    }

    const float dx = screenEnd.x - screenStart.x;
    const float dy = screenEnd.y - screenStart.y;
    const float length = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
    const float nx = -dy / length;
    const float ny = dx / length;
    const float sideOffset = s.bulletTracerThickness * 1.2f;
    const float hitBoost = tracer.hitConfirmed ? 1.15f : 1.0f;

    float glowColor[4] = {s.bulletTracerColor[0], s.bulletTracerColor[1],
                          s.bulletTracerColor[2],
                          s.bulletTracerColor[3] * beamAlphaRatio * 0.30f * hitBoost};
    float sideColor[4] = {s.bulletTracerColor[0], s.bulletTracerColor[1],
                          s.bulletTracerColor[2],
                          s.bulletTracerColor[3] * beamAlphaRatio * 0.18f * hitBoost};
    float tracerColor[4] = {s.bulletTracerColor[0], s.bulletTracerColor[1],
                            s.bulletTracerColor[2],
                            s.bulletTracerColor[3] * beamAlphaRatio * hitBoost};

    drawList.DrawLine(screenStart.x, screenStart.y, screenEnd.x, screenEnd.y,
                      glowColor, s.bulletTracerThickness + 3.5f);
    drawList.DrawLine(screenStart.x + nx * sideOffset,
                      screenStart.y + ny * sideOffset, screenEnd.x + nx * sideOffset,
                      screenEnd.y + ny * sideOffset, sideColor,
                      s.bulletTracerThickness);
    drawList.DrawLine(screenStart.x - nx * sideOffset,
                      screenStart.y - ny * sideOffset, screenEnd.x - nx * sideOffset,
                      screenEnd.y - ny * sideOffset, sideColor,
                      s.bulletTracerThickness);
    drawList.DrawLine(screenStart.x, screenStart.y, screenEnd.x, screenEnd.y,
                      tracerColor, s.bulletTracerThickness);

    if (tracer.hasImpact && tracer.impactFadeUntil > now) {
      const float impactDuration =
          std::max(0.05f, tracer.impactFadeUntil - tracer.lastUpdatedAt);
      const float impactAlphaRatio = std::clamp(
          (tracer.impactFadeUntil - now) / impactDuration, 0.0f, 1.0f);
      DrawImpactRing(drawList, tracer.end, snapshot.viewMatrix, screenWidth,
                     screenHeight, s,
                     impactAlphaRatio * (tracer.hitConfirmed ? 1.10f : 1.0f));
    }
  }
}

static void DrawHitmarker(Render::DrawList &drawList, float lastHitEventTime,
                          float now, const EspSnapshot &s, int screenWidth,
                          int screenHeight) {
  if (!s.showHitmarker || lastHitEventTime < 0.0f) {
    return;
  }

  const float totalLife = std::max(0.05f, s.hitmarkerLife);
  const float age = now - lastHitEventTime;
  if (age < 0.0f || age >= totalLife) {
    return;
  }

  const float alpha = std::clamp(1.0f - (age / totalLife), 0.0f, 1.0f);
  const float size = 11.0f;
  const float gap = 3.5f;
  const float centerX = screenWidth * 0.5f;
  const float centerY = screenHeight * 0.5f;

  float hitColor[4] = {s.hitmarkerColor[0], s.hitmarkerColor[1],
                       s.hitmarkerColor[2], s.hitmarkerColor[3] * alpha};
  drawList.DrawLine(centerX - size, centerY - size, centerX - gap, centerY - gap,
                    hitColor, 1.8f);
  drawList.DrawLine(centerX + size, centerY - size, centerX + gap, centerY - gap,
                    hitColor, 1.8f);
  drawList.DrawLine(centerX - size, centerY + size, centerX - gap, centerY + gap,
                    hitColor, 1.8f);
  drawList.DrawLine(centerX + size, centerY + size, centerX + gap, centerY + gap,
                    hitColor, 1.8f);
}

static void DrawOffscreenIndicator(Render::DrawList &drawList,
                                   const SDK::Entity &player,
                                   const SDK::Matrix4x4 &viewMatrix,
                                   int screenWidth, int screenHeight,
                                   const EspSnapshot &s) {
  const SDK::Vector3 &pos = player.renderPosition;
  float w = viewMatrix.m[3][0] * pos.x + viewMatrix.m[3][1] * pos.y +
            viewMatrix.m[3][2] * pos.z + viewMatrix.m[3][3];
  if (w < 0.01f)
    return;

  float x = viewMatrix.m[0][0] * pos.x + viewMatrix.m[0][1] * pos.y +
            viewMatrix.m[0][2] * pos.z + viewMatrix.m[0][3];
  float y = viewMatrix.m[1][0] * pos.x + viewMatrix.m[1][1] * pos.y +
            viewMatrix.m[1][2] * pos.z + viewMatrix.m[1][3];

  float screenX = (screenWidth / 2.0f) * (1.0f + x / w);
  float screenY = (screenHeight / 2.0f) * (1.0f - y / w);

  float pad = 40.0f;
  float cx = std::clamp(screenX, pad, screenWidth - pad);
  float cy = std::clamp(screenY, pad, screenHeight - pad);

  float color[4];
  if (player.isTeammate) {
    color[0] = s.teamColor[0];
    color[1] = s.teamColor[1];
    color[2] = s.teamColor[2];
    color[3] = s.teamColor[3] * 0.5f;
  } else {
    color[0] = s.offscreenColor[0];
    color[1] = s.offscreenColor[1];
    color[2] = s.offscreenColor[2];
    color[3] = s.offscreenColor[3];
  }

  float angle = std::atan2(screenY - screenHeight / 2.0f,
                           screenX - screenWidth / 2.0f);
  float size = 12.0f;
  float tipX = cx + std::cos(angle) * size;
  float tipY = cy - std::sin(angle) * size;
  // Base points fan out by a small angle (~23 degrees) to form an arrowhead.
  // (atan2 returns radians, so the previous +/-2.5 rad was ~143 degrees and
  // produced a nearly-flat/incorrect triangle.)
  constexpr float kArrowHalfAngle = 0.4f;
  float baseAngle1 = angle + kArrowHalfAngle;
  float baseAngle2 = angle - kArrowHalfAngle;
  float b1x = cx + std::cos(baseAngle1) * size;
  float b1y = cy - std::sin(baseAngle1) * size;
  float b2x = cx + std::cos(baseAngle2) * size;
  float b2y = cy - std::sin(baseAngle2) * size;

  drawList.DrawLine(b1x, b1y, tipX, tipY, color, 2.0f);
  drawList.DrawLine(b2x, b2y, tipX, tipY, color, 2.0f);

  char distBuf[16];
  snprintf(distBuf, sizeof(distBuf), "%.0fm", player.distance);
  drawList.AddText(cx - 10, cy + 16, distBuf, color);
}

static void DrawRoundedSkeleton(Render::DrawList &drawList,
                                const SDK::Entity &player,
                                const SDK::Matrix4x4 &viewMatrix, int screenWidth,
                                int screenHeight, const EspSnapshot &s,
                                float boxWidth) {
  float boneCol[4] = {s.boneColor[0], s.boneColor[1], s.boneColor[2],
                      s.boneColor[3]};
  float shadowC[4] = {s.skeletonOutlineColor[0], s.skeletonOutlineColor[1],
                      s.skeletonOutlineColor[2], s.skeletonOutlineColor[3]};

  // Draw the rig's actual adjacency graph. Skipping an off-screen bone must
  // not join unrelated points (the old smoothed chains caused bent skeletons).
  for (const auto &edge : s_boneConnections) {
    if (edge[0] >= static_cast<int>(player.bonePositions.size()) ||
        edge[1] >= static_cast<int>(player.bonePositions.size())) continue;
    SDK::Vector2 a{}, b{};
    if (!Core::Math::WorldToScreen(player.bonePositions[edge[0]], a, viewMatrix,
                                   screenWidth, screenHeight) ||
        !Core::Math::WorldToScreen(player.bonePositions[edge[1]], b, viewMatrix,
                                   screenWidth, screenHeight)) continue;
    if (s.skeletonOutline) drawList.DrawLine(a.x, a.y, b.x, b.y, shadowC, 2.8f);
    drawList.DrawLine(a.x, a.y, b.x, b.y, boneCol, 1.5f);
  }

  if (static_cast<int>(player.bonePositions.size()) > BONE_HEAD) {
    SDK::Vector2 screenBoneHead;
    if (Core::Math::WorldToScreen(player.bonePositions[BONE_HEAD], screenBoneHead,
                                  viewMatrix, screenWidth, screenHeight)) {
      const float headRadius = boxWidth * 0.18f;
      if (s.skeletonOutline) {
        drawList.DrawCircle(screenBoneHead.x, screenBoneHead.y, headRadius + 1.0f,
                            shadowC, 28, 2.5f);
      }
      drawList.DrawCircle(screenBoneHead.x, screenBoneHead.y, headRadius, boneCol,
                          28, 1.2f);
    }
  }
}

void Esp::Render(const FeatureFrame &frame, Render::DrawList &drawList) {
  EspSnapshot s = SnapshotEsp(frame.settings.esp);
  if (!s.enabled)
    return;

  const bool anyActive = s.showBox || s.showHealth || s.showName ||
                         s.showWeapon || s.showDistance || s.showBones ||
                         s.showSnapLines || s.showBulletTracers ||
                         s.showHitmarker;
  if (!anyActive)
    return;

  const int screenWidth = Render::Overlay::GetGameWidth();
  const int screenHeight = Render::Overlay::GetGameHeight();
  if (screenWidth <= 0 || screenHeight <= 0)
    return;

  const auto &game = frame.game;

  DrawBulletTracers(drawList, m_activeTracers, game, s, screenWidth,
                    screenHeight);
  DrawHitmarker(drawList, m_lastHitEventTime, game.frameTimeSeconds, s,
                screenWidth, screenHeight);

  const SDK::Matrix4x4 viewMatrix = game.viewMatrix;
  const auto &players = game.players;

  for (const auto &player : players) {
    if (!player.IsValid() || !player.IsAlive()) {
      continue;
    }
    if (player.isTeammate && !s.showTeammates) {
      continue;
    }

    if (s.frustumCullingEnabled && !player.onScreen) {
      if (s.showOffscreen) {
        DrawOffscreenIndicator(drawList, player, viewMatrix, screenWidth,
                               screenHeight, s);
      }
      continue;
    }

    const float *currentBoxColor = player.isTeammate ? s.teamColor : s.boxColor;
    float drawColor[4] = {currentBoxColor[0], currentBoxColor[1],
                          currentBoxColor[2], currentBoxColor[3]};

    SDK::Vector3 feetPos = player.renderPosition;
    SDK::Vector3 headPos = feetPos;
    headPos.z += 72.0f;

    SDK::Vector2 screenFeet, screenHead;
    if (!Core::Math::WorldToScreen(feetPos, screenFeet, viewMatrix, screenWidth,
                                   screenHeight) ||
        !Core::Math::WorldToScreen(headPos, screenHead, viewMatrix, screenWidth,
                                   screenHeight)) {
      continue;
    }

    const float height = screenFeet.y - screenHead.y;
    if (height <= 0.0f) {
      continue;
    }
    const float width = height / 2.0f;
    const float x = screenFeet.x - width / 2.0f;
    const float y = screenHead.y;

    if (s.showSnapLines) {
      float sc[4] = {s.snapLineColor[0], s.snapLineColor[1], s.snapLineColor[2],
                     s.snapLineColor[3]};
      drawList.DrawLine(static_cast<float>(screenWidth) / 2.0f,
                        static_cast<float>(screenHeight), screenFeet.x,
                        screenFeet.y, sc, 1.0f);
    }

    if (s.showBox) {
      float outlineColor[4] = {0.0f, 0.0f, 0.0f, 0.85f};

      switch (s.boxStyle) {
      case BoxStyle::Corners: {
        float shadowCol[4] = {0.f, 0.f, 0.f, 0.7f};
        drawList.DrawCornerBox(x - 1, y - 1, width + 2, height + 2, shadowCol,
                               3.0f);
        drawList.DrawCornerBox(x, y, width, height, drawColor, 1.5f);
        break;
      }
      case BoxStyle::Filled: {
        float fillCol[4] = {drawColor[0], drawColor[1], drawColor[2],
                            s.fillBoxAlpha};
        drawList.DrawFilledRect(x, y, width, height, fillCol);
        drawList.DrawBox(x - 1, y - 1, width + 2, height + 2, outlineColor,
                         1.0f);
        drawList.DrawBox(x, y, width, height, drawColor, 1.5f);
        break;
      }
      default:
        drawList.DrawBox(x - 1, y - 1, width + 2, height + 2, outlineColor,
                         1.0f);
        drawList.DrawBox(x, y, width, height, drawColor, 1.5f);
        drawList.DrawBox(x + 1, y + 1, width - 2, height - 2, outlineColor,
                         1.0f);
        break;
      }
    }

    if (s.showHealth) {
      float hpPercent = std::clamp(static_cast<float>(player.health) / 100.0f,
                                   0.0f, 1.0f);
      float hpColor[4] = {1.0f - hpPercent, hpPercent, 0.0f, 1.0f};
      float bgColor[4] = {0.0f, 0.0f, 0.0f, 0.9f};

      if (s.healthBarStyle == HealthBarStyle::Bottom) {
        const float barY = y + height + 3.0f;
        const float barH = 4.0f;
        const float barW = width * hpPercent;
        drawList.DrawLine(x, barY, x + width, barY, bgColor, barH);
        drawList.DrawLine(x, barY, x + barW, barY, hpColor, barH - 1.0f);
      } else {
        const float hpHeight = height * hpPercent;
        drawList.DrawLine(x - 6, y, x - 6, y + height, bgColor, 3.0f);
        drawList.DrawLine(x - 6, y + height - hpHeight, x - 6, y + height,
                          hpColor, 2.0f);
      }

      if (s.showHealthText) {
        std::string hpStr = std::to_string(player.health) + " HP";
        float textCol[4] = {hpColor[0], hpColor[1], hpColor[2], 1.0f};
        if (s.healthBarStyle == HealthBarStyle::Bottom) {
          drawList.AddText(x, y + height + 9.0f, hpStr.c_str(), textCol);
        } else {
          drawList.AddText(x - 22.0f, y, hpStr.c_str(), textCol);
        }
      }
    }

    if (s.showName) {
      float nCol[4] = {s.nameColor[0], s.nameColor[1], s.nameColor[2],
                       s.nameColor[3]};
      std::string drawName = player.name.empty() ? "Player" : player.name;
      drawList.AddText(x + width / 2.0f - 15.0f, y - 15.0f, drawName.c_str(),
                       nCol);
    }

    if (s.showWeapon && !player.weapon.empty()) {
      float wCol[4] = {s.weaponColor[0], s.weaponColor[1], s.weaponColor[2],
                       s.weaponColor[3]};
      drawList.AddText(x + width / 2.0f - 15.0f, y + height + 2.0f,
                       player.weapon.c_str(), wCol);
    }

    if (s.showDistance && player.distance > 0.0f) {
      float dCol[4] = {s.distColor[0], s.distColor[1], s.distColor[2],
                       s.distColor[3]};
      std::string distStr =
          std::to_string(static_cast<int>(player.distance)) + "m";
      drawList.AddText(x + width - 5.0f, y + height + 2.0f, distStr.c_str(),
                       dCol);
    }

    if (s.showBones && !player.bonePositions.empty() &&
        (s.skeletonMaxDistance <= 0.0f ||
         player.distance <= s.skeletonMaxDistance)) {
      DrawRoundedSkeleton(drawList, player, viewMatrix, screenWidth, screenHeight,
                          s, width);
    }
  }
}

void Esp::RenderUI() {}

} // namespace Features
