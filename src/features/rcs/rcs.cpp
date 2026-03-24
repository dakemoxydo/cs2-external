#include "rcs.h"
#include "rcs_config.h"
#include "config/settings.h"
#include "core/game/game_manager.h"
#include "input/input_manager.h"
#include "core/process/stealth.h"
#include <windows.h>
#include <algorithm>
#include <atomic>

namespace Features {

bool RCSSystem::IsWeaponSupported(const std::string& weaponName) {
    if (weaponName.empty()) return false;
    
    // Convert to lowercase for safety
    std::string w = weaponName;
    std::transform(w.begin(), w.end(), w.begin(), ::tolower);
    
    // Snipers, Pistols, Shotguns should not have RCS
    // Only Rifles, SMGs, Heavy are supported internally
    const char* unsupported[] = {
        "deagle", "elite", "fiveseven", "glock", "taser", "hkp2000", "p250", "cz75a", // pistols
        "revolver", "tec9", "usp_silencer", "ssg08", "awp", "g3sg1", "scar20", // snipers/pistols
        "xm1014", "mag7", "sawedoff", "nova" // shotguns
    };
    
    for (const char* restricted : unsupported) {
        if (w == restricted) return false;
    }
    
    return true; // Supported
}

static SDK::Vector2 s_oldPunch = {0.0f, 0.0f};

void RCSSystem::Update() {
    if (!Config::Settings.rcs.enabled) {
        s_oldPunch = {0.0f, 0.0f};
        return;
    }

    uintptr_t clientBase = Core::GameManager::GetClientBase();
    if (!clientBase) return;

    // Check weapon
    std::string weapon = Core::GameManager::GetLocalWeaponName();
    if (!IsWeaponSupported(weapon)) {
        s_oldPunch = {0.0f, 0.0f};
        return;
    }

    int shotsFired = Core::GameManager::GetLocalShotsFired();
    if (shotsFired < Config::Settings.rcs.startBullet) {
        s_oldPunch = {0.0f, 0.0f};
        return;
    }

    SDK::Vector2 aimPunch = Core::GameManager::GetLocalAimPunch();
    
    // If shooting stopped or punch reset, reset old punch
    if (shotsFired == 0 || (aimPunch.x == 0 && aimPunch.y == 0)) {
        s_oldPunch = {0.0f, 0.0f};
        return;
    }

    // Only applyStandalone RCS when shooting (LBUTTON held) AND aimbot is not currently active
    // because Aimbot already subtracts punch natively for perfect aiming!
    bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool isAimbotting = Config::Settings.aimbot.enabled && (GetAsyncKeyState(Config::Settings.aimbot.hotkey) & 0x8000) != 0;
    
    if (isShooting && !isAimbotting) {
        if (s_oldPunch.x != 0 || s_oldPunch.y != 0) {
            SDK::Vector2 punchDelta = {
                aimPunch.x - s_oldPunch.x,
                aimPunch.y - s_oldPunch.y
            };
            
            // In CS2, punch angle * 2.0 matches the on-screen crosshair pixel movement 
            float pitchComp = punchDelta.x * 2.0f * Config::Settings.rcs.pitchStrength;
            float yawComp = punchDelta.y * 2.0f * Config::Settings.rcs.yawStrength;
            
            // Convert to mouse movements
            // COUNTS_PER_DEG depends on user's in-game sensitivity setting. (Assuming sensitivity = 2.0 for reference)
            const float sens = Config::Settings.aimbot.sensitivity; 
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

void RCSSystem::Render(Render::DrawList &drawList) {
    // Empty
}

} // namespace Features
