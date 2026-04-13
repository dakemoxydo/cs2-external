#include "rcs.h"
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

namespace {
SDK::Vector2 s_prevPunch = {0.0f, 0.0f};
float s_smoothX = 0.0f;
float s_smoothY = 0.0f;

void ResetRcsState() {
    s_prevPunch = {0.0f, 0.0f};
    s_smoothX = 0.0f;
    s_smoothY = 0.0f;
}
}

void RCSSystem::Update() {
    struct S {
        bool enabled;
        int key, startBullet;
        float pitchStrength, yawStrength, smooth, sensitivity;
    };
    S s;
    {
        std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
        auto &RCS = Config::Settings.rcs;
        auto &A = Config::Settings.aimbot;
        s = {RCS.enabled, RCS.key, RCS.startBullet, RCS.pitchStrength,
             RCS.yawStrength, RCS.smooth, A.sensitivity};
    }

    if (!s.enabled) { ResetRcsState(); return; }
    if (Render::Menu::IsOpen()) { ResetRcsState(); return; }
    if (!Input::InputManager::IsKeyDown(s.key)) { ResetRcsState(); return; }

    const auto snapshot = Core::GameManager::GetSnapshot();
    if (!snapshot || snapshot->localPawn == 0) { ResetRcsState(); return; }

    const std::string &weapon = snapshot->localWeaponName;
    if (!IsWeaponSupported(weapon)) { ResetRcsState(); return; }

    const int shotsFired = snapshot->localShotsFired;
    const float punchX = snapshot->localAimPunch.x;
    const float punchY = snapshot->localAimPunch.y;

    if (shotsFired <= 1 || shotsFired < s.startBullet) {
        s_prevPunch = {punchX, punchY};
        s_smoothX = 0.0f;
        s_smoothY = 0.0f;
        return;
    }

    const float sensitivity = (s.sensitivity > 0.0f) ? s.sensitivity : 1.0f;
    const float deltaX = (punchX - s_prevPunch.x) * -1.0f;
    const float deltaY = (punchY - s_prevPunch.y) * -1.0f;

    const float moveX = (deltaY * s.pitchStrength * 2.0f / sensitivity) / -0.022f;
    const float moveY = (deltaX * s.yawStrength * 2.0f / sensitivity) / 0.022f;

    const float factor = std::clamp(s.smooth, 1.0f, 100.0f);
    s_smoothX += (moveX - s_smoothX) / factor;
    s_smoothY += (moveY - s_smoothY) / factor;

    constexpr float kMaxCompensation = 200.0f;
    s_smoothX = std::clamp(s_smoothX, -kMaxCompensation, kMaxCompensation);
    s_smoothY = std::clamp(s_smoothY, -kMaxCompensation, kMaxCompensation);

    const int sendX = static_cast<int>(std::round(s_smoothX));
    const int sendY = static_cast<int>(std::round(s_smoothY));

    if (sendX != 0 || sendY != 0) {
        Input::InputManager::SendMouseDelta(sendX, sendY);
    }

    s_prevPunch = {punchX, punchY};
}

void RCSSystem::Render(Render::DrawList &) {}

void RCSSystem::RenderUI() {}

} // namespace Features
