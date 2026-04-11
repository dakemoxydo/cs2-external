#include "sound_esp.h"
#include "sound_esp_config.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/sdk/offsets.h"
#include "render/draw/draw_list.h"
#include "render/overlay/overlay.h"
#include <cmath>
#include <imgui.h>
#include <shared_mutex>

namespace Features {

struct SoundEspSnapshot {
  bool enabled, showTeammates;
  float footstepColor[4], jumpColor[4], landColor[4];
  float footstepMaxRadius, jumpMaxRadius, landMaxRadius;
  float expandDuration, fadeDuration, thickness;
  int segments;
};

static SoundEspSnapshot SnapshotSoundEsp() {
  SoundEspSnapshot s;
  std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
  auto &SE = Config::Settings.soundEsp;
  s.enabled = SE.enabled;
  s.showTeammates = SE.showTeammates;
  s.footstepMaxRadius = std::max(1.0f, SE.footstepMaxRadius);
  s.jumpMaxRadius = std::max(1.0f, SE.jumpMaxRadius);
  s.landMaxRadius = std::max(1.0f, SE.landMaxRadius);
  s.expandDuration = std::max(0.01f, SE.expandDuration);
  s.fadeDuration = std::max(0.01f, SE.fadeDuration);
  s.thickness = std::max(0.1f, SE.thickness);
  s.segments = std::max(3, SE.segments);
  std::copy(std::begin(SE.footstepColor), std::end(SE.footstepColor), s.footstepColor);
  std::copy(std::begin(SE.jumpColor), std::end(SE.jumpColor), s.jumpColor);
  std::copy(std::begin(SE.landColor), std::end(SE.landColor), s.landColor);
  return s;
}

void SoundEsp::Update() {
  SoundEspSnapshot s = SnapshotSoundEsp();
  if (!s.enabled) {
    m_rings.clear();
    m_prevOnGround.clear();
    return;
  }

  double now = ImGui::GetTime();
  const auto players = Core::GameManager::GetSnapshot()->players;

  // Persistent footstep debounce map (pruned at end of Update)
  static std::unordered_map<uint32_t, double> s_lastFootstepTime;

  for (const auto &player : players) {
    if (!player.IsValid() || !player.IsAlive()) continue;
    if (player.isTeammate && !s.showTeammates) continue;

    // Read pre-computed state from snapshot (computed in memory thread)
    bool isOnGround = player.isOnGround;
    float speed = player.speed;

    bool wasOnGround = false;
    auto it = m_prevOnGround.find(player.pawnHandle);
    if (it != m_prevOnGround.end()) {
      wasOnGround = it->second;
    }

    SDK::Vector3 feetPos = player.renderPosition;

    // Landing detection
    if (!wasOnGround && isOnGround && speed > 80.0f && speed < 600.0f) {
      SoundRing ring;
      ring.worldPos = feetPos;
      float landRadius = s.landMaxRadius * std::min(1.0f, speed / 400.0f);
      ring.maxRadius = std::max(s.landMaxRadius * 0.5f, landRadius);
      ring.color[0] = s.landColor[0];
      ring.color[1] = s.landColor[1];
      ring.color[2] = s.landColor[2];
      ring.color[3] = s.landColor[3];
      ring.startTime = now;
      m_rings.push_back(ring);
    }
    // Jump detection
    else if (wasOnGround && !isOnGround && speed > 50.0f && speed < 300.0f) {
      SoundRing ring;
      ring.worldPos = feetPos;
      ring.maxRadius = s.jumpMaxRadius;
      ring.color[0] = s.jumpColor[0];
      ring.color[1] = s.jumpColor[1];
      ring.color[2] = s.jumpColor[2];
      ring.color[3] = s.jumpColor[3];
      ring.startTime = now;
      m_rings.push_back(ring);
    }
    // Footstep detection
    else if (wasOnGround && isOnGround && speed > 50.0f && speed < 350.0f) {
      double lastTime = 0;
      auto ft = s_lastFootstepTime.find(player.pawnHandle);
      if (ft != s_lastFootstepTime.end()) {
        lastTime = ft->second;
      }
      if (now - lastTime > 0.25) {
        s_lastFootstepTime[player.pawnHandle] = now;
        SoundRing ring;
        ring.worldPos = feetPos;
        float stepRadius = s.footstepMaxRadius * std::min(1.0f, speed / 250.0f);
        ring.maxRadius = std::max(s.footstepMaxRadius * 0.4f, stepRadius);
        ring.color[0] = s.footstepColor[0];
        ring.color[1] = s.footstepColor[1];
        ring.color[2] = s.footstepColor[2];
        ring.color[3] = s.footstepColor[3];
        ring.startTime = now;
        m_rings.push_back(ring);
      }
    }

    m_prevOnGround[player.pawnHandle] = isOnGround;
  }

  // Prune expired rings
  double maxAge = s.expandDuration + s.fadeDuration;
  m_rings.erase(
    std::remove_if(m_rings.begin(), m_rings.end(),
      [now, maxAge](const SoundRing &r) { return (now - r.startTime) > maxAge; }),
    m_rings.end()
  );

  // Prune stale m_prevOnGround entries
  for (auto it = m_prevOnGround.begin(); it != m_prevOnGround.end();) {
    bool found = false;
    for (const auto &player : players) {
      if (player.pawnHandle == it->first) { found = true; break; }
    }
    if (!found) {
      it = m_prevOnGround.erase(it);
    } else {
      ++it;
    }
  }

  // Prune stale s_lastFootstepTime entries (same active set)
  for (auto it = s_lastFootstepTime.begin(); it != s_lastFootstepTime.end();) {
    bool found = false;
    for (const auto &player : players) {
      if (player.pawnHandle == it->first) { found = true; break; }
    }
    if (!found) {
      it = s_lastFootstepTime.erase(it);
    } else {
      ++it;
    }
  }
}

void SoundEsp::Render(Render::DrawList &drawList) {
  SoundEspSnapshot s = SnapshotSoundEsp();
  if (!s.enabled) return;

  const int screenWidth = Render::Overlay::GetGameWidth();
  const int screenHeight = Render::Overlay::GetGameHeight();
  if (screenWidth <= 0 || screenHeight <= 0) return;

  const SDK::Matrix4x4 viewMatrix = Core::GameManager::GetViewMatrix();
  double now = ImGui::GetTime();

  for (const auto &ring : m_rings) {
    double elapsed = now - ring.startTime;
    if (elapsed < 0) continue;
    if (elapsed > s.expandDuration + s.fadeDuration) continue;

    float currentRadius;
    float alpha;

    if (elapsed < s.expandDuration) {
      float t = static_cast<float>(elapsed / s.expandDuration);
      currentRadius = ring.maxRadius * t;
      alpha = ring.color[3];
    } else {
      currentRadius = ring.maxRadius;
      float fadeT = static_cast<float>((elapsed - s.expandDuration) / s.fadeDuration);
      alpha = ring.color[3] * (1.0f - fadeT);
    }

    if (alpha < 0.01f) continue;

    SDK::Vector2 screenPos;
    if (!Core::Math::WorldToScreen(ring.worldPos, screenPos, viewMatrix, screenWidth, screenHeight))
      continue;

    SDK::Vector3 offsetPos = {ring.worldPos.x + currentRadius, ring.worldPos.y, ring.worldPos.z};
    SDK::Vector2 screenOffset;
    if (Core::Math::WorldToScreen(offsetPos, screenOffset, viewMatrix, screenWidth, screenHeight)) {
      float screenRadius = std::abs(screenOffset.x - screenPos.x);
      if (screenRadius < 1.0f) screenRadius = 1.0f;

      float color[4] = {ring.color[0], ring.color[1], ring.color[2], alpha};
      drawList.DrawCircle(screenPos.x, screenPos.y, screenRadius, color, s.segments, s.thickness);
    }
  }
}

void SoundEsp::RenderUI() {}

} // namespace Features
