#pragma once
#include <cstddef>
#include <cstdint>

namespace Core::Constants {

inline constexpr uintptr_t MIN_VALID_ADDRESS = 0x10000;
inline constexpr uintptr_t MAX_VALID_ADDRESS = 0x7FFFFFFFFFFFFFFFULL;

inline constexpr int AUTO_ATTACH_INTERVAL_SEC = 5;
inline constexpr int DEFAULT_UPS_LIMIT = 240;
inline constexpr int DEFAULT_FPS_LIMIT = 240;

inline constexpr int ENTITY_LIST_HEADER_OFFSET = 0x10;
inline constexpr size_t ENTITY_IDENTITY_ENTRY_SIZE = 0x70;
inline constexpr int MAX_PLAYERS = 64;

inline constexpr float ESP_MAX_DISTANCE_UNITS = 5000.0f;
inline constexpr size_t MAX_POSITION_CACHE_SIZE = 128;
inline constexpr int INVALID_SLOT_SKIP_FRAMES = 15;

} // namespace Core::Constants
