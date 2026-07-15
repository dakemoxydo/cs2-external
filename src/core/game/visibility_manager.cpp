#include "visibility_manager.h"

namespace Core {

// Visibility is derived in GameManager from EntitySpottedState_t::m_bSpottedByMask.
// This compatibility facade intentionally performs no process writes, remote
// allocations, pattern scans, or remote-thread execution.
void VisibilityManager::PrepareFrame(float) {}

VisibilityResult VisibilityManager::QueryPlayerVisibility(
    uintptr_t, uintptr_t, const SDK::Vector3 &, const SDK::Vector3 &) {
  return {};
}

bool VisibilityManager::IsAvailable() { return false; }
void VisibilityManager::ResetFrameState() {}
void VisibilityManager::ResetProcessState() {}

} // namespace Core
