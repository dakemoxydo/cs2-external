#include "game_state_store.h"

namespace Core {

GameStateStore::GameStateStore()
    : snapshot_(std::make_shared<const GameSnapshot>()) {}

void GameStateStore::Publish(GameSnapshot snapshot) {
  snapshot_.store(std::make_shared<const GameSnapshot>(std::move(snapshot)),
                  std::memory_order_release);
}

std::shared_ptr<const GameSnapshot> GameStateStore::Load() const {
  return snapshot_.load(std::memory_order_acquire);
}

} // namespace Core
