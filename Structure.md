# Структура и принципы работы проекта CS2 External

Данный проект является внешним (external) программным обеспечением для игры Counter-Strike 2. Он работает как отдельный процесс, не внедряя (inject) DLL в саму игру, что повышает безопасность.

## 📁 Структура проекта

Проект разделён на логические слои (Application, Core, Features, Render, Config), строго следующих правилу односторонних зависимостей.

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

#### `process/` — Управление процессом
*   `process.h/cpp`: Поиск окна, аттач к `cs2.exe`, PID, хендл.
*   `module.h/cpp`: Базовые адреса модулей (`client.dll`).
*   `stealth.h/cpp`: Анти-детект (PEB spoofing).

#### `game/` — Игровые сущности
*   `game_manager.h/cpp`: Центральный узел — данные игрока, противники, матч.
*   `game_manager_getters.cpp`: Вынесенные геттеры.
*   `entity_list.h`: Обход списка объектов.
*   `local_player.h`: Доступ к данным своего персонажа.

#### `sdk/` — Техническая информация
*   `offsets.h`: Смещения памяти (inline переменные C++17).
*   `offset_loader.h/cpp`: Загрузка оффсетов из кэша/GitHub (OOP класс).
*   `updater.h/cpp`: Backward compatibility wrapper для `OffsetLoader`.
*   `entity.h` / `entity_classes.h`: Классы игроков, костей, объектов.
*   `player.h`, `structs.h`: Дополнительные структуры.

#### `math/` — Математика
*   `math.h/cpp`: Векторы, углы, матрицы.

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

#### Фичи:
| Фича | Файлы | Описание |
|------|-------|----------|
| **Aimbot** | `aimbot.h/cpp`, `aimbot_config.h`, `aimbot_ui.h` | Автонаведение, FOV, smooth, target bone |
| **ESP** | `esp.h/cpp`, `esp_config.h`, `esp_ui.h` | Box, health bar, name, weapon, skeleton, snaplines |
| **Triggerbot** | `triggerbot.h/cpp`, `triggerbot_config.h`, `triggerbot_ui.h` | Автовыстрел при наведении |
| **RCS** | `rcs.h/cpp`, `rcs_config.h`, `rcs_ui.h` | Компенсация отдачи (standalone) |
| **Misc** | `misc.h/cpp`, `misc_config.h`, `misc_ui.h` | AWP crosshair и другое |
| **Bomb** | `bomb.h/cpp`, `bomb_config.h`, `bomb_ui.h` | Таймер бомбы, носитель |
| **Radar** | `radar.h/cpp`, `radar_config.h`, `radar_ui.h` | 2D-радар с позициями |
| **Debug** | `debug_overlay.h/cpp`, `debug_overlay_config.h`, `debug_overlay_ui.h` | Отладочная визуализация |

#### Менеджер фич:
*   `feature_base.h`: Интерфейс `IFeature`
*   `feature_manager.h/cpp`: Регистрация, UpdateAll(), RenderAll()

### 3. `src/render/` — Визуализация и интерфейс

#### `overlay/` — Окно оверлея
*   `overlay.h/cpp`: Прозрачное окно поверх CS2, UpdatePosition().

#### `renderer/` — Графический движок
*   `renderer.h/cpp`: DirectX 11 инициализация, BeginFrame/EndFrame.
*   `imgui_manager.h/cpp`: ImGui инициализация, NewFrame, Render, Shutdown.

#### `draw/` — Рисование
*   `draw_list.h/cpp`: Линии, текст, фигуры, боксы.
*   `world_to_screen.h`: Преобразование 3D → 2D.

#### `menu/` — Главное меню (модульное)
| Файл | Описание |
|------|----------|
| `menu.h/cpp` | Каркас: header, sidebar, tab switcher (~140 строк) |
| `menu_theme.h/cpp` | 7 тем: Midnight, Blood, Cyber, Lavender, Gold, Monochrome, Toxic |
| `tab_legit.h/cpp` | Вкладка LEGIT: Aimbot + Triggerbot |
| `tab_visuals.h/cpp` | Вкладка VISUALS: ESP + Radar + Bomb |
| `tab_misc.h/cpp` | Вкладка MISC: Crosshair |
| `tab_settings.h/cpp` | Вкладка SETTINGS: Configs, Themes, Performance |
| `ui_components.h/cpp` | UI-виджеты: SettingToggle, SettingHotkey, SettingColor, BeginCard |

### 4. `src/config/` — Конфигурация
*   `config_manager.h/cpp`: Config Registry — авто-сериализация через `BuildRegistry()`.
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
*   `FeatureManager` координирует фичи
*   `Menu` делегирует UI фичам через `RenderUI()`

### Парадигмы кода (Важно для ИИ)
1. **Config Registry (Авто-сериализация)**: Параметры добавляются в `BuildRegistry()` — цикл Save/Load проходит по массиву автоматически.
2. **Feature UI (Open-Closed Principle)**: Каждая фича рендерит свой UI через `RenderUI()`. Menu.cpp делегирует, не хардкодит.
3. **Application Layer**: Вся логика инициализации в классе `Application`, main.cpp — тонкая обёртка.

---

## ⚠️ Важные подсказки и правила

### Окно CS2 и оверлей
- Оверлей находит окно CS2 через `FindWindowA(nullptr, "Counter-Strike 2")`
- Fallback через `EnumWindows` если не найдено
- `UpdatePosition()` вызывается КАЖДЫЙ КАДР
- **ВАЖНО:** WorldToScreen использует `Overlay::GetGameWidth/Height()`

### Система оффсетов
- Оффсеты загружаются из КЭША (`offsets_cache.json`, `client_cache.json`)
- Кнопка "Update Offsets from GitHub" → принудительное обновление
- `offsets.h` использует `inline` переменные (C++17) — НЕ создавать offsets.cpp
- `OffsetLoader` — OOP класс для загрузки, `Updater` — backward compatibility

### Чтение памяти
- `MemoryManager::Read<T>()` — игнорирует ошибки
- `MemoryManager::ReadOptional<T>()` — возвращает `std::optional<T>`
- `MemoryManager::ReadBatch()` — пакетное чтение
- **ВАЖНО:** Не добавлять verbose-логирование в NtRead

### Конфигурация
- Config Registry: параметры в `BuildRegistry()` → авто-сериализация
- `Settings` — глобальная структура

### Feature Manager
- `RegisterAll()` — регистрация фич
- `UpdateAll()` — ТОЛЬКО из render thread (содержит GetAsyncKeyState/SendInput)
- `RenderAll(drawList)` — отрисовка через DrawList

### Сборка
- `build.bat` — основной скрипт
- CMakeLists.txt: `GLOB_RECURSE CONFIGURE_DEPENDS` — новые файлы подхватываются автоматически
- `dxguid` добавлен в target_link_libraries
