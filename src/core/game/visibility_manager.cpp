#include "visibility_manager.h"
#include "../memory/memory_manager.h"
#include "../memory/pattern_scanner.h"
#include "../process/module.h"
#include "../process/process.h"
#include "core/constants.h"
#include "utils/logger.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>

namespace Core {

namespace {

constexpr const char *kTraceShapePattern =
    "48 89 5C 24 ?? 48 89 4C 24 ?? 55 57";
constexpr const char *kInitTraceEntitiesOnlyPattern =
    "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 0F B6 41 ?? 33 FF 24";
constexpr const char *kGameTraceManagerPattern = "4C 8B 2D ? ? ? ? 24";

constexpr uint64_t CONTENTS_SOLID = (1ull << 0);
constexpr uint64_t CONTENTS_HITBOX = (1ull << 1);
constexpr uint64_t CONTENTS_BLOCK_LOS = (1ull << 6);
constexpr uint64_t CONTENTS_WINDOW = (1ull << 8);
constexpr uint64_t CONTENTS_PLAYER = (1ull << 14);
constexpr uint64_t CONTENTS_NPC = (1ull << 15);
constexpr uint64_t CONTENTS_DEBRIS = (1ull << 16);
constexpr uint64_t MASK_PLAYER_VISIBLE =
    CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC |
    CONTENTS_DEBRIS | CONTENTS_HITBOX | CONTENTS_BLOCK_LOS;

constexpr int kTraceLayer = 4;
constexpr float kVisibilityCacheSeconds = 0.10f;
constexpr int kMaxTracesPerFrame = 16;
constexpr size_t kMaxRemoteThunkSize = 1024;

enum RayType : uint8_t {
  RAY_TYPE_LINE = 0,
};

struct TraceLine {
  SDK::Vector3 startOffset{};
  float radius = 0.0f;
};

struct Ray {
  union {
    TraceLine line;
  };
  uint8_t type = RAY_TYPE_LINE;
};

struct TraceResult {
  uint8_t pad0[8];
  uintptr_t hitEntity = 0;
  uintptr_t hitbox = 0;
  uint8_t pad1[0x60];
  SDK::Vector3 start{};
  SDK::Vector3 end{};
  uint8_t pad2[0x1C];
  float fraction = 0.0f;
  uint8_t pad3[6];
  bool didHit = false;
  uint8_t pad4[0x20];
};

struct TraceFilter {
  std::byte pad01[0x8];
  int64_t traceMask = 0;
  std::array<int64_t, 2> v1{};
  std::array<int32_t, 4> skipHandles{};
  std::array<int16_t, 2> collisions{};
  int16_t v2 = 0;
  uint8_t layer = 0;
  uint8_t v4 = 0;
  uint8_t flags = 0;
};

using InitTraceFilterFn =
    TraceFilter *(__thiscall *)(void *, uintptr_t, uint32_t, int, int16_t);
using TraceShapeFn = bool(__fastcall *)(void *, Ray *, const SDK::Vector3 &,
                                        const SDK::Vector3 &, TraceFilter *,
                                        TraceResult *);

struct TraceShapeCtx {
  TraceShapeFn traceShape = nullptr;
  void *gameTraceManager = nullptr;
  TraceFilter *traceFilter = nullptr;
  SDK::Vector3 start{};
  SDK::Vector3 end{};
  TraceResult *traceResult = nullptr;
  Ray *ray = nullptr;
};

struct InitTraceFilterCtx {
  TraceFilter *filter = nullptr;
  InitTraceFilterFn fn = nullptr;
  uintptr_t skipPawn = 0;
  uint32_t mask = 0;
  int layer = 0;
};

struct CacheEntry {
  bool visible = false;
  float checkedAt = -1000.0f;
};

struct CacheKey {
  uintptr_t enemyPawn = 0;
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;

