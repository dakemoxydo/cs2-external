#include "core/game/game_state_store.h"

#include <cassert>

int main() {
  Core::GameStateStore store;
  const auto empty = store.Load();
  assert(empty);
  assert(empty->players.empty());

  Core::GameSnapshot frame;
  frame.clientBase = 0x1234;
  frame.localTeam = 3;
  frame.localWeaponName = "weapon_ak47";
  store.Publish(std::move(frame));

  const auto published = store.Load();
  assert(published->clientBase == 0x1234);
  assert(published->localTeam == 3);
  assert(published->localWeaponName == "weapon_ak47");
  return 0;
}
