#include "footsteps_esp.h"
#include "footsteps_esp_config.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "core/math/math.h"
#include "core/sdk/offsets.h"
#include "core/memory/memory_manager.h"
#include "render/draw/draw_list.h"
#include "render/overlay/overlay.h"
#include <cmath>
#include <imgui.h>

namespace Features {

void FootstepsEsp::Update() {
  if (!Config::Settings.footstepsEsp.enabled)
    return;

  double now = ImGui::GetTime();
  auto &C = Config::Settings.footstepsEsp;
  auto players = Core::GameManager::GetRenderPlayers();
  SDK::Vector3 localPos = Core::GameManager::GetLocalPos();

  for (const auto &player : players) {
    if (!player.IsValid() || !player.IsAlive())
      continue;
    if (player.isTeammate && !C.showTeammates)
      continue;

    uint32_t flags = Core::MemoryManager::Read<uint32_t>(player.address + SDK::Offsets::m_fFlags);
    bool isOnGround = (flags & 1) != 0;

    bool wasOnGround = false;
    auto it = m_prevOnGround.find(player.address);
    if (it != m_prevOnGround.end()) {
      wasOnGround = it->second;
    }

    SDK::Vector3 velocity = Core::MemoryManager::Read<SDK::Vector3>(player.address + SDK::Offsets::m_vecVelocity);
    float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);

    SDK::Vector3 feetPos = player.renderPosition;

    if (!wasOnGround && isOnGround && speed > 80.0f) {
      FootstepRing ring;
      ring.worldPos = feetPos;
      float landRadius = C.landMaxRadius * std::min(1.0f, speed / 400.0f);
      ring.maxRadius = std::max(C.landMaxRadius * 0.5f, landRadius);
      ring.color[0] = C.landColor[0];
      ring.color[1] = C.landColor[1];
      ring.color[2] = C.landColor[2];
      ring.color[3] = C.landColor[3];
      ring.startTime = now;
      m_rings.push_back(ring);
    } else if (wasOnGround && !isOnGround && speed > 50.0f) {
      FootstepRing ring;
      ring.worldPos = feetPos;
      ring.maxRadius = C.jumpMaxRadius;
      ring.color[0] = C.jumpColor[0];
      ring.color[1] = C.jumpColor[1];
      ring.color[2] = C.jumpColor[2];
      ring.color[3] = C.jumpColor[3];
      ring.startTime = now;
      m_rings.push_back(ring);
    } else if (wasOnGround && isOnGround && speed > 50.0f) {
      static std::unordered_map<uintptr_t, double> s_lastFootstepTime;
      double lastTime = 0;
      auto ft = s_lastFootstepTime.find(player.address);
      if (ft != s_lastFootstepTime.end()) {
        lastTime = ft->second;
      }
      if (now - lastTime > 0.25) {
        s_lastFootstepTime[player.address] = now;
        FootstepRing ring;
        ring.worldPos = feetPos;
        float stepRadius = C.footstepMaxRadius * std::min(1.0f, speed / 250.0f);
        ring.maxRadius = std::max(C.footstepMaxRadius * 0.4f, stepRadius);
        ring.color[0] = C.footstepColor[0];
        ring.color[1] = C.footstepColor[1];
        ring.color[2] = C.footstepColor[2];
        ring.color[3] = C.footstepColor[3];
        ring.startTime = now;
        m_rings.push_back(ring);
      }
    }

    m_prevOnGround[player.address] = isOnGround;
  }

  double maxAge = C.expandDuration + C.fadeDuration;
  m_rings.erase(
    std::remove_if(m_rings.begin(), m_rings.end(),
      [now, maxAge](const FootstepRing &r) { return (now - r.startTime) > maxAge; }),
    m_rings.end()
  );
}

void FootstepsEsp::Render(Render::DrawList &drawList) {
  if (!Config::Settings.footstepsEsp.enabled)
    return;

  auto &C = Config::Settings.footstepsEsp;
  const int screenWidth = Render::Overlay::GetGameWidth();
  const int screenHeight = Render::Overlay::GetGameHeight();
  if (screenWidth <= 0 || screenHeight <= 0)
    return;

  const SDK::Matrix4x4 viewMatrix = Core::GameManager::GetViewMatrix();
  double now = ImGui::GetTime();

  for (const auto &ring : m_rings) {
    double elapsed = now - ring.startTime;
    if (elapsed < 0) continue;
    if (elapsed > C.expandDuration + C.fadeDuration) continue;

    float currentRadius;
    float alpha;

    if (elapsed < C.expandDuration) {
      float t = static_cast<float>(elapsed / C.expandDuration);
      currentRadius = ring.maxRadius * t;
      alpha = ring.color[3];
    } else {
      currentRadius = ring.maxRadius;
      float fadeT = static_cast<float>((elapsed - C.expandDuration) / C.fadeDuration);
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
      drawList.DrawCircle(screenPos.x, screenPos.y, screenRadius, color, C.segments, C.thickness);
    }
  }
}

} // namespace Features
