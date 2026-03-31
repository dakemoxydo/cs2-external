#pragma once
#include <string>
#include <unordered_map>

namespace Features {

struct SkinEntry {
    int weaponIndex = 0;
    int paintKit = 0;       // ID скина
    int seed = 0;           // Seed скина
    float wear = 0.0001f;   // Износ (0.0001 = Factory New)
    int statTrak = -1;      // -1 = отключён
    std::string skinName = "";
    std::string weaponName = "";
};

struct MusicKitEntry {
    int musicKit = 0;       // ID музыкального набора
    std::string musicKitName = "";
};

struct SkinchangerConfig {
    bool enabled = false;
    
    // Скины для оружия (key = weapon definition index)
    std::unordered_map<int, SkinEntry> weaponSkins;
    
    // Музыкальный набор
    MusicKitEntry musicKit;
    
    // Скин для ножа
    SkinEntry knifeSkin;
    bool knifeEnabled = false;
    
    // Скин для перчаток
    SkinEntry glovesSkin;
    bool glovesEnabled = false;
    
    // Применять скины
    bool applySkins = false;
    
    // Автоприменение при смене оружия
    bool autoApply = true;
};

} // namespace Features