  bool operator==(const CacheKey &other) const {
    return enemyPawn == other.enemyPawn && x == other.x && y == other.y &&
           z == other.z;
  }
};

struct CacheKeyHasher {
  size_t operator()(const CacheKey &key) const noexcept {
    size_t seed = std::hash<uintptr_t>{}(key.enemyPawn);
    seed ^= std::hash<int32_t>{}(key.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int32_t>{}(key.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int32_t>{}(key.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

uintptr_t s_traceShapeAddress = 0;
uintptr_t s_initTraceFilterAddress = 0;
uintptr_t s_gameTraceManagerAddress = 0;
void *s_remoteTraceShapeShellcode = nullptr;
void *s_remoteTraceShapeCtx = nullptr;
void *s_remoteTraceResult = nullptr;
void *s_remoteRay = nullptr;
void *s_remoteInitFilterShellcode = nullptr;
void *s_remoteInitFilterCtx = nullptr;
TraceFilter *s_remoteTraceFilter = nullptr;
uintptr_t s_traceFilterLocalPawn = 0;
bool s_initialized = false;
bool s_failed = false;
HANDLE s_pendingRemoteThread = nullptr;
DWORD s_initializedProcessId = 0;
float s_frameTime = -1.0f;
int s_remainingBudget = kMaxTracesPerFrame;
std::unordered_map<CacheKey, CacheEntry, CacheKeyHasher> s_cache;

#pragma code_seg(".TraceShapeExec")
#pragma optimize("", off)
#pragma runtime_checks("", off)
#pragma check_stack(off)
__declspec(safebuffers) DWORD WINAPI RemoteTraceShapeThunk(LPVOID parameter) {
  auto *ctx = static_cast<TraceShapeCtx *>(parameter);
  if (!ctx || !ctx->traceShape) {
    return 0;
  }

  ctx->traceShape(ctx->gameTraceManager, ctx->ray, ctx->start, ctx->end,
                  ctx->traceFilter, ctx->traceResult);
  return 1;
}
DWORD WINAPI RemoteTraceShapeThunkEnd() { return 0; }

__declspec(safebuffers) DWORD WINAPI RemoteInitFilterThunk(LPVOID parameter) {
  auto *ctx = static_cast<InitTraceFilterCtx *>(parameter);
  if (!ctx || !ctx->fn || !ctx->filter) {
    return 0;
  }

  ctx->fn(ctx->filter, ctx->skipPawn, ctx->mask, ctx->layer, 15);
  return 1;
}
DWORD WINAPI RemoteInitFilterThunkEnd() { return 0; }
#pragma check_stack()
#pragma runtime_checks("", restore)
#pragma optimize("", on)
#pragma code_seg()

uintptr_t ResolveRipRelativePointer(uintptr_t instruction, int displacementOffset,
                                    int instructionSize) {
  const int32_t displacement =
      MemoryManager::Read<int32_t>(instruction + displacementOffset);
  if (displacement == 0) {
    return 0;
  }

  return instruction + instructionSize + displacement;
}

bool EnsureRemoteBuffer(void *&remoteAddress, size_t size, const void *initialData,
                        DWORD protect = PAGE_EXECUTE_READWRITE) {
  if (remoteAddress != nullptr) {
    return true;
  }

  remoteAddress = Process::AllocRemote(size, protect);
  if (!remoteAddress) {
    return false;
  }

  if (initialData != nullptr &&
      !MemoryManager::WriteRaw(reinterpret_cast<uintptr_t>(remoteAddress),
                               initialData, size)) {
    Process::FreeRemote(remoteAddress);
    remoteAddress = nullptr;
    return false;
  }

  return true;
}

bool ExecuteRemoteProcedure(void *remoteStart, void *remoteCtx,
                            DWORD timeoutMs = 250) {
  if (s_pendingRemoteThread != nullptr) {
    const DWORD pendingWait = WaitForSingleObject(s_pendingRemoteThread, 0);
    if (pendingWait == WAIT_TIMEOUT) {
      return false;
    }
    CloseHandle(s_pendingRemoteThread);
    s_pendingRemoteThread = nullptr;
  }

  HANDLE thread = Process::CreateRemoteThreadSimple(remoteStart, remoteCtx);
  if (!thread) {
    return false;
  }

  const DWORD wait = WaitForSingleObject(thread, timeoutMs);
  if (wait == WAIT_TIMEOUT) {
    // Keep the handle and all remote buffers alive. A later call will reap the
    // thread after it completes; reusing its context while it runs is unsafe.
    s_pendingRemoteThread = thread;
    return false;
  }
  DWORD exitCode = 0;
  GetExitCodeThread(thread, &exitCode);
  CloseHandle(thread);
  return wait == WAIT_OBJECT_0 && exitCode == 1;
}

bool EnsureCoreInitialized() {
  const DWORD currentProcessId = Process::GetProcessId();
  if (s_initialized && currentProcessId != 0 &&
      s_initializedProcessId != currentProcessId) {
    VisibilityManager::ResetProcessState();
  }

  if (s_initialized) {
    return true;
  }
  if (s_failed) {
    return false;
  }

  const uintptr_t clientBase = Module::GetBaseAddress(L"client.dll");
  if (clientBase == 0) {
    return false;
  }

  s_traceShapeAddress =
      PatternScanner::FindPattern(L"client.dll", kTraceShapePattern);
  s_initTraceFilterAddress =
      PatternScanner::FindPattern(L"client.dll", kInitTraceEntitiesOnlyPattern);
  const uintptr_t managerInstruction =
      PatternScanner::FindPattern(L"client.dll", kGameTraceManagerPattern);

  if (s_traceShapeAddress == 0 || s_initTraceFilterAddress == 0 ||
      managerInstruction == 0) {
    Utils::Logger::Warn("VisibilityManager: failed to resolve TraceShape patterns");
    s_failed = true;
    return false;
  }

  const uintptr_t managerPtrAddress =
      ResolveRipRelativePointer(managerInstruction, 3, 7);
  s_gameTraceManagerAddress = MemoryManager::Read<uintptr_t>(managerPtrAddress);
  if (s_gameTraceManagerAddress <= Constants::MIN_VALID_ADDRESS) {
    Utils::Logger::Warn("VisibilityManager: failed to resolve game trace manager");
    s_failed = true;
    return false;
  }

  const size_t traceThunkSize =
      reinterpret_cast<uintptr_t>(&RemoteTraceShapeThunkEnd) -
      reinterpret_cast<uintptr_t>(&RemoteTraceShapeThunk);
  const size_t initThunkSize =
      reinterpret_cast<uintptr_t>(&RemoteInitFilterThunkEnd) -
      reinterpret_cast<uintptr_t>(&RemoteInitFilterThunk);
  if (traceThunkSize == 0 || traceThunkSize > kMaxRemoteThunkSize ||
      initThunkSize == 0 || initThunkSize > kMaxRemoteThunkSize) {
    Utils::Logger::Warn("VisibilityManager: compiler produced an unsafe thunk layout");
    s_failed = true;
    return false;
  }
  Ray defaultRay{};
  defaultRay.line.startOffset = {0.0f, 0.0f, 0.0f};
  defaultRay.line.radius = 0.0f;
  defaultRay.type = RAY_TYPE_LINE;
  TraceResult defaultTrace{};

  if (!EnsureRemoteBuffer(s_remoteTraceShapeShellcode, traceThunkSize,
                          reinterpret_cast<void *>(&RemoteTraceShapeThunk)) ||
      !EnsureRemoteBuffer(s_remoteTraceShapeCtx, sizeof(TraceShapeCtx), nullptr) ||
      !EnsureRemoteBuffer(s_remoteTraceResult, sizeof(TraceResult), &defaultTrace) ||
      !EnsureRemoteBuffer(s_remoteRay, sizeof(Ray), &defaultRay) ||
      !EnsureRemoteBuffer(s_remoteInitFilterShellcode, initThunkSize,
                          reinterpret_cast<void *>(&RemoteInitFilterThunk)) ||
      !EnsureRemoteBuffer(s_remoteInitFilterCtx, sizeof(InitTraceFilterCtx),
                          nullptr)) {
    Utils::Logger::Warn("VisibilityManager: failed to allocate remote buffers");
    s_failed = true;
    return false;
  }

  s_initialized = true;
  s_initializedProcessId = currentProcessId;
  Utils::Logger::Info("VisibilityManager: TraceShape visibility ready");
  return true;
}

bool EnsureTraceFilter(uintptr_t localPawn) {
  if (!EnsureCoreInitialized() || localPawn <= Constants::MIN_VALID_ADDRESS) {
    return false;
  }

  if (s_traceFilterLocalPawn == localPawn && s_remoteTraceFilter != nullptr) {
    return true;
  }

  if (s_remoteTraceFilter != nullptr) {
    Process::FreeRemote(s_remoteTraceFilter);
    s_remoteTraceFilter = nullptr;
  }

  s_remoteTraceFilter = static_cast<TraceFilter *>(
      Process::AllocRemote(sizeof(TraceFilter), PAGE_READWRITE));
  if (!s_remoteTraceFilter) {
    return false;
  }

  InitTraceFilterCtx ctx{};
  ctx.filter = s_remoteTraceFilter;
  ctx.fn = reinterpret_cast<InitTraceFilterFn>(s_initTraceFilterAddress);
  ctx.skipPawn = localPawn;
  ctx.mask = static_cast<uint32_t>(MASK_PLAYER_VISIBLE);
  ctx.layer = kTraceLayer;

  if (!MemoryManager::WriteRaw(reinterpret_cast<uintptr_t>(s_remoteInitFilterCtx),
                               &ctx, sizeof(ctx)) ||
      !ExecuteRemoteProcedure(s_remoteInitFilterShellcode, s_remoteInitFilterCtx,
                              500)) {
    Process::FreeRemote(s_remoteTraceFilter);
    s_remoteTraceFilter = nullptr;
    return false;
  }

  s_traceFilterLocalPawn = localPawn;
  return true;
}

bool RunTraceShape(uintptr_t localPawn, uintptr_t enemyPawn,
                   const SDK::Vector3 &start, const SDK::Vector3 &end,
                   bool &visible) {
  visible = false;
  if (!EnsureTraceFilter(localPawn)) {
    return false;
  }

  Ray ray{};
  ray.line.startOffset = {0.0f, 0.0f, 0.0f};
  ray.line.radius = 0.0f;
  ray.type = RAY_TYPE_LINE;
  if (!MemoryManager::WriteRaw(reinterpret_cast<uintptr_t>(s_remoteRay), &ray,
                               sizeof(ray))) {
    return false;
  }

  TraceShapeCtx ctx{};
  ctx.traceShape = reinterpret_cast<TraceShapeFn>(s_traceShapeAddress);
  ctx.gameTraceManager = reinterpret_cast<void *>(s_gameTraceManagerAddress);
  ctx.traceFilter = s_remoteTraceFilter;
  ctx.start = start;
  ctx.end = end;
  ctx.traceResult = static_cast<TraceResult *>(s_remoteTraceResult);
  ctx.ray = static_cast<Ray *>(s_remoteRay);

  if (!MemoryManager::WriteRaw(reinterpret_cast<uintptr_t>(s_remoteTraceShapeCtx),
                               &ctx, sizeof(ctx)) ||
      !ExecuteRemoteProcedure(s_remoteTraceShapeShellcode, s_remoteTraceShapeCtx,
                              500)) {
    return false;
  }

  TraceResult trace{};
  if (!MemoryManager::ReadRaw(reinterpret_cast<uintptr_t>(s_remoteTraceResult), &trace,
                              sizeof(trace))) {
    return false;
  }

  const bool reachedEnemy = trace.hitEntity == enemyPawn;
  const bool noBlockingHit = !trace.didHit;
  const bool nearlyReachedTarget = trace.fraction >= 0.97f;
  visible = reachedEnemy || noBlockingHit || nearlyReachedTarget;
  return true;
}

CacheKey BuildCacheKey(uintptr_t enemyPawn, const SDK::Vector3 &end) {
  constexpr float kQuantize = 4.0f;
  return {
      enemyPawn,
      static_cast<int32_t>(std::lround(end.x / kQuantize)),
      static_cast<int32_t>(std::lround(end.y / kQuantize)),
      static_cast<int32_t>(std::lround(end.z / kQuantize)),
  };
}

} // namespace

void VisibilityManager::PrepareFrame(float nowSeconds) {
  if (std::abs(nowSeconds - s_frameTime) > 0.0001f) {
    s_frameTime = nowSeconds;
    s_remainingBudget = kMaxTracesPerFrame;
  }
}

VisibilityResult VisibilityManager::QueryPlayerVisibility(
    uintptr_t localPawn, uintptr_t enemyPawn, const SDK::Vector3 &start,
    const SDK::Vector3 &end) {
  VisibilityResult result{};
  if (localPawn <= Constants::MIN_VALID_ADDRESS ||
      enemyPawn <= Constants::MIN_VALID_ADDRESS) {
    return result;
  }

  const CacheKey key = BuildCacheKey(enemyPawn, end);
  auto &cache = s_cache[key];
  if (cache.checkedAt > 0.0f &&
      (s_frameTime - cache.checkedAt) <= kVisibilityCacheSeconds) {
    result.hasValue = true;
    result.visible = cache.visible;
    return result;
  }

  if (s_remainingBudget <= 0 && cache.checkedAt > 0.0f) {
    result.hasValue = true;
    result.visible = cache.visible;
    return result;
  }

  bool visible = false;
  if (!RunTraceShape(localPawn, enemyPawn, start, end, visible)) {
    return result;
  }

  --s_remainingBudget;
  cache.visible = visible;
  cache.checkedAt = s_frameTime;
  result.hasValue = true;
  result.visible = visible;
  return result;
}

bool VisibilityManager::IsAvailable() { return EnsureCoreInitialized(); }

void VisibilityManager::ResetFrameState() {
  s_cache.clear();
  s_frameTime = -1.0f;
  s_remainingBudget = kMaxTracesPerFrame;
}

void VisibilityManager::ResetProcessState() {
  ResetFrameState();
  s_traceFilterLocalPawn = 0;
  bool canFreeRemoteBuffers = true;
  if (s_pendingRemoteThread != nullptr) {
    const DWORD wait = WaitForSingleObject(s_pendingRemoteThread, 0);
    canFreeRemoteBuffers = wait != WAIT_TIMEOUT;
    CloseHandle(s_pendingRemoteThread);
    s_pendingRemoteThread = nullptr;
  }
  if (s_remoteTraceFilter != nullptr) {
    if (canFreeRemoteBuffers) Process::FreeRemote(s_remoteTraceFilter);
    s_remoteTraceFilter = nullptr;
  }
  if (s_remoteTraceShapeShellcode != nullptr) {
    if (canFreeRemoteBuffers) Process::FreeRemote(s_remoteTraceShapeShellcode);
    s_remoteTraceShapeShellcode = nullptr;
  }
  if (s_remoteTraceShapeCtx != nullptr) {
    if (canFreeRemoteBuffers) Process::FreeRemote(s_remoteTraceShapeCtx);
    s_remoteTraceShapeCtx = nullptr;
  }
  if (s_remoteTraceResult != nullptr) {
    if (canFreeRemoteBuffers) Process::FreeRemote(s_remoteTraceResult);
    s_remoteTraceResult = nullptr;
  }
  if (s_remoteRay != nullptr) {
    if (canFreeRemoteBuffers) Process::FreeRemote(s_remoteRay);
    s_remoteRay = nullptr;
  }
  if (s_remoteInitFilterShellcode != nullptr) {
    if (canFreeRemoteBuffers) Process::FreeRemote(s_remoteInitFilterShellcode);
    s_remoteInitFilterShellcode = nullptr;
  }
  if (s_remoteInitFilterCtx != nullptr) {
    if (canFreeRemoteBuffers) Process::FreeRemote(s_remoteInitFilterCtx);
    s_remoteInitFilterCtx = nullptr;
  }
  s_traceShapeAddress = 0;
  s_initTraceFilterAddress = 0;
  s_gameTraceManagerAddress = 0;
  s_initialized = false;
  s_failed = false;
  s_initializedProcessId = 0;
}

} // namespace Core
