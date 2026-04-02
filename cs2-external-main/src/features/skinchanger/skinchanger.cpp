#include "skinchanger.h"
#include "core/memory/memory_manager.h"
#include "core/game/game_manager.h"
#include "core/sdk/offsets.h"
#include "core/sdk/entity.h"
#include "render/draw/draw_list.h"
#include <algorithm>
#include <iostream>

namespace Features {

Skinchanger::Skinchanger() {
    InitializeSkinDatabase();
}

void Skinchanger::OnEnable() {
    m_config.enabled = true;
    std::cout << "[Skinchanger] Enabled" << std::endl;
}

void Skinchanger::OnDisable() {
    m_config.enabled = false;
    std::cout << "[Skinchanger] Disabled" << std::endl;
}

void Skinchanger::Update() {
    if (!m_config.enabled) {
        return;
    }

    if (m_config.applySkins) {
        ApplySkins();
        m_config.applySkins = false; // Сбросить флаг после применения
    }

    if (m_config.autoApply) {
        UpdateWeaponSkins();
    }
}

void Skinchanger::Render(Render::DrawList& drawList) {
    // Skinchanger не рендерит ничего на экран
    // UI обрабатывается через меню
}

void Skinchanger::ApplySkins() {
    std::cout << "[Skinchanger] Applying skins..." << std::endl;
    UpdateWeaponSkins();
}

void Skinchanger::UpdateWeaponSkins() {
    // Получаем локального игрока
    uintptr_t localPawn = Core::GameManager::GetLocalPlayerPawn();
    if (!localPawn) {
        return;
    }

    // Получаем список игроков
    auto players = Core::GameManager::GetRenderPlayers();
    
    for (auto& player : players) {
        if (!player.IsValid() || !player.IsAlive()) {
            continue;
        }

        // Проверяем, есть ли скин для текущего оружия игрока
        // В реальном проекте здесь нужно получить entity оружия и применить скин
        // Для сейчас это заглушка - полная реализация требует оффсетов для оружия
    }
}

bool Skinchanger::ApplySkinToWeapon(uintptr_t weaponEntity, const SkinEntry& skin) {
    if (!weaponEntity || skin.paintKit == 0) {
        return false;
    }

    // В CS2 скины применяются через систему EconItem
    // Основные оффсеты для изменения скинов:
    // - m_AttributeManager (CEconItemAttributeManager)
    // - m_Item (CEconItemView)
    // - m_iItemIDHigh
    // - m_iAccountID
    // - m_iEntityQuality
    // - m_OriginalOwnerXuidLow
    // - m_OriginalOwnerXuidHigh
    // - m_nFallbackPaintKit
    // - m_nFallbackSeed
    // - m_flFallbackWear
    // - m_nFallbackStatTrak

    // Записываем fallback paint kit
    uintptr_t fallbackPaintKitOffset = 0x0; // Будет заполнено из offsets.h
    if (fallbackPaintKitOffset > 0) {
        Core::MemoryManager::Write<int>(weaponEntity + fallbackPaintKitOffset, skin.paintKit);
    }

    // Записываем fallback seed
    uintptr_t fallbackSeedOffset = 0x0;
    if (fallbackSeedOffset > 0) {
        Core::MemoryManager::Write<int>(weaponEntity + fallbackSeedOffset, skin.seed);
    }

    // Записываем fallback wear
    uintptr_t fallbackWearOffset = 0x0;
    if (fallbackWearOffset > 0) {
        Core::MemoryManager::Write<float>(weaponEntity + fallbackWearOffset, skin.wear);
    }

    // Записываем fallback stattrak
    uintptr_t fallbackStatTrakOffset = 0x0;
    if (fallbackStatTrakOffset > 0) {
        Core::MemoryManager::Write<int>(weaponEntity + fallbackStatTrakOffset, skin.statTrak);
    }

    // Force update - записываем itemIDHigh = -1 для принудительного обновления
    uintptr_t itemIDHighOffset = 0x0;
    if (itemIDHighOffset > 0) {
        Core::MemoryManager::Write<int>(weaponEntity + itemIDHighOffset, -1);
    }

    return true;
}

void Skinchanger::InitializeSkinDatabase() {
    // Инициализация базы данных скинов
    // Это упрощённая версия - в реальном проекте скины загружаются из JSON
    
    // AK-47 скины
    WeaponDefinition ak47;
    ak47.definitionIndex = static_cast<int>(WeaponIndex::Ak47);
    ak47.name = "weapon_ak47";
    ak47.displayName = "AK-47";
    ak47.skins = {
        {0, "default", "Default"},
        {180, "asiimov", "Asiimov"},
        {226, "vulcan", "Vulcan"},
        {282, "fire_serpent", "Fire Serpent"},
        {302, "hydroponic", "Hydroponic"},
        {316, "case_hardened", "Case Hardened"},
        {340, "bloodsport", "Bloodsport"},
        {380, "neon_rider", "Neon Rider"},
        {422, "the_emperor", "The Emperor"},
        {456, "phantom_disruptor", "Phantom Disruptor"},
        {474, "ice_coaled", "Ice Coaled"},
        {506, "slipstream", "Slipstream"},
        {524, "frontside_misty", "Frontside Misty"},
        {600, "aquamarine_revenge", "Aquamarine Revenge"},
        {639, "redline", "Redline"},
        {675, "wasteland_rebel", "Wasteland Rebel"},
        {707, "fuel_injector", "Fuel Injector"},
        {724, "orbit_mk01", "Orbit MK01"},
        {745, "point_disarray", "Point Disarray"},
        {799, "safety_net", "Safety Net"},
        {801, "inheritance", "Inheritance"},
        {836, "rat_rod", "Rat Rod"},
        {885, "green_laminate", "Green Laminate"},
        {922, "nightwish", "Nightwish"},
        {941, "ice_coaled", "Ice Coaled"},
        {1004, "head_shot", "Head Shot"},
        {1018, "safari_mesh", "Safari Mesh"},
        {1035, "jaguar", "Jaguar"},
        {1070, "bravo", "Bravo"},
        {1121, "x_ray", "X-Ray"},
        {1143, "blue_laminate", "Blue Laminate"},
        {1165, "legion_of_anubis", "Legion of Anubis"},
        {1221, "gold_ara", "Gold Arabesque"},
        {1238, "panthera_onca", "Panthera Onca"},
        {1255, "olive_polis", "Olive Polyspace"},
        {1283, "wild_lotus", "Wild Lotus"},
    };
    m_weaponDefinitions.push_back(ak47);

    // M4A4 скины
    WeaponDefinition m4a4;
    m4a4.definitionIndex = static_cast<int>(WeaponIndex::M4a1);
    m4a4.name = "weapon_m4a1";
    m4a4.displayName = "M4A4";
    m4a4.skins = {
        {0, "default", "Default"},
        {155, "asiimov", "Asiimov"},
        {187, "howl", "Howl"},
        {215, "imperial_dragon", "Imperial Dragon"},
        {255, "cyber_security", "Cyber Security"},
        {309, "desolate_space", "Desolate Space"},
        {336, "daybreak", "Daybreak"},
        {384, "buzz_kill", "Buzz Kill"},
        {400, "dragon_king", "Dragon King"},
        {449, "griffin", "Griffin"},
        {471, "in_living_color", "In Living Color"},
        {480, "hellfire", "Hellfire"},
        {512, "emperor", "Emperor"},
        {533, "neo_noir", "Neo-Noir"},
        {588, "royal_palace", "Royal Palace"},
        {632, "tooth_fairy", "Tooth Fairy"},
        {664, "mainframe", "Mainframe"},
        {694, "desolate_space", "Desolate Space"},
        {722, "the_battlestar", "The Battlestar"},
        {756, "clutch", "Clutch"},
        {794, "polymer", "Polymer"},
        {828, "converter", "Converter"},
        {872, "global_offensive", "Global Offensive"},
        {917, "temukau", "Temukau"},
        {967, "cyberpunk", "Cyberpunk"},
        {1000, "chopper", "Chopper"},
        {1043, "dark_angel", "Dark Angel"},
        {1091, "motherboard", "Motherboard"},
        {1137, "eye_of_horuz", "Eye of Horus"},
        {1167, "zodiac", "Zodiac"},
        {1203, "light_box", "Light Box"},
        {1249, "the_emperor", "The Emperor"},
    };
    m_weaponDefinitions.push_back(m4a4);

    // M4A1-S скины
    WeaponDefinition m4a1s;
    m4a1s.definitionIndex = static_cast<int>(WeaponIndex::M4a1Silencer);
    m4a1s.name = "weapon_m4a1_silencer";
    m4a1s.displayName = "M4A1-S";
    m4a1s.skins = {
        {0, "default", "Default"},
        {189, "atomic_alloy", "Atomic Alloy"},
        {225, "guardian", "Guardian"},
        {254, "cyrex", "Cyrex"},
        {290, "hyper_beast", "Hyper Beast"},
        {321, "golden_coil", "Golden Coil"},
        {361, "nightmare", "Nightmare"},
        {389, "mecha_industries", "Mecha Industries"},
        {430, "decimator", "Decimator"},
        {440, "bright_water", "Bright Water"},
        {473, "blood_tiger", "Blood Tiger"},
        {497, "hot_rod", "Hot Rod"},
        {525, "master_piece", "Master Piece"},
        {548, "chanticos_fire", "Chantico's Fire"},
        {587, "kill_confirmed", "Kill Confirmed"},
        {630, "leaded_glass", "Leaded Glass"},
        {662, "player_two", "Player Two"},
        {692, "briefing", "Briefing"},
        {720, "blue_phosphor", "Blue Phosphor"},
        {754, "flashback", "Flashback"},
        {792, "printstream", "Printstream"},
        {826, "nightmare", "Nightmare"},
        {870, "control_panel", "Control Panel"},
        {915, "welcome_to_the_jungle", "Welcome to the Jungle"},
        {965, "black_lotus", "Black Lotus"},
        {998, "decimator", "Decimator"},
        {1041, "moss_galaxy", "Moss Galaxy"},
        {1089, "blueberry_gum", "Blueberry Gum"},
        {1135, "purple_ddpat", "Purple DD PAT"},
        {1165, "midnight_storm", "Midnight Storm"},
        {1201, "danger_close", "Danger Close"},
        {1247, "black_lotus", "Black Lotus"},
    };
    m_weaponDefinitions.push_back(m4a1s);

    // AWP скины
    WeaponDefinition awp;
    awp.definitionIndex = static_cast<int>(WeaponIndex::Awp);
    awp.name = "weapon_awp";
    awp.displayName = "AWP";
    awp.skins = {
        {0, "default", "Default"},
        {181, "asiimov", "Asiimov"},
        {212, "dragon_lore", "Dragon Lore"},
        {251, "hyper_beast", "Hyper Beast"},
        {279, "lightning_strike", "Lightning Strike"},
        {300, "corticera", "Corticera"},
        {326, "redline", "Redline"},
        {344, "electric_hive", "Electric Hive"},
        {375, "graphite", "Graphite"},
        {395, "boom", "Boom"},
        {424, "pit_viper", "Pit Viper"},
        {446, "containment_breach", "Containment Breach"},
        {451, "manowar", "Man-o'-war"},
        {475, "phobos", "Phobos"},
        {500, "worm_god", "Worm God"},
        {527, "medusa", "Medusa"},
        {550, "oni_taiji", "Oni Taiji"},
        {589, "neon_noir", "Neon Noir"},
        {620, "the_wildfire", "The Wildfire"},
        {652, "fade", "Fade"},
        {684, "safari_mesh", "Safari Mesh"},
        {714, "sun_in_leo", "Sun in Leo"},
        {736, "elite_build", "Elite Build"},
        {758, "fever_dream", "Fever Dream"},
        {789, "prince", "Prince"},
        {823, "neo_noir", "Neo-Noir"},
        {867, "mortis", "Mortis"},
        {912, "the_prince", "The Prince"},
        {962, "containment_breach", "Containment Breach"},
        {995, "chromatic_aberration", "Chromatic Aberration"},
        {1038, "duality", "Duality"},
        {1086, "silk_tiger", "Silk Tiger"},
        {1132, "gungnir", "Gungnir"},
        {1162, "rattlesnake", "Rattlesnake"},
        {1198, "wildfire", "Wildfire"},
        {1244, "desert_hydra", "Desert Hydra"},
    };
    m_weaponDefinitions.push_back(awp);

    // USP-S скины
    WeaponDefinition usps;
    usps.definitionIndex = static_cast<int>(WeaponIndex::UspSilencer);
    usps.name = "weapon_usp_silencer";
    usps.displayName = "USP-S";
    usps.skins = {
        {0, "default", "Default"},
        {183, "orion", "Orion"},
        {217, "caiman", "Caiman"},
        {236, "serum", "Serum"},
        {277, "cyrex", "Cyrex"},
        {290, "guardian", "Guardian"},
        {313, "road_rash", "Road Rash"},
        {339, "business_class", "Business Class"},
        {367, "neo_noir", "Neo-Noir"},
        {392, "kill_confirmed", "Kill Confirmed"},
        {412, "torque", "Torque"},
        {443, "monster_mash", "Monster Mash"},
        {454, "blue_phosphor", "Blue Phosphor"},
        {478, "cyberpunk", "Cyberpunk"},
        {504, "printstream", "Printstream"},
        {529, "target_acquired", "Target Acquired"},
        {553, "black_lotus", "Black Lotus"},
        {583, "caiman", "Caiman"},
        {615, "flashback", "Flashback"},
        {647, "check_engine", "Check Engine"},
        {677, "orange_anodized", "Orange Anodized"},
        {705, "purple_ddp", "Purple DDP"},
        {733, "corvus", "Corvus"},
        {765, "the_imperator", "The Imperator"},
        {797, "black_lotus", "Black Lotus"},
        {831, "printstream", "Printstream"},
        {875, "target_acquired", "Target Acquired"},
        {920, "welcome_to_the_jungle", "Welcome to the Jungle"},
        {970, "black_lotus", "Black Lotus"},
        {1003, "printstream", "Printstream"},
        {1046, "target_acquired", "Target Acquired"},
        {1094, "black_lotus", "Black Lotus"},
        {1140, "printstream", "Printstream"},
        {1170, "target_acquired", "Target Acquired"},
        {1206, "black_lotus", "Black Lotus"},
        {1252, "printstream", "Printstream"},
    };
    m_weaponDefinitions.push_back(usps);

    // Glock-18 скины
    WeaponDefinition glock;
    glock.definitionIndex = static_cast<int>(WeaponIndex::Glock);
    glock.name = "weapon_glock";
    glock.displayName = "Glock-18";
    glock.skins = {
        {0, "default", "Default"},
        {179, "fade", "Fade"},
        {208, "dragon_tattoo", "Dragon Tattoo"},
        {230, "steel_disruption", "Steel Disruption"},
        {255, "wasteland_rebel", "Wasteland Rebel"},
        {278, "water_elemental", "Water Elemental"},
        {293, "reactor", "Reactor"},
        {319, "royal_legion", "Royal Legion"},
        {347, "twilight_galaxy", "Twilight Galaxy"},
        {369, "bullet_rain", "Bullet Rain"},
        {381, "weasel", "Weasel"},
        {397, "vogue", "Vogue"},
        {427, "off_world", "Off World"},
        {448, "moonrise", "Moonrise"},
        {479, "wasteland_rebel", "Wasteland Rebel"},
        {505, "gamma_doppler", "Gamma Doppler"},
        {528, "neo_noir", "Neo-Noir"},
        {552, "block_18", "Block-18"},
        {582, "death_rattle", "Death Rattle"},
        {614, "ironwork", "Ironwork"},
        {646, "sacrifice", "Sacrifice"},
        {676, "warhawk", "Warhawk"},
        {704, "red_tire", "Red Tire"},
        {732, "snack_attack", "Snack Attack"},
        {764, "oxide_oasis", "Oxide Oasis"},
        {796, "gold_tooth", "Gold Tooth"},
        {830, "winterized", "Winterized"},
        {874, "blue_fissure", "Blue Fissure"},
        {919, "synth_leaf", "Synth Leaf"},
        {969, "vogue", "Vogue"},
        {1002, "off_world", "Off World"},
        {1045, "moonrise", "Moonrise"},
        {1093, "wasteland_rebel", "Wasteland Rebel"},
        {1139, "gamma_doppler", "Gamma Doppler"},
        {1169, "neo_noir", "Neo-Noir"},
        {1205, "block_18", "Block-18"},
        {1251, "death_rattle", "Death Rattle"},
    };
    m_weaponDefinitions.push_back(glock);

    // Desert Eagle скины
    WeaponDefinition deagle;
    deagle.definitionIndex = static_cast<int>(WeaponIndex::Deagle);
    deagle.name = "weapon_deagle";
    deagle.displayName = "Desert Eagle";
    deagle.skins = {
        {0, "default", "Default"},
        {185, "cobalt_disruption", "Cobalt Disruption"},
        {210, "crimson_web", "Crimson Web"},
        {237, "conspiracy", "Conspiracy"},
        {273, "heirloom", "Heirloom"},
        {296, "meteorite", "Meteorite"},
        {322, "golden_koi", "Golden Koi"},
        {348, "copper_balboa", "Copper Balboa"},
        {370, "pilot", "Pilot"},
        {398, "corinthian", "Corinthian"},
        {428, "midnight_storm", "Midnight Storm"},
        {449, "sunrise", "Sunrise"},
        {480, "oxblood", "Oxblood"},
        {506, "mecha_industries", "Mecha Industries"},
        {529, "trigger_discipline", "Trigger Discipline"},
        {553, "blue_streak", "Blue Streak"},
        {583, "code_red", "Code Red"},
        {615, "fennec", "Fennec"},
        {647, "lightning_strike", "Lightning Strike"},
        {677, "printstream", "Printstream"},
        {705, "the_magician", "The Magician"},
        {733, "night_heist", "Night Heist"},
        {765, "akimbo", "Akimbo"},
        {797, "emerald_poison_dart", "Emerald Poison Dart"},
        {831, "blue_spray", "Blue Spray"},
        {875, "sunset", "Sunset"},
        {920, "printstream", "Printstream"},
        {970, "the_magician", "The Magician"},
        {1003, "night_heist", "Night Heist"},
        {1046, "akimbo", "Akimbo"},
        {1094, "emerald_poison_dart", "Emerald Poison Dart"},
        {1140, "blue_spray", "Blue Spray"},
        {1170, "sunset", "Sunset"},
        {1206, "printstream", "Printstream"},
        {1252, "the_magician", "The Magician"},
    };
    m_weaponDefinitions.push_back(deagle);

    // Нож скины
    WeaponDefinition knife;
    knife.definitionIndex = static_cast<int>(WeaponIndex::Knife);
    knife.name = "weapon_knife";
    knife.displayName = "Knife";
    knife.skins = {
        {0, "default", "Default"},
        {5, "fade", "Fade"},
        {12, "crimson_web", "Crimson Web"},
        {38, "doppler", "Doppler"},
        {40, "tiger_tooth", "Tiger Tooth"},
        {41, "damascus_steel", "Damascus Steel"},
        {42, "marble_fade", "Marble Fade"},
        {43, "rust_coat", "Rust Coat"},
        {44, "stained", "Stained"},
        {45, "case_hardened", "Case Hardened"},
        {46, "safari_mesh", "Safari Mesh"},
        {47, "boreal_forest", "Boreal Forest"},
        {48, "night", "Night"},
        {49, "urban_masked", "Urban Masked"},
        {50, "forest_ddpat", "Forest DD PAT"},
        {51, "blue_steel", "Blue Steel"},
        {52, "night_stripe", "Night Stripe"},
        {53, "autotronic", "Autotronic"},
        {54, "bright_water", "Bright Water"},
        {55, "freehand", "Freehand"},
        {56, "gamma_doppler", "Gamma Doppler"},
        {57, "lore", "Lore"},
        {58, "black_laminate", "Black Laminate"},
        {59, "ultraviolet", "Ultraviolet"},
        {60, "vanilla", "Vanilla"},
        {61, "fade", "Fade"},
        {62, "crimson_web", "Crimson Web"},
        {63, "doppler", "Doppler"},
        {64, "tiger_tooth", "Tiger Tooth"},
        {65, "damascus_steel", "Damascus Steel"},
        {66, "marble_fade", "Marble Fade"},
        {67, "rust_coat", "Rust Coat"},
        {68, "stained", "Stained"},
        {69, "case_hardened", "Case Hardened"},
        {70, "safari_mesh", "Safari Mesh"},
        {71, "boreal_forest", "Boreal Forest"},
        {72, "night", "Night"},
        {73, "urban_masked", "Urban Masked"},
        {74, "forest_ddpat", "Forest DD PAT"},
        {75, "blue_steel", "Blue Steel"},
        {76, "night_stripe", "Night Stripe"},
        {77, "autotronic", "Autotronic"},
        {78, "bright_water", "Bright Water"},
        {79, "freehand", "Freehand"},
        {80, "gamma_doppler", "Gamma Doppler"},
        {81, "lore", "Lore"},
        {82, "black_laminate", "Black Laminate"},
        {83, "ultraviolet", "Ultraviolet"},
        {84, "vanilla", "Vanilla"},
        {85, "fade", "Fade"},
        {86, "crimson_web", "Crimson Web"},
        {87, "doppler", "Doppler"},
        {88, "tiger_tooth", "Tiger Tooth"},
        {89, "damascus_steel", "Damascus Steel"},
        {90, "marble_fade", "Marble Fade"},
        {91, "rust_coat", "Rust Coat"},
        {92, "stained", "Stained"},
        {93, "case_hardened", "Case Hardened"},
        {94, "safari_mesh", "Safari Mesh"},
        {95, "boreal_forest", "Boreal Forest"},
        {96, "night", "Night"},
        {97, "urban_masked", "Urban Masked"},
        {98, "forest_ddpat", "Forest DD PAT"},
        {99, "blue_steel", "Blue Steel"},
        {100, "night_stripe", "Night Stripe"},
    };
    m_weaponDefinitions.push_back(knife);

    // Перчатки
    WeaponDefinition gloves;
    gloves.definitionIndex = static_cast<int>(WeaponIndex::Gloves);
    gloves.name = "weapon_gloves";
    gloves.displayName = "Gloves";
    gloves.skins = {
        {0, "default", "Default"},
        {10001, "bloodhound_gloves", "Bloodhound Gloves"},
        {10002, "driver_gloves", "Driver Gloves"},
        {10003, "hand_wraps", "Hand Wraps"},
        {10004, "moto_gloves", "Moto Gloves"},
        {10005, "specialist_gloves", "Specialist Gloves"},
        {10006, "sport_gloves", "Sport Gloves"},
        {10007, "hydra_gloves", "Hydra Gloves"},
    };
    m_weaponDefinitions.push_back(gloves);

    // Сортируем по имени для удобства
    std::sort(m_weaponDefinitions.begin(), m_weaponDefinitions.end(),
        [](const WeaponDefinition& a, const WeaponDefinition& b) {
            return a.displayName < b.displayName;
        });
}

SkinEntry Skinchanger::GetSkinForWeapon(int weaponIndex) {
    auto it = m_config.weaponSkins.find(weaponIndex);
    if (it != m_config.weaponSkins.end()) {
        return it->second;
    }
    
    // Возвращаем пустой скин
    SkinEntry empty;
    empty.weaponIndex = weaponIndex;
    return empty;
}

void Skinchanger::LoadSkinDatabase() {
    // В реальном проекте здесь загружается JSON с базой данных скинов
    // Для сейчас используем встроенную базу данных
    InitializeSkinDatabase();
}

} // namespace Features
