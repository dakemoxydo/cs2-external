#include "rcs.h"
#include "features/feature_frame.h"
#include "rcs_config.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "input/input_manager.h"
#include "core/process/stealth.h"
#include "render/menu/menu.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <shared_mutex>

namespace Features {

bool RCSSystem::IsWeaponSupported(const std::string& weaponName) {
    if (weaponName.empty()) return false;

    std::string w = weaponName;
    std::transform(w.begin(), w.end(), w.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    const char* unsupported[] = {
        "deagle", "elite", "fiveseven", "glock", "taser", "hkp2000", "p250", "cz75a",
        "revolver", "tec9", "usp_silencer", "ssg08", "awp", "g3sg1", "scar20",
        "xm1014", "mag7", "sawedoff", "nova"
    };

    for (const char* restricted : unsupported) {
        if (w == restricted) return false;
    }

    return true;
}

void RCSSystem::ResetState() {
    prevPunch_ = {0.0f, 0.0f};
    smoothX_ = 0.0f;
    smoothY_ = 0.0f;
}

void RCSSystem::OnDisable() { ResetState(); }

void RCSSystem::Update(const FeatureFrame &frame) {
    struct S {
        bool enabled;
        int key, startBullet;
        float pitchStrength, yawStrength, smooth, sensitivity;
    };
    S s;
    {
        const auto &RCS = frame.settings.rcs;
        const auto &A = frame.settings.aimbot;
        s = {RCS.enabled, RCS.key, RCS.startBullet, RCS.pitchStrength,
             RCS.yawStrength, RCS.smooth, A.sensitivity};
    }

    if (!s.enabled) { ResetState(); return; }
    if (Render::Menu::IsOpen()) { ResetState(); return; }
    if (!Input::InputManager::IsKeyDown(s.key)) { ResetState(); return; }

    const auto &snapshot = frame.game;
    if (snapshot.localPawn == 0) { ResetState(); return; }

    const std::string &weapon = snapshot.localWeaponName;
    if (!IsWeaponSupported(weapon)) { ResetState(); return; }

    const int shotsFired = snapshot.localShotsFired;
    const float punchX = snapshot.localAimPunch.x;
    const float punchY = snapshot.localAimPunch.y;

    if (shotsFired <= 1 || shotsFired < s.startBullet) {
        prevPunch_ = {punchX, punchY};
        smoothX_ = 0.0f;
        smoothY_ = 0.0f;
        return;
    }

    const float sensitivity = (s.sensitivity > 0.0f) ? s.sensitivity : 1.0f;
    const float deltaX = (punchX - prevPunch_.x) * -1.0f;
    const float deltaY = (punchY - prevPunch_.y) * -1.0f;

    const float moveX = (deltaY * s.yawStrength * 2.0f / sensitivity) / -0.022f;
    const float moveY = (deltaX * s.pitchStrength * 2.0f / sensitivity) / 0.022f;

    const float factor = std::clamp(s.smooth, 1.0f, 100.0f);
    smoothX_ += (moveX - smoothX_) / factor;
    smoothY_ += (moveY - smoothY_) / factor;

    constexpr float kMaxCompensation = 200.0f;
    smoothX_ = std::clamp(smoothX_, -kMaxCompensation, kMaxCompensation);
    smoothY_ = std::clamp(smoothY_, -kMaxCompensation, kMaxCompensation);

    const int sendX = static_cast<int>(std::round(smoothX_));
    const int sendY = static_cast<int>(std::round(smoothY_));

    if (sendX != 0 || sendY != 0) {
        Input::InputManager::SendMouseDelta(sendX, sendY);
    }

    prevPunch_ = {punchX, punchY};
}

void RCSSystem::Render(const FeatureFrame &, Render::DrawList &) {}

void RCSSystem::RenderUI() {}

} // namespace Features
