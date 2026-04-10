# Структура и принципы работы проекта CS2 External

Данный проект является внешним (external) программным обеспечением для игры Counter-Strike 2. Он работает как отдельный процесс, не внедряя (inject) DLL в саму игру, что повышает безопасность.

## 📁 Структура проекта

Проект разделён на логические слои (Application, Core, Features, Render, Config), строго следующих правилу односторонних зависимостей.

### Архитектура папок

```
cs2overlay/
├── build.bat
├── offsets/                         ← папка проекта (рядом с build.bat)
│   └── output/                      ← сюда кидаешь папку output из cs2-dumper
│       ├── client_dll.json
│       ├── client_dll.hpp
│       ├── offsets.json
│       ├── offsets.hpp
│       └── ...
├── build/
│   └── Release/
│       ├── cs2overlay.exe
│       └── cache_offsets/           ← создаётся при билде / при GitHub update
│           ├── offsets.json
│           ├── client_dll.json
│           ├── offsets.hpp          (если есть)
│           └── client_dll.hpp       (если есть)
├── data/                            ← устаревшая папка (больше не используется)
└── src/
```

### 0. `src/main.cpp` — Точка входа
Минимальный файл (10 строк) — создаёт и запускает `Application`.

### 1. `src/core/` — Ядро чита

#### `application/` — Управление жизненным циклом
*   `application.h/cpp`: Главный класс `Application` — инициализация, главный цикл (Run), завершение (Shutdown).
*   `app_state.h`: Состояние приложения (running, menuOpen, shouldClose) с атомарными флагами.
*   `app_config.h`: Конфигурация приложения (fpsLimit, upsLimit, vsyncEnabled).

#### `memory/` — Работа с памятью
*   `memory_manager.h`: Обёртка над `NtReadVirtualMemory`.
    *   `Read<T>()` — игнорирует ошибки, возвращает 0 при неудаче
    *   `ReadOptional<T>()` — возвращает `std::optional<T>`
    *   `ReadBatch()` — пакетное чтение непрерывных блоков
    *   `ReadChain<T>()` — хелпер для цепочек указателей
    *   `ReadRaw()` — чтение сырых байтов в буфер

#### `process/` — Управление процессом
*   `process.h/cpp`: Поиск окна, аттач к `cs2.exe`, PID, хендл.
*   `module.h/cpp`: Базовые адреса модулей (`client.dll`, и др.).
*   `stealth.h/cpp`: Анти-детект (PEB spoofing).

#### `game/` — Игровые сущности
*   `game_manager.h/cpp`: Центральный узел — данные игрока, противники, матч.
    *   Double-buffered entity list (`playerBuffers[2]`)
    *   `SetScreenSize()` / `IsOnScreen()` — frustum culling
    *   `EnableBoneRead()` / `EnableWeaponRead()` — lazy reading flags
    *   `GetRenderPlayers()` — thread-safe getter
*   `game_manager_getters.cpp`: Вынесенные геттеры.
*   `entity_list.h`: Обход списка объектов.
*   `local_player.h`: Доступ к данным своего персонажа.

#### `sdk/` — Техническая информация
*   `offsets.h`: Смещения памяти (inline переменные C++17). Все поля = 0 по умолчанию (кроме internal: m_hPawn, m_bIsLocalPlayerController, m_entitySpottedState, m_bSpottedByMaskOffset, m_boneArrayOffset). Заполняются при загрузке из дампа.
*   `offset_file_loader.h/cpp`: Загрузка сырых файлов оффсетов.
    *   `LoadFromCacheDir()` — читает `cache_offsets/` рядом с `.exe` (JSON + HPP)
    *   `DownloadFromGitHub()` — скачивает с `a2x/cs2-dumper`
    *   `SaveToCacheDir()` — сохраняет JSON в `cache_offsets/`
