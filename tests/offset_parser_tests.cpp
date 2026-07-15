#include "core/sdk/offset_parser.h"
#include <cassert>
#include <iostream>

int main() {
  SDK::OffsetParser parser;
  const std::string offsets = R"({
    "client.dll": {
      "dwEntityList": 4096,
      "dwLocalPlayerPawn": 8192,
      "dwViewMatrix": 12288
    }
  })";
  const std::string client = R"({
    "client.dll": { "classes": {
      "C_BaseEntity": { "fields": {
        "m_fFlags": 16, "m_vecVelocity": 20, "m_iHealth": 24,
        "m_iTeamNum": 28, "m_pGameSceneNode": 36,
        "m_flSimulationTime": 56
      } },
      "C_CSPlayerPawn": { "fields": {
        "m_vecViewOffset": 52, "m_entitySpottedState": 64
      } },
      "C_BasePlayerPawn": { "fields": { "m_vOldOrigin": 32 } },
      "CCSPlayerController": { "fields": {
        "m_hPlayerPawn": 44, "m_iszPlayerName": 48
      } },
      "CBasePlayerController": { "fields": {
        "m_bIsLocalPlayerController": 60
      } },
      "EntitySpottedState_t": { "fields": { "m_bSpottedByMask": 12 } },
      "UnrelatedClass": { "fields": { "m_iHealth": 9999 } }
    } }
  })";

  SDK::OffsetFileLoader::FileResult files;
  files.offsetsJson = offsets;
  files.clientJson = client;
  const SDK::OffsetSet parsed = parser.Parse(files);
  assert(parsed.HasRequired());
  assert(parsed.dwEntityList == 4096);
  assert(parsed.m_iHealth == 24);
  assert(parsed.m_vOldOrigin == 32);
  assert(parsed.m_bIsLocalPlayerController == 60);
  assert(parsed.m_entitySpottedState == 64);
  assert(parsed.m_bSpottedByMask == 12);

  SDK::OffsetFileLoader::FileResult invalidFiles;
  invalidFiles.offsetsJson = "{}";
  invalidFiles.clientJson = "{}";
  const SDK::OffsetSet invalid = parser.Parse(invalidFiles);
  assert(!invalid.HasRequired());

  SDK::OffsetFileLoader::FileResult unsafeFiles;
  unsafeFiles.offsetsJson = R"({"client.dll":{"dwEntityList":-1}})";
  unsafeFiles.clientJson = "{}";
  assert(!parser.Parse(unsafeFiles).HasRequired());
  std::cout << "offset parser tests passed\n";
  return 0;
}
