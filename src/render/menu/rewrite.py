import re

path = r'c:\Users\dakem\Documents\Antigravity Projects\cs2overlay\src\render\menu\menu.cpp'
with open(path, 'r', encoding='utf-8') as f:
    code = f.read()

start_idx = code.find('#include "features/aimbot/aimbot.h"')
end_idx = code.find('} // namespace Features\n')
if start_idx != -1 and end_idx != -1:
    code = code[:start_idx] + '#include "config/settings.h"\n' + code[end_idx + 24:]

esp_checkbox = '''        bool en = Features::config.enabled;
        if (ImGui::Checkbox("Enable ESP", &en)) {
          for (auto &f : Features::FeatureManager::features)
            if (std::string(f->GetName()) == "ESP")
              f->SetEnabled(en);
          Features::config.enabled = en;
        }'''
esp_checkbox_new = '''        if (ImGui::Checkbox("Enable ESP", &Config::Settings.esp.enabled)) {
          Config::ConfigManager::ApplySettings();
        }'''
code = code.replace(esp_checkbox, esp_checkbox_new)

code = code.replace('Features::config.', 'Config::Settings.esp.')
code = code.replace('Features::aimbotConfig.', 'Config::Settings.aimbot.')
code = code.replace('Features::triggerbotConfig.', 'Config::Settings.triggerbot.')
code = code.replace('Features::miscConfig.', 'Config::Settings.misc.')
code = code.replace('Features::radarConfig.', 'Config::Settings.radar.')
code = code.replace('Features::bombConfig.', 'Config::Settings.bomb.')
code = code.replace('Features::debugConfig.', 'Config::Settings.debug.')

def replace_enable_loop(feature_name, settings_name):
    global code
    pattern = r'if \(ImGui::Checkbox\(\"Enable ' + feature_name + r'\", &Config::Settings\.' + settings_name + r'\.enabled\)\) \{\s+for \(auto &f : Features::FeatureManager::features\)\s+if \(std::string\(f->GetName\(\)\) == \"[a-zA-Z]+\"\)\s+f->SetEnabled\(Config::Settings\.' + settings_name + r'\.enabled\);\s+\}'
    replacement = r'if (ImGui::Checkbox("Enable ' + feature_name + r'", &Config::Settings.' + settings_name + r'.enabled)) {\n          Config::ConfigManager::ApplySettings();\n        }'
    code = re.sub(pattern, replacement, code, flags=re.DOTALL)

replace_enable_loop('Aimbot', 'aimbot')
replace_enable_loop('Triggerbot', 'triggerbot')
replace_enable_loop('Radar', 'radar')
replace_enable_loop('Bomb Timer', 'bomb')

pattern = r'if \(ImGui::Checkbox\(\"Enable\", &Config::Settings\.misc\.awpCrosshair\)\) \{\s+for \(auto &f : Features::FeatureManager::features\)\s+if \(std::string\(f->GetName\(\)\) == \"Misc\"\)\s+f->SetEnabled\(Config::Settings\.misc\.awpCrosshair\);\s+\}'
replacement = r'if (ImGui::Checkbox("Enable", &Config::Settings.misc.awpCrosshair)) {\n          Config::ConfigManager::ApplySettings();\n        }'
code = re.sub(pattern, replacement, code, flags=re.DOTALL)

pattern = r'if \(ImGui::Checkbox\(\"Enable Debug Overlay\",\s*&Config::Settings\.debug\.enabled\)\) \{\s+for \(auto &f : Features::FeatureManager::features\)\s+if \(std::string\(f->GetName\(\)\) == \"DebugOverlay\"\)\s+f->SetEnabled\(Config::Settings\.debug\.enabled\);\s+\}'
replacement = r'if (ImGui::Checkbox("Enable Debug Overlay", &Config::Settings.debug.enabled)) {\n          Config::ConfigManager::ApplySettings();\n        }'
code = re.sub(pattern, replacement, code, flags=re.DOTALL)

code = code.replace('isOpen = !isOpen;', 'isOpen = !isOpen;\n  Config::Settings.menuIsOpen = isOpen;')

with open(path, 'w', encoding='utf-8') as f:
    f.write(code)
print("Menu updated.")
