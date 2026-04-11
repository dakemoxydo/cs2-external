#include "rcs.h"
#include "rcs_config.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "input/input_manager.h"
#include "core/process/stealth.h"
#include "render/menu/menu.h"
#include <windows.h>
#include <algorithm>
#include <atomic>
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

static SDK::Vector2 s_oldPunch = {0.0f, 0.0f};

void RCSSystem::Update() {
    // Snapshot settings once for consistency
    struct S {
        bool enabled, aimbotEnabled;
        int aimbotHotkey, startBullet;
        float pitchStrength, yawStrength, sensitivity;
    };
    S s;
    {
        std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
        auto &RCS = Config::Settings.rcs;
        auto &A = Config::Settings.aimbot;
        s = {RCS.enabled, A.enabled, A.hotkey, RCS.startBullet,
             RCS.pitchStrength, RCS.yawStrength, A.sensitivity};
    }

    if (!s.enabled) { s_oldPunch = {0.0f, 0.0f}; return; }

    // Do not apply RCS while the menu is open
    if (Render::Menu::IsOpen()) { s_oldPunch = {0.0f, 0.0f}; return; }

    uintptr_t clientBase = Core::GameManager::GetClientBase();
    if (!clientBase) return;

    std::string weapon = Core::GameManager::GetLocalWeaponName();
    if (!IsWeaponSupported(weapon)) { s_oldPunch = {0.0f, 0.0f}; return; }

    int shotsFired = Core::GameManager::GetLocalShotsFired();
    if (shotsFired < s.startBullet) { s_oldPunch = {0.0f, 0.0f}; return; }

    SDK::Vector2 aimPunch = Core::GameManager::GetLocalAimPunch();
    if (shotsFired == 0 || (aimPunch.x == 0 && aimPunch.y == 0)) {
        s_oldPunch = {0.0f, 0.0f};
        return;
    }

    bool isShooting = Input::InputManager::IsKeyDown(VK_LBUTTON);
    bool isAimbotting = s.aimbotEnabled && Input::InputManager::IsKeyDown(s.aimbotHotkey);

    if (isShooting && !isAimbotting) {
        if (s_oldPunch.x != 0 || s_oldPunch.y != 0) {
            SDK::Vector2 punchDelta = {aimPunch.x - s_oldPunch.x, aimPunch.y - s_oldPunch.y};

            float pitchComp = punchDelta.x * 2.0f * s.pitchStrength;
            float yawComp = punchDelta.y * 2.0f * s.yawStrength;

            const float sens = s.sensitivity;
            const float countsPerDeg = 1.0f / (0.022f * sens);

            float mouseDy = pitchComp * countsPerDeg;
            float mouseDx = -yawComp * countsPerDeg;

            int mdx = static_cast<int>(mouseDx);
            int mdy = static_cast<int>(mouseDy);

            if (mdx != 0 || mdy != 0) {
                Input::InputManager::SendMouseDelta(mdx, mdy);
            }
        }
    }

    s_oldPunch = aimPunch;
}

void RCSSystem::Render(Render::DrawList &) {}

void RCSSystem::RenderUI() {}

} // namespace Features
