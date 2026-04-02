#pragma once
#include "features/feature_base.h"
#include "skinchanger_config.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Render {
class DrawList;
}

namespace Features {

// Определение индексов оружия CS2
enum class WeaponIndex : int {
    Deagle = 1,
    Elite = 2,
    Fiveseven = 3,
    Glock = 4,
    Ak47 = 7,
    Aug = 8,
    Awp = 9,
    Famas = 10,
    G3sg1 = 11,
    Galilar = 13,
    M249 = 14,
    M4a1 = 16,
    Mac10 = 17,
    P90 = 19,
    Mp5sd = 23,
    Ump45 = 24,
    Xm1014 = 25,
    Bizon = 26,
    Mag7 = 27,
    Negev = 28,
    Sawedoff = 29,
    Tec9 = 30,
    Taser = 31,
    Hkp2000 = 32,
    Mp7 = 33,
    Mp9 = 34,
    Nova = 35,
    P250 = 36,
    Scar20 = 38,
    Sg556 = 39,
    Ssg08 = 40,
    Knife = 42,
    Flashbang = 43,
    Hegrenade = 44,
    Smokegrenade = 45,
    Molotov = 46,
    Decoy = 47,
    Incgrenade = 48,
    C4 = 49,
    KnifeT = 59,
    M4a1Silencer = 60,
    UspSilencer = 61,
    Cz75a = 63,
    Revolver = 64,
    Bayonet = 500,
    KnifeFlip = 505,
    KnifeGut = 506,
    KnifeKarambit = 507,
    KnifeM9Bayonet = 508,
    KnifeTactical = 509,
    KnifeFalchion = 512,
    KnifeDagger = 514,
    KnifeButterfly = 515,
    KnifePush = 516,
    KnifeUrsus = 519,
    KnifeGypsyJackknife = 520,
    KnifeStiletto = 522,
    KnifeWidowmaker = 523,
    KnifeSkeleton = 525,
    KnifeKukri = 526,
    Gloves = 5027,
};

// Структура для хранения информации о скинах
struct SkinDefinition {
    int paintKit;
    const char* name;
    const char* displayName;
};

struct WeaponDefinition {
    int definitionIndex;
    const char* name;
    const char* displayName;
    std::vector<SkinDefinition> skins;
};

class Skinchanger : public IFeature {
public:
    Skinchanger();
    ~Skinchanger() override = default;

    void Update() override;
    void Render(Render::DrawList& drawList) override;
    const char* GetName() override { return "Skinchanger"; }

    void OnEnable() override;
    void OnDisable() override;

    // Публичные методы для доступа к данным
    SkinchangerConfig& GetConfig() { return m_config; }
    const std::vector<WeaponDefinition>& GetWeaponDefinitions() const { return m_weaponDefinitions; }

    // Применить скины к оружию
    void ApplySkins();
    
    // Загрузить скины из базы данных
    void LoadSkinDatabase();

private:
    SkinchangerConfig m_config;
    
    // База данных скинов
    std::vector<WeaponDefinition> m_weaponDefinitions;
    
    // Инициализация базы данных скинов
    void InitializeSkinDatabase();
    
    // Получить скин для оружия
    SkinEntry GetSkinForWeapon(int weaponIndex);
    
    // Применить скин к оружию
    bool ApplySkinToWeapon(uintptr_t weaponEntity, const SkinEntry& skin);
    
    // Обновить скины для всех оружий
    void UpdateWeaponSkins();
};

} // namespace Features
