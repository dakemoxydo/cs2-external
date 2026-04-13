#include "sound_esp.h"
#include "sound_esp_config.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "render/draw/draw_list.h"
#include "render/overlay/overlay.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <shared_mutex>

namespace Features {

namespace {

constexpr float kGroundLift = 2.0f;
constexpr size_t kMaxActiveRings = 48;

struct SoundEspSnapshot {
  bool enabled = false;
  bool showTeammates = false;
  float footstepColor[4] = {};
  float jumpColor[4] = {};
  float landColor[4] = {};
  float footstepMaxRadius = 1.0f;
  float jumpMaxRadius = 1.0f;
  float landMaxRadius = 1.0f;
  float expandDuration = 0.01f;
  float fadeDuration = 0.01f;
  float thickness = 0.1f;
  int segments = 3;
};

float EaseOutQuad(float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t);
}

SoundEspSnapshot SnapshotSoundEsp() {
  SoundEspSnapshot snapshot;
  std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
  auto &settings = Config::Settings.soundEsp;

  snapshot.enabled = settings.enabled;
  snapshot.showTeammates = settings.showTeammates;
  snapshot.footstepMaxRadius = (std::max)(1.0f, settings.footstepMaxRadius);
  snapshot.jumpMaxRadius = (std::max)(1.0f, settings.jumpMaxRadius);
  snapshot.landMaxRadius = (std::max)(1.0f, settings.landMaxRadius);
  snapshot.expandDuration = (std::max)(0.01f, settings.expandDuration);
  snapshot.fadeDuration = (std::max)(0.01f, settings.fadeDuration);
  snapshot.thickness = (std::max)(0.1f, settings.thickness);
  snapshot.segments = (std::max)(12, settings.segments);

  std::copy(std::begin(settings.footstepColor), std::end(settings.footstepColor),
            snapshot.footstepColor);
  std::copy(std::begin(settings.jumpColor), std::end(settings.jumpColor),
            snapshot.jumpColor);
  std::copy(std::begin(settings.landColor), std::end(settings.landColor),
            snapshot.landColor);

  return snapshot;
}

bool ProjectWorldPoint(const SDK::Vector3 &worldPos,
                       const SDK::Matrix4x4 &viewMatrix, int screenWidth,
                       int screenHeight, SDK::Vector2 &screenPos) {
  return Core::Math::WorldToScreen(worldPos, screenPos, viewMatrix, screenWidth,
                                   screenHeight);
}

void DrawProjectedRing(Render::DrawList &drawList, const SDK::Vector3 &center,
                       float radius, float zOffset, float color[4], int segments,
                       float thickness, const SDK::Matrix4x4 &viewMatrix,
                       int screenWidth, int screenHeight) {
  if (radius <= 0.1f || segments < 3) {
    return;
  }

  constexpr float kTwoPi = 6.28318530718f;
  std::vector<SDK::Vector2> projectedPoints(static_cast<size_t>(segments) + 1);
  std::vector<bool> projectedValid(static_cast<size_t>(segments) + 1, false);
  for (int i = 0; i <= segments; ++i) {
    const float angle =
        (static_cast<float>(i) / static_cast<float>(segments)) * kTwoPi;
    SDK::Vector3 worldPoint = {
        center.x + std::cos(angle) * radius,
        center.y + std::sin(angle) * radius,
        center.z + zOffset,
    };

    SDK::Vector2 screenPoint = {};
    if (ProjectWorldPoint(worldPoint, viewMatrix, screenWidth, screenHeight,
                          screenPoint)) {
      projectedPoints[static_cast<size_t>(i)] = screenPoint;
      projectedValid[static_cast<size_t>(i)] = true;
    }
  }

  for (int i = 1; i <= segments; ++i) {
    if (!projectedValid[static_cast<size_t>(i - 1)] ||
        !projectedValid[static_cast<size_t>(i)]) {
      continue;
    }

    const SDK::Vector2 &a = projectedPoints[static_cast<size_t>(i - 1)];
    const SDK::Vector2 &b = projectedPoints[static_cast<size_t>(i)];
    drawList.DrawLine(a.x, a.y, b.x, b.y, color, thickness);
  }

}

void DrawProjectedConnectors(Render::DrawList &drawList,
                             const SDK::Vector3 &center, float radius,
                             float height, float color[4], int connectorCount,
                             float thickness, const SDK::Matrix4x4 &viewMatrix,
                             int screenWidth, int screenHeight) {
  if (radius <= 0.1f || height <= 0.1f || connectorCount <= 0) {
    return;
  }

  constexpr float kTwoPi = 6.28318530718f;
  for (int i = 0; i < connectorCount; ++i) {
    const float angle =
        (static_cast<float>(i) / static_cast<float>(connectorCount)) * kTwoPi;
    SDK::Vector3 lower = {
        center.x + std::cos(angle) * radius,
        center.y + std::sin(angle) * radius,
        center.z,
    };
    SDK::Vector3 upper = lower;
    upper.z += height;

    SDK::Vector2 lowerScreen = {};
    SDK::Vector2 upperScreen = {};
    if (ProjectWorldPoint(lower, viewMatrix, screenWidth, screenHeight,
                          lowerScreen) &&
        ProjectWorldPoint(upper, viewMatrix, screenWidth, screenHeight,
                          upperScreen)) {
      drawList.DrawLine(lowerScreen.x, lowerScreen.y, upperScreen.x,
                        upperScreen.y, color, thickness);
    }
  }
}

SoundRing MakeRing(const SDK::Vector3 &origin, const float sourceColor[4],
                   float maxRadius, float waveHeight, float startTime) {
  SoundRing ring{};
  ring.worldPos = origin;
  ring.worldPos.z += kGroundLift;
  ring.maxRadius = (std::max)(1.0f, maxRadius);
  ring.waveHeight = (std::max)(2.0f, waveHeight);
  ring.startTime = startTime;
  std::copy(sourceColor, sourceColor + 4, ring.color);
  return ring;
}

} // namespace

