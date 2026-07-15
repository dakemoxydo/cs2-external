#pragma once

#include "game_snapshot.h"

#include <atomic>
#include <memory>

namespace Core {

class GameStateStore {
public:
  GameStateStore();

  void Publish(GameSnapshot snapshot);
  [[nodiscard]] std::shared_ptr<const GameSnapshot> Load() const;

private:
  std::atomic<std::shared_ptr<const GameSnapshot>> snapshot_;
};

} // namespace Core