*   `offset_parser.h/cpp`: Парсинг сырых файлов в структурированные оффсеты.
    *   `ParseJson()` — парсит `offsets.json` + `client_dll.json` через nlohmann::json
    *   `ParseHpp()` — парсит `offsets.hpp` + `client_dll.hpp` через regex (#define)
    *   Приоритет: JSON > HPP. Если оба формата есть — используется JSON.
*   `offset_applier.h/cpp`: Применение распарсенных оффсетов к `SDK::Offsets`.
    *   `Apply()` — копирует все поля из ParsedOffsets → SDK::Offsets
    *   `Validate()` — проверяет критичные поля (dwEntityList, dwLocalPlayerPawn, dwViewMatrix)
    *   `LogStatus()` — логирует все значения в консоль, предупреждает о missing
*   `offset_loader.h/cpp`: Facade — объединяет FileLoader → Parser → Applier.
    *   `LoadOffsets()` — основной вход: cache_offsets/ → (если невалиден) GitHub → парсинг → применение
    *   `ReloadOffsets()` — перечитать cache_offsets/ без скачивания
    *   `ForceUpdateFromGitHub()` — скачать → сохранить → распарсить → применить
*   `updater.h`: Backward compatibility wrapper для `OffsetLoader` (async через std::future).
    *   `UpdateOffsets()`, `ForceUpdateOffsets()`, `ReloadOffsets()`
*   `entity.h` / `entity_classes.h`: Классы игроков, костей, объектов.
    *   `Entity` struct: renderPosition, bonePositions, onScreen, distance, health, name, weapon
    *   `BombInfo` struct: isPlanted, timeLeft, site
*   `player.h`, `structs.h`: Дополнительные структуры.

#### `math/` — Математика
*   `math.h/cpp`: Векторы, углы, матрицы, WorldToScreen.

#### `input/` — Ввод
*   `input_manager.h/cpp`: Обработка мыши/клавиатуры.
*   `keybinds.h`: Горячие клавиши.

### 2. `src/features/` — Игровой функционал

Все фичи наследуются от `IFeature` (`feature_base.h`) с методами:
*   `Update()` — логика (вызывается из render thread)
*   `Render(DrawList&)` — отрисовка в оверлее
*   `RenderUI()` — отрисовка настроек в меню
*   `GetName()` — имя фичи
*   `OnEnable()` / `OnDisable()` — хуки включения/выключения
*   `Initialize()` — lazy init (вызывается один раз при первом включении)

#### Фичи:
| Фича | Файлы | Описание |
|------|-------|----------|
| **Aimbot** | `aimbot.h/cpp`, `aimbot_config.h`, `aimbot_ui.h` | Автонаведение, FOV, smooth, target bone |
| **ESP** | `esp.h/cpp`, `esp_config.h`, `esp_ui.h` | Box, health bar, name, weapon, skeleton, snaplines, frustum culling |
| **Footsteps ESP** | `footsteps_esp/footsteps_esp.h/cpp`, `footsteps_esp_config.h`, `footsteps_esp_ui.h` | Расширяющиеся круги под ногами врагов при ходьбе, прыжках, приземлении |
| **Triggerbot** | `triggerbot.h/cpp`, `triggerbot_config.h`, `triggerbot_ui.h` | Автовыстрел при наведении |
| **RCS** | `rcs.h/cpp`, `rcs_config.h`, `rcs_ui.h` | Компенсация отдачи (standalone) |
| **Misc** | `misc.h/cpp`, `misc_config.h`, `misc_ui.h` | AWP crosshair и другое |
| **Bomb** | `bomb.h/cpp`, `bomb_config.h`, `bomb_ui.h` | Таймер бомбы, носитель |
| **Radar** | `radar.h/cpp`, `radar_config.h`, `radar_ui.h` | 2D-радар с позициями |
| **Debug** | `debug_overlay.h/cpp`, `debug_overlay_config.h`, `debug_overlay_ui.h` | Отладочная визуализация |

#### Менеджер фич:
*   `feature_base.h`: Интерфейс `IFeature`
*   `feature_manager.h/cpp`: Factory pattern — фичи регистрируются как фабрики, инстансы создаются при первом включении.
    *   `RegisterAll()` — регистрация всех фич
    *   `UpdateAll()` / `RenderAll()` — обновление и отрисовка
    *   `ApplySettings()` — lazy-init при изменении enabled state
    *   `EnsureFeatureInitialized()` / `EnsureAllInitialized()` / `GetFeature()` — хелперы

### 3. `src/render/` — Визуализация и интерфейс

#### `overlay/` — Окно оверлея
*   `overlay.h/cpp`: Прозрачное окно поверх CS2, UpdatePosition().
*   `GetGameWidth()` / `GetGameHeight()` — размеры игровой области (для WorldToScreen).

#### `renderer/` — Графический движок
*   `renderer.h/cpp`: DirectX 11 инициализация, BeginFrame/EndFrame.
*   `imgui_manager.h/cpp`: ImGui инициализация, NewFrame, Render, Shutdown.

#### `draw/` — Рисование
*   `draw_list.h/cpp`: Линии, текст, фигуры, боксы, круги, углы.

#### `menu/` — Главное меню (модульное)
| Файл | Описание |
|------|----------|
| `menu.h/cpp` | Каркас: header, sidebar, tab switcher (~140 строк) |
| `menu_theme.h/cpp` | 7 тем: Midnight, Blood, Cyber, Lavender, Gold, Monochrome, Toxic |
| `tab_legit.h/cpp` | Вкладка LEGIT: Aimbot + Triggerbot |
| `tab_visuals.h/cpp` | Вкладка VISUALS: ESP + Footsteps ESP + Radar + Bomb |
| `tab_misc.h/cpp` | Вкладка MISC: Crosshair |
| `tab_settings.h/cpp` | Вкладка SETTINGS: Configs, Themes, Performance, Debug, Frustum Culling, Offsets |
| `ui_components.h/cpp` | UI-виджеты: SettingToggle, SettingHotkey, SettingColor, BeginCard |

### 4. `src/config/` — Конфигурация
*   `config_manager.h/cpp`: Config Registry — авто-сериализация через `BuildRegistry()`.
    *   `Load()` / `Save()` — загрузка/сохранение конфигов
    *   `ApplySettings()` — lazy-init фич при изменении настроек
*   `settings.h`: Глобальная структура `GlobalSettings` со всеми конфигами.

### 5. `src/utils/` — Утилиты
*   `logger.h/cpp`: Расширенный логгер с уровнями (Debug/Info/Warn/Error), вывод в консоль и файл.
*   `string_utils.h`: Утилиты для строк (ToLower, ToUpper, StartsWith, EndsWith, Trim).
*   `math.h`: Математические хелперы.
*   `timer.h`: Таймер для замеров времени.

### 6. `external/` — Сторонние библиотеки
*   `imgui/`: ImGui с бэкендами Win32 + DirectX 11.

---

## ⚙️ Принципы работы

### Многопоточная архитектура
Чит разделяет логику на два параллельных потока:

1.  **Memory Thread (Поток памяти)**:
    *   Работает в бесконечном цикле (частота ~64-240 UPS).
    *   Читает данные из памяти CS2, обновляет `GameManager`.
    *   Инкапсулирован в `Application::MemoryThreadLoop()`.

2.  **Render Thread (Основной поток)**:
    *   Обработка Windows сообщений, отрисовка Overlay.
    *   Логика фич (aimbot angles, triggerbot).
    *   Рисует ESP на основе данных из Memory Thread.
    *   Инкапсулирован в `Application::RenderLoop()`.

### Иерархия управления
*   `main.cpp` → `Application::Initialize()` → `Application::Run()`
*   `Application` управляет Memory Thread и Render Loop
*   `GameManager` предоставляет данные через double-buffering
*   `FeatureManager` координирует фичи (factory pattern, lazy init)
*   `Menu` делегирует UI фичам через `RenderUI()`

### Парадигмы кода (Важно для ИИ)
1. **Config Registry (Авто-сериализация)**: Параметры добавляются в `BuildRegistry()` — цикл Save/Load проходит по массиву автоматически.
2. **Feature UI (Open-Closed Principle)**: Каждая фича рендерит свой UI через `RenderUI()`. Menu.cpp делегирует, не хардкодит.
3. **Application Layer**: Вся логика инициализации в классе `Application`, main.cpp — тонкая обёртка.
4. **Factory Pattern**: Фичи регистрируются как фабрики, инстансы создаются только при первом включении.
5. **Double-Buffering**: Entity list записывается в один буфер, читается из другого — без блокировок.

---

## ⚠️ Важные подсказки и правила

### Окно CS2 и оверлей
- Оверлей находит окно CS2 через `FindWindowA(nullptr, "Counter-Strike 2")`
- Fallback через `EnumWindows` если не найдено
- `UpdatePosition()` вызывается КАЖДЫЙ КАДР
- **ВАЖНО:** WorldToScreen использует `Overlay::GetGameWidth/Height()`
- `GameManager::SetScreenSize()` вызывается каждый кадр из render loop

### Система оффсетов (3-этапный пайплайн)

**Архитектура папок:**
```
offsets/output/     ← пользователь кладёт сюда папку output из cs2-dumper
build.bat           ← копирует из offsets/output/ → build/Release/cache_offsets/
build/Release/cache_offsets/  ← runtime читает отсюда
```

**Поток данных:**
1. **Build-time**: `build.bat` ищет `offsets/output/` → копирует JSON/HPP файлы в `build/Release/cache_offsets/`
2. **Runtime**: exe читает `cache_offsets/` рядом с собой → `OffsetParser` парсит JSON (приоритет) или HPP (fallback через regex) → `OffsetApplier` применяет к `SDK::Offsets`
3. **GitHub fallback**: если `cache_offsets/` нет или невалиден → скачивает с `a2x/cs2-dumper` → сохраняет в `cache_offsets/` → парсит
4. **UI Update**: кнопка "Update Offsets from GitHub" → скачивает → перезаписывает `cache_offsets/` → парсит заново
5. **UI Reload**: кнопка "Reload Offsets from Disk" → перечитывает `cache_offsets/` без скачивания

**Поддержка форматов:**
- Приоритет: `.json` (`offsets.json` + `client_dll.json`)
- Fallback: `.hpp` (`offsets.hpp` + `client_dll.hpp`) — парсинг через regex `#define`
- Если оба формата есть — используется JSON

**Файлы SDK:**
- `offset_file_loader.h/cpp` — загрузка файлов из `cache_offsets/` или GitHub
- `offset_parser.h/cpp` — парсинг JSON (nlohmann) + HPP (regex)
- `offset_applier.h/cpp` — запись в `SDK::Offsets`, валидация, логирование
- `offset_loader.h/cpp` — Facade: FileLoader → Parser → Applier
- `offsets.h` — `inline` переменные (C++17), все = 0 по умолчанию (кроме internal)
- `updater.h` — async wrapper с `UpdateOffsets()`, `ForceUpdateOffsets()`, `ReloadOffsets()`

**Критичные поля для валидации:** `dwEntityList`, `dwLocalPlayerPawn`, `dwViewMatrix` — если хотя бы одно = 0, ESP не будет работать.

### Чтение памяти
- `MemoryManager::Read<T>()` — игнорирует ошибки
- `MemoryManager::ReadOptional<T>()` — возвращает `std::optional<T>`
- `MemoryManager::ReadBatch()` — пакетное чтение
- `MemoryManager::ReadRaw()` — чтение сырых байтов
- **ВАЖНО:** Не добавлять verbose-логирование в NtRead

### Конфигурация
- Config Registry: параметры в `BuildRegistry()` → авто-сериализация
- `Settings` — глобальная структура
- Конфиги сохраняются в папку `configs/` рядом с exe

### Feature Manager
- `RegisterAll()` — регистрация фич как фабрик
- `UpdateAll()` — ТОЛЬКО из render thread (содержит GetAsyncKeyState/SendInput)
- `RenderAll(drawList)` — отрисовка через DrawList
- `ApplySettings()` — lazy-init: создаёт инстанс фичи только при enabled = true
- `EnsureFeatureInitialized(name)` — принудительная инициализация конкретной фичи

### Frustum Culling
- `GameManager::SetScreenSize(w, h)` — вызывается каждый кадр
- `GameManager::IsOnScreen(worldPos)` — проверка через viewMatrix projection
- `Entity::onScreen` — флаг, обновляется каждый кадр в `Update()`
- ESP пропускает off-screen entities; `showOffscreen` рисует стрелки на краю экрана
- `frustumCullingEnabled` — глобальный переключатель (Settings → Performance & Debug)

### Footsteps ESP
- Обнаружение через чтение `m_fFlags` (bit 0 = FL_ONGROUND) + `m_vecVelocity` из памяти CS2
- **Walking**: on ground + velocity > 50 → жёлтый круг (~25 units max)
- **Jumping**: was on ground → now airborne → голубой круг (~40 units max)
- **Landing**: was airborne → now on ground → оранжевый круг (radius зависит от скорости падения, до ~55 units)
- Круги расширяются за `expandDuration`, затем затухают за `fadeDuration`
- Радиус в пикселях вычисляется через проекцию смещённой точки в screen space
- Дебаунс footsteps: не чаще 250ms на одного игрока

### Сборка
- `build.bat` — основной скрипт
- `build.bat` копирует файлы из `offsets/output/` в `build/Release/cache_offsets/` перед сборкой
- CMakeLists.txt: `GLOB_RECURSE CONFIGURE_DEPENDS` — новые файлы подхватываются автоматически
- `dxguid` добавлен в target_link_libraries
- Использовать только папку `build/` — не плодить build2, build3 и т.д.
- `.gitignore` содержит `build*/` — все build-папки игнорируются git