void SoundEsp::ResetState() {
  m_rings.clear();
  m_lastAudioEventId = 0;
  m_lastObservedLocalPawn = 0;
}

void SoundEsp::Update() {
  const SoundEspSnapshot snapshot = SnapshotSoundEsp();
  if (!snapshot.enabled) {
    ResetState();
    return;
  }

  const auto gameSnapshot = Core::GameManager::GetSnapshot();
  if (!gameSnapshot) {
    ResetState();
    return;
  }

  if (gameSnapshot->localPawn == 0) {
    ResetState();
    return;
  }

  if (gameSnapshot->localPawn != m_lastObservedLocalPawn) {
    m_rings.clear();
    m_lastAudioEventId = 0;
    m_lastObservedLocalPawn = gameSnapshot->localPawn;
  }

  const float now = gameSnapshot->frameTimeSeconds;
  for (const auto &event : gameSnapshot->movementAudioEvents) {
    if (event.id <= m_lastAudioEventId) {
      continue;
    }
    if (event.isTeammate && !snapshot.showTeammates) {
      m_lastAudioEventId = std::max(m_lastAudioEventId, event.id);
      continue;
    }

    const float strength = std::clamp(event.strength, 0.25f, 1.35f);
    switch (event.type) {
    case SDK::MovementAudioType::Jump:
      m_rings.push_back(MakeRing(
          event.origin, snapshot.jumpColor, snapshot.jumpMaxRadius * strength,
          10.0f + strength * 8.0f, now));
      break;
    case SDK::MovementAudioType::Land:
      m_rings.push_back(MakeRing(
          event.origin, snapshot.landColor,
          (std::max)(snapshot.landMaxRadius * 0.5f,
                     snapshot.landMaxRadius * strength),
          12.0f + strength * 10.0f, now));
      break;
    default:
      m_rings.push_back(MakeRing(
          event.origin, snapshot.footstepColor,
          (std::max)(snapshot.footstepMaxRadius * 0.4f,
                     snapshot.footstepMaxRadius * strength),
          8.0f + strength * 6.0f, now));
      break;
    }

    m_lastAudioEventId = std::max(m_lastAudioEventId, event.id);
  }

  if (m_rings.size() > kMaxActiveRings) {
    m_rings.erase(m_rings.begin(),
                  m_rings.begin() +
                      static_cast<std::ptrdiff_t>(m_rings.size() - kMaxActiveRings));
  }

  const float maxAge = snapshot.expandDuration + snapshot.fadeDuration;
  m_rings.erase(std::remove_if(m_rings.begin(), m_rings.end(),
                               [now, maxAge](const SoundRing &ring) {
                                 return (now - ring.startTime) > maxAge;
                               }),
                m_rings.end());
}

void SoundEsp::Render(Render::DrawList &drawList) {
  const SoundEspSnapshot snapshot = SnapshotSoundEsp();
  if (!snapshot.enabled) {
    return;
  }

  const auto gameSnapshot = Core::GameManager::GetSnapshot();
  if (!gameSnapshot) {
    return;
  }

  const int screenWidth = Render::Overlay::GetGameWidth();
  const int screenHeight = Render::Overlay::GetGameHeight();
  if (screenWidth <= 0 || screenHeight <= 0) {
    return;
  }

  const SDK::Matrix4x4 &viewMatrix = gameSnapshot->viewMatrix;
  const float now = gameSnapshot->frameTimeSeconds;

  for (const auto &ring : m_rings) {
    const double elapsed = now - ring.startTime;
    if (elapsed < 0.0 ||
        elapsed > snapshot.expandDuration + snapshot.fadeDuration) {
      continue;
    }

    float currentRadius = ring.maxRadius;
    float alpha = ring.color[3];
    float waveHeight = ring.waveHeight;
    float upperAlpha = alpha * 0.5f;

    if (elapsed < snapshot.expandDuration) {
      const float t = static_cast<float>(elapsed / snapshot.expandDuration);
      const float eased = EaseOutQuad(t);
      currentRadius = ring.maxRadius * eased;
      alpha = ring.color[3] * (0.55f + 0.45f * (1.0f - t * 0.3f));
      waveHeight = ring.waveHeight * (1.0f - t * 0.35f);
      upperAlpha = alpha * 0.55f;
    } else {
      const float fadeT =
          static_cast<float>((elapsed - snapshot.expandDuration) /
                             snapshot.fadeDuration);
      alpha = ring.color[3] * (1.0f - fadeT);
      waveHeight = ring.waveHeight * (1.0f - fadeT * 0.7f);
      upperAlpha = alpha * 0.45f;
    }

    if (alpha < 0.01f || currentRadius <= 0.1f) {
      continue;
    }

    float baseColor[4] = {ring.color[0], ring.color[1], ring.color[2], alpha};
    float upperColor[4] = {ring.color[0], ring.color[1], ring.color[2],
                           upperAlpha};

    DrawProjectedRing(drawList, ring.worldPos, currentRadius, 0.0f, baseColor,
                      snapshot.segments, snapshot.thickness, viewMatrix,
                      screenWidth, screenHeight);
    DrawProjectedRing(drawList, ring.worldPos, currentRadius, waveHeight,
                      upperColor, snapshot.segments,
                      (std::max)(1.0f, snapshot.thickness - 0.25f), viewMatrix,
                      screenWidth, screenHeight);
    DrawProjectedConnectors(
        drawList, ring.worldPos, currentRadius, waveHeight, upperColor,
        (std::max)(4, snapshot.segments / 6),
        (std::max)(1.0f, snapshot.thickness - 0.4f), viewMatrix, screenWidth,
        screenHeight);
  }
}

void SoundEsp::RenderUI() {}

} // namespace Features
