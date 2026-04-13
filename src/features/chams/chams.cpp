#include "chams.h"
#include "chams_config.h"
#include "config/settings.h"
#include "core/constants.h"
#include "core/game/game_manager.h"
#include "core/memory/memory_manager.h"
#include "core/sdk/entity.h"
#include "core/sdk/offsets.h"
#include "core/sdk/structs.h"
#include "render/overlay/overlay.h"
#include "render/renderer/renderer.h"
#include "utils/logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <windows.h>
#include <wrl/client.h>

namespace Features {

namespace {

using Microsoft::WRL::ComPtr;
using json = nlohmann::json;

constexpr int kMaxBones = 128;
constexpr float kTeleportDistanceSq = 10000.0f;

struct ChamsSnapshot {
  bool enabled = false;
  bool showTeammates = false;
  bool wireframe = false;
  bool visibleCheck = true;
  int materialType = 1;
  float alpha = 1.0f;
  float wireAmount = 0.85f;
  float fillColor[4] = {};
  float hiddenColor[4] = {};
  float fillColorTeam[4] = {};
  float hiddenColorTeam[4] = {};
  float wireColor[4] = {};
};

struct BoneMatrix3x4 {
  float m[3][4]{};

  static BoneMatrix3x4 Identity() {
    BoneMatrix3x4 out{};
    out.m[0][0] = 1.0f;
    out.m[1][1] = 1.0f;
    out.m[2][2] = 1.0f;
    return out;
  }

  static BoneMatrix3x4 Lerp(const BoneMatrix3x4 &a, const BoneMatrix3x4 &b,
                            float t) {
    BoneMatrix3x4 out{};
    for (int row = 0; row < 3; ++row) {
      for (int col = 0; col < 4; ++col) {
        out.m[row][col] = a.m[row][col] + (b.m[row][col] - a.m[row][col]) * t;
      }
    }
    return out;
  }

  BoneMatrix3x4 Multiply(const BoneMatrix3x4 &other) const {
    BoneMatrix3x4 out{};
    for (int row = 0; row < 3; ++row) {
      out.m[row][0] = m[row][0] * other.m[0][0] + m[row][1] * other.m[1][0] +
                      m[row][2] * other.m[2][0];
      out.m[row][1] = m[row][0] * other.m[0][1] + m[row][1] * other.m[1][1] +
                      m[row][2] * other.m[2][1];
      out.m[row][2] = m[row][0] * other.m[0][2] + m[row][1] * other.m[1][2] +
                      m[row][2] * other.m[2][2];
      out.m[row][3] = m[row][0] * other.m[0][3] + m[row][1] * other.m[1][3] +
                      m[row][2] * other.m[2][3] + m[row][3];
    }
    return out;
  }

  SDK::Vector3 GetOrigin() const { return {m[0][3], m[1][3], m[2][3]}; }

  bool IsValid() const {
    for (const auto &row : m) {
      for (float value : row) {
        if (!std::isfinite(value)) {
          return false;
        }
      }
    }

    return std::abs(m[0][3]) < 50000.0f && std::abs(m[1][3]) < 50000.0f &&
           std::abs(m[2][3]) < 50000.0f;
  }

  void To4x4RowMajor(float out[16]) const {
    out[0] = m[0][0];
    out[1] = m[0][1];
    out[2] = m[0][2];
    out[3] = m[0][3];
    out[4] = m[1][0];
    out[5] = m[1][1];
    out[6] = m[1][2];
    out[7] = m[1][3];
    out[8] = m[2][0];
    out[9] = m[2][1];
    out[10] = m[2][2];
    out[11] = m[2][3];
    out[12] = 0.0f;
    out[13] = 0.0f;
    out[14] = 0.0f;
    out[15] = 1.0f;
  }
};

struct MeshVertex {
  SDK::Vector3 position{};
  SDK::Vector3 normal{};
  std::array<uint16_t, 4> jointIndices{};
  std::array<float, 4> weights{};
};

struct GpuVertex {
  float position[3];
  float normal[3];
  uint32_t boneIndices = 0;
  float weights[4];
};

struct SkinnedMesh {
  std::vector<MeshVertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<BoneMatrix3x4> inverseBindMatrices;
  std::vector<BoneMatrix3x4> bindPoseMatrices;
  std::vector<int16_t> gltfToGameBoneMap;
  ComPtr<ID3D11Buffer> vertexBuffer;
  ComPtr<ID3D11Buffer> indexBuffer;
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;

  bool IsLoaded() const { return !vertices.empty() && !indices.empty(); }

  void Reset() {
    vertices.clear();
    indices.clear();
    inverseBindMatrices.clear();
    bindPoseMatrices.clear();
    gltfToGameBoneMap.clear();
    vertexBuffer.Reset();
    indexBuffer.Reset();
    vertexCount = 0;
    indexCount = 0;
  }
};

struct DrawCommand {
  SkinnedMesh *mesh = nullptr;
  std::vector<BoneMatrix3x4> combinedMatrices;
  float fillColor[4] = {};
  float wireColor[4] = {};
  float alpha = 1.0f;
  int renderMode = 0;
  int materialType = 1;
};

struct GameBoneTransform {
  SDK::Vector3 position{};
  float scale = 1.0f;
  float qx = 0.0f;
  float qy = 0.0f;
  float qz = 0.0f;
  float qw = 1.0f;
};

struct LiveBonePose {
  BoneMatrix3x4 matrix = BoneMatrix3x4::Identity();
  bool hasOrientation = false;
};

struct AccessorInfo {
  int bufferView = -1;
  int byteOffset = 0;
  int componentType = 0;
  int count = 0;
  std::string type;
};

struct BufferViewInfo {
  int byteOffset = 0;
  int byteLength = 0;
  int byteStride = 0;
};

struct PrimitiveSource {
  size_t baseVertex = 0;
  size_t vertexCount = 0;
  int normalAccessor = -1;
};

struct CBViewProjection {
  float viewProjection[4][4];
  float screenWidth = 0.0f;
  float screenHeight = 0.0f;
  float pad[2]{};
};

struct CBBones {
  float bones[kMaxBones][16];
};

struct CBMaterial {
  float fillColor[4];
  float wireColor[4];
  int renderMode = 0;
  float alpha = 1.0f;
  int materialType = 1;
  float rimPower = 2.5f;
  float cameraPos[3];
  float pad = 0.0f;
};

struct RendererStateGuard {
  explicit RendererStateGuard(ID3D11DeviceContext *context) : ctx(context) {
    if (!ctx) {
      return;
    }

    ctx->IAGetPrimitiveTopology(&topology);
    ctx->IAGetInputLayout(inputLayout.GetAddressOf());
    ctx->VSGetShader(vertexShader.GetAddressOf(), nullptr, nullptr);
    ctx->PSGetShader(pixelShader.GetAddressOf(), nullptr, nullptr);
    ctx->VSGetConstantBuffers(0, 3, vsConstantBuffers);
    ctx->PSGetConstantBuffers(0, 3, psConstantBuffers);
    ctx->IAGetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &vertexStride,
                            &vertexOffset);
    ctx->IAGetIndexBuffer(indexBuffer.GetAddressOf(), &indexFormat,
                          &indexOffset);
    ctx->RSGetState(rasterizerState.GetAddressOf());
    viewportCount = 1;
    ctx->RSGetViewports(&viewportCount, &viewport);
    ctx->OMGetBlendState(blendState.GetAddressOf(), blendFactor, &sampleMask);
    ctx->OMGetDepthStencilState(depthStencilState.GetAddressOf(), &stencilRef);
    ctx->OMGetRenderTargets(1, renderTargetView.GetAddressOf(),
                            depthStencilView.GetAddressOf());
  }

  ~RendererStateGuard() {
    if (!ctx) {
      return;
    }

    ID3D11Buffer *vsCbs[3] = {vsConstantBuffers[0], vsConstantBuffers[1],
                              vsConstantBuffers[2]};
    ID3D11Buffer *psCbs[3] = {psConstantBuffers[0], psConstantBuffers[1],
                              psConstantBuffers[2]};
    ID3D11Buffer *vb = vertexBuffer.Get();
    ID3D11Buffer *ib = indexBuffer.Get();
    ID3D11RenderTargetView *rtv = renderTargetView.Get();

    ctx->IASetPrimitiveTopology(topology);
    ctx->IASetInputLayout(inputLayout.Get());
    ctx->VSSetShader(vertexShader.Get(), nullptr, 0);
    ctx->PSSetShader(pixelShader.Get(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 3, vsCbs);
    ctx->PSSetConstantBuffers(0, 3, psCbs);
    ctx->IASetVertexBuffers(0, 1, &vb, &vertexStride, &vertexOffset);
    ctx->IASetIndexBuffer(ib, indexFormat, indexOffset);
    ctx->RSSetState(rasterizerState.Get());
    ctx->RSSetViewports(viewportCount, &viewport);
    ctx->OMSetBlendState(blendState.Get(), blendFactor, sampleMask);
    ctx->OMSetDepthStencilState(depthStencilState.Get(), stencilRef);
    ctx->OMSetRenderTargets(1, &rtv, depthStencilView.Get());

    for (auto *&buffer : vsConstantBuffers) {
      if (buffer) {
        buffer->Release();
        buffer = nullptr;
      }
    }
    for (auto *&buffer : psConstantBuffers) {
      if (buffer) {
        buffer->Release();
        buffer = nullptr;
      }
    }
  }

  ID3D11DeviceContext *ctx = nullptr;
  D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
  ComPtr<ID3D11InputLayout> inputLayout;
  ComPtr<ID3D11VertexShader> vertexShader;
  ComPtr<ID3D11PixelShader> pixelShader;
  ID3D11Buffer *vsConstantBuffers[3]{};
  ID3D11Buffer *psConstantBuffers[3]{};
  ComPtr<ID3D11Buffer> vertexBuffer;
  UINT vertexStride = 0;
  UINT vertexOffset = 0;
  ComPtr<ID3D11Buffer> indexBuffer;
  DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
  UINT indexOffset = 0;
  ComPtr<ID3D11RasterizerState> rasterizerState;
  D3D11_VIEWPORT viewport{};
  UINT viewportCount = 0;
  ComPtr<ID3D11BlendState> blendState;
  float blendFactor[4]{};
  UINT sampleMask = 0;
  ComPtr<ID3D11DepthStencilState> depthStencilState;
  UINT stencilRef = 0;
  ComPtr<ID3D11RenderTargetView> renderTargetView;
  ComPtr<ID3D11DepthStencilView> depthStencilView;
};

static const char *kShaderSource = R"(
cbuffer CBViewProjection : register(b0) {
  row_major float4x4 g_ViewProjection;
  float g_ScreenW;
  float g_ScreenH;
  float2 _pad0;
};

cbuffer CBBoneMatrices : register(b1) {
  row_major float4x4 g_BoneMatrices[128];
};

cbuffer CBMaterial : register(b2) {
  float4 g_FillColor;
  float4 g_WireColor;
  int g_RenderMode;
  float g_Alpha;
  int g_MaterialType;
  float g_RimPower;
  float3 g_CamPos;
  float _pad1;
};

struct VS_INPUT {
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  uint4 BoneIndices : BLENDINDICES;
  float4 BoneWeights : BLENDWEIGHT;
};

struct PS_INPUT {
  float4 Position : SV_POSITION;
  float3 WorldPos : TEXCOORD0;
  float3 WorldNormal : TEXCOORD1;
};

PS_INPUT VS_Skinning(VS_INPUT input) {
  PS_INPUT output = (PS_INPUT)0;
  float4 skinnedPos = float4(0, 0, 0, 0);
  float3 skinnedNormal = float3(0, 0, 0);

  [unroll]
  for (int i = 0; i < 4; ++i) {
    float weight = input.BoneWeights[i];
    if (weight > 0.0001f) {
      uint idx = input.BoneIndices[i];
      skinnedPos += mul(g_BoneMatrices[idx], float4(input.Position, 1.0f)) * weight;
      skinnedNormal += mul((float3x3)g_BoneMatrices[idx], input.Normal) * weight;
    }
  }

  output.WorldPos = skinnedPos.xyz;
  output.WorldNormal = normalize(skinnedNormal);
  output.Position = mul(g_ViewProjection, skinnedPos);
  return output;
}

float4 PS_Fill(PS_INPUT input) : SV_TARGET {
  float3 normalDir = normalize(input.WorldNormal);
  float3 baseRgb = g_FillColor.rgb;
  float baseAlpha = g_FillColor.a * g_Alpha;

  if (g_MaterialType == 0) {
    return float4(baseRgb, baseAlpha);
  }

  if (g_MaterialType == 1) {
    float3 lightDir = normalize(float3(0.3f, 1.0f, 0.5f));
    float ndl = saturate(dot(normalDir, lightDir));
    float lighting = 0.35f + 0.65f * ndl;
    return float4(baseRgb * lighting, baseAlpha);
  }

  float3 viewDir = normalize(g_CamPos - input.WorldPos);
  float fresnel = pow(1.0f - saturate(dot(normalDir, viewDir)), g_RimPower);
  float3 rimRgb = lerp(baseRgb, float3(1, 1, 1), fresnel * 0.7f);
  float rimAlpha = baseAlpha + fresnel * (1.0f - baseAlpha) * 0.5f;
  return float4(rimRgb, saturate(rimAlpha));
}

float4 PS_Wire(PS_INPUT input) : SV_TARGET {
  return float4(g_WireColor.rgb, g_WireColor.a * g_Alpha);
}
)";

template <typename T>
void CopyColor(T &dst, const float (&src)[4]) {
  std::copy(std::begin(src), std::end(src), std::begin(dst));
}

ChamsSnapshot SnapshotChams() {
  ChamsSnapshot snapshot{};
  std::shared_lock<std::shared_mutex> lock(Config::SettingsMutex);
  const auto &settings = Config::Settings.chams;
  snapshot.enabled = settings.enabled;
  snapshot.showTeammates = settings.showTeammates;
  snapshot.wireframe = settings.wireframe;
  snapshot.visibleCheck = settings.visibleCheck;
  snapshot.materialType = settings.materialType;
  snapshot.alpha = std::clamp(settings.alpha, 0.10f, 1.0f);
  snapshot.wireAmount = std::clamp(settings.wireAmount, 0.10f, 1.0f);
  CopyColor(snapshot.fillColor, settings.fillColor);
  CopyColor(snapshot.hiddenColor, settings.hiddenColor);
  CopyColor(snapshot.fillColorTeam, settings.fillColorTeam);
  CopyColor(snapshot.hiddenColorTeam, settings.hiddenColorTeam);
  CopyColor(snapshot.wireColor, settings.wireColor);
  snapshot.wireColor[3] =
      std::clamp(snapshot.wireColor[3] * snapshot.wireAmount, 0.0f, 1.0f);
  return snapshot;
}

std::filesystem::path FindAssetPath(const std::string &filename) {
  char exePath[MAX_PATH] = {};
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  std::filesystem::path current = std::filesystem::path(exePath).parent_path();

  for (int i = 0; i < 6; ++i) {
    const auto candidate = current / "assets" / "models" / filename;
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
    if (!current.has_parent_path()) {
      break;
    }
    current = current.parent_path();
  }

  const auto cwdCandidate =
      std::filesystem::current_path() / "assets" / "models" / filename;
  if (std::filesystem::exists(cwdCandidate)) {
    return cwdCandidate;
  }

  return {};
}

BoneMatrix3x4 ConvertInverseBindMatrix(const float *columnMajor4x4) {
  BoneMatrix3x4 out{};
  out.m[0][0] = columnMajor4x4[0];
  out.m[0][1] = columnMajor4x4[4];
  out.m[0][2] = columnMajor4x4[8];
  out.m[0][3] = columnMajor4x4[12];
  out.m[1][0] = columnMajor4x4[1];
  out.m[1][1] = columnMajor4x4[5];
  out.m[1][2] = columnMajor4x4[9];
  out.m[1][3] = columnMajor4x4[13];
  out.m[2][0] = columnMajor4x4[2];
  out.m[2][1] = columnMajor4x4[6];
  out.m[2][2] = columnMajor4x4[10];
  out.m[2][3] = columnMajor4x4[14];
  return out;
}

BoneMatrix3x4 InvertRigidMatrix(const BoneMatrix3x4 &matrix) {
  BoneMatrix3x4 out{};

  out.m[0][0] = matrix.m[0][0];
  out.m[0][1] = matrix.m[1][0];
  out.m[0][2] = matrix.m[2][0];
  out.m[1][0] = matrix.m[0][1];
  out.m[1][1] = matrix.m[1][1];
  out.m[1][2] = matrix.m[2][1];
  out.m[2][0] = matrix.m[0][2];
  out.m[2][1] = matrix.m[1][2];
  out.m[2][2] = matrix.m[2][2];

  const float tx = matrix.m[0][3];
  const float ty = matrix.m[1][3];
  const float tz = matrix.m[2][3];
  out.m[0][3] = -(out.m[0][0] * tx + out.m[0][1] * ty + out.m[0][2] * tz);
  out.m[1][3] = -(out.m[1][0] * tx + out.m[1][1] * ty + out.m[1][2] * tz);
  out.m[2][3] = -(out.m[2][0] * tx + out.m[2][1] * ty + out.m[2][2] * tz);
  return out;
}

SDK::Vector3 operator+(const SDK::Vector3 &a, const SDK::Vector3 &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

SDK::Vector3 operator-(const SDK::Vector3 &a, const SDK::Vector3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

SDK::Vector3 &operator+=(SDK::Vector3 &a, const SDK::Vector3 &b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
}

SDK::Vector3 Cross(const SDK::Vector3 &a, const SDK::Vector3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

void Normalize(SDK::Vector3 &value) {
  const float length =
      std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
  if (length > 0.0001f) {
    const float invLength = 1.0f / length;
    value.x *= invLength;
    value.y *= invLength;
    value.z *= invLength;
  }
}

int ComponentCountForType(const std::string &type) {
  if (type == "SCALAR")
    return 1;
  if (type == "VEC2")
    return 2;
  if (type == "VEC3")
    return 3;
  if (type == "VEC4")
    return 4;
  if (type == "MAT4")
    return 16;
  return 1;
}

int ComponentSize(int componentType) {
  switch (componentType) {
  case 5121:
    return 1;
  case 5123:
    return 2;
  case 5125:
  case 5126:
    return 4;
  default:
    return 0;
  }
}

size_t AccessorStride(const AccessorInfo &accessor,
                      const BufferViewInfo &bufferView) {
  if (bufferView.byteStride > 0) {
    return static_cast<size_t>(bufferView.byteStride);
  }
  return static_cast<size_t>(ComponentCountForType(accessor.type) *
                             ComponentSize(accessor.componentType));
}

template <typename T>
const T *AccessorElement(const uint8_t *binData,
                         const std::vector<AccessorInfo> &accessors,
                         const std::vector<BufferViewInfo> &bufferViews,
                         int accessorIndex, int elementIndex) {
  const auto &accessor = accessors[accessorIndex];
  const auto &bufferView = bufferViews[accessor.bufferView];
  const size_t stride = AccessorStride(accessor, bufferView);
  const uint8_t *base =
      binData + bufferView.byteOffset + accessor.byteOffset + stride * elementIndex;
  return reinterpret_cast<const T *>(base);
}

bool LoadGlbMesh(const std::filesystem::path &path, SkinnedMesh &mesh) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return false;
  }

  const size_t fileSize = static_cast<size_t>(file.tellg());
  file.seekg(0);
  std::vector<uint8_t> fileData(fileSize);
  file.read(reinterpret_cast<char *>(fileData.data()),
            static_cast<std::streamsize>(fileSize));
  file.close();

  if (fileSize < 20) {
    return false;
  }

  const uint32_t magic = *reinterpret_cast<const uint32_t *>(&fileData[0]);
  const uint32_t version = *reinterpret_cast<const uint32_t *>(&fileData[4]);
  const uint32_t totalLength = *reinterpret_cast<const uint32_t *>(&fileData[8]);
  if (magic != 0x46546C67 || version != 2 || totalLength > fileSize) {
    return false;
  }

  size_t chunkOffset = 12;
  if (chunkOffset + 8 > fileSize) {
    return false;
  }

  const uint32_t jsonLength =
      *reinterpret_cast<const uint32_t *>(&fileData[chunkOffset]);
  const uint32_t jsonType =
      *reinterpret_cast<const uint32_t *>(&fileData[chunkOffset + 4]);
  chunkOffset += 8;
  if (jsonType != 0x4E4F534A || chunkOffset + jsonLength > fileSize) {
    return false;
  }

  std::string jsonString(reinterpret_cast<char *>(&fileData[chunkOffset]),
                         jsonLength);
  chunkOffset += jsonLength;

  if (chunkOffset + 8 > fileSize) {
    return false;
  }

  const uint32_t binLength =
      *reinterpret_cast<const uint32_t *>(&fileData[chunkOffset]);
  const uint32_t binType =
      *reinterpret_cast<const uint32_t *>(&fileData[chunkOffset + 4]);
  chunkOffset += 8;
  if (binType != 0x004E4942 || chunkOffset + binLength > fileSize) {
    return false;
  }

  json gltf;
  try {
    gltf = json::parse(jsonString);
  } catch (...) {
    return false;
  }

  if (!gltf.contains("skins") || gltf["skins"].empty() ||
      !gltf.contains("bufferViews") || !gltf.contains("accessors") ||
      !gltf.contains("nodes") || !gltf.contains("meshes")) {
    return false;
  }

  std::vector<BufferViewInfo> bufferViews;
  for (const auto &bufferView : gltf["bufferViews"]) {
    bufferViews.push_back({bufferView.value("byteOffset", 0),
                           bufferView.value("byteLength", 0),
                           bufferView.value("byteStride", 0)});
  }

  std::vector<AccessorInfo> accessors;
  for (const auto &accessor : gltf["accessors"]) {
    if (!accessor.contains("bufferView")) {
      return false;
    }
    accessors.push_back({accessor["bufferView"].get<int>(),
                         accessor.value("byteOffset", 0),
                         accessor.value("componentType", 0),
                         accessor.value("count", 0),
                         accessor.value("type", std::string("SCALAR"))});
  }

  const auto &skin = gltf["skins"][0];
  if (!skin.contains("joints") || !skin.contains("inverseBindMatrices")) {
    return false;
  }

  const int ibmIndex = skin["inverseBindMatrices"].get<int>();
  const auto &ibmAccessor = accessors[ibmIndex];
  mesh.inverseBindMatrices.resize(static_cast<size_t>(ibmAccessor.count));
  mesh.bindPoseMatrices.resize(static_cast<size_t>(ibmAccessor.count));
  const uint8_t *binData = &fileData[chunkOffset];
  for (int i = 0; i < ibmAccessor.count; ++i) {
    const float *matrixPtr = AccessorElement<float>(
        binData, accessors, bufferViews, ibmIndex, i);
    mesh.inverseBindMatrices[static_cast<size_t>(i)] =
        ConvertInverseBindMatrix(matrixPtr);
    mesh.bindPoseMatrices[static_cast<size_t>(i)] =
        InvertRigidMatrix(mesh.inverseBindMatrices[static_cast<size_t>(i)]);
  }

  mesh.gltfToGameBoneMap.resize(skin["joints"].size());
  for (size_t i = 0; i < mesh.gltfToGameBoneMap.size(); ++i) {
    mesh.gltfToGameBoneMap[i] = static_cast<int16_t>(i);
  }

  std::vector<PrimitiveSource> primitiveSources;
  for (const auto &node : gltf["nodes"]) {
    if (!node.contains("mesh") || !node.contains("skin")) {
      continue;
    }

    const auto &gltfMesh = gltf["meshes"][node["mesh"].get<int>()];
    for (const auto &primitive : gltfMesh["primitives"]) {
      if (!primitive.contains("attributes") || !primitive.contains("indices")) {
        continue;
      }
      const auto &attributes = primitive["attributes"];
      if (!attributes.contains("POSITION") || !attributes.contains("JOINTS_0") ||
          !attributes.contains("WEIGHTS_0")) {
        continue;
      }

      const int positionAccessor = attributes["POSITION"].get<int>();
      const int jointsAccessor = attributes["JOINTS_0"].get<int>();
      const int weightsAccessor = attributes["WEIGHTS_0"].get<int>();
      const int indicesAccessor = primitive["indices"].get<int>();
      const int normalAccessor =
          attributes.contains("NORMAL") ? attributes["NORMAL"].get<int>() : -1;

      const auto &positionInfo = accessors[positionAccessor];
      const auto &jointsInfo = accessors[jointsAccessor];
      const auto &indicesInfo = accessors[indicesAccessor];

      const uint32_t baseVertex = static_cast<uint32_t>(mesh.vertices.size());
      primitiveSources.push_back(
          {mesh.vertices.size(), static_cast<size_t>(positionInfo.count), normalAccessor});

      for (int vertexIndex = 0; vertexIndex < positionInfo.count; ++vertexIndex) {
        MeshVertex vertex{};
        const float *positionPtr = AccessorElement<float>(
            binData, accessors, bufferViews, positionAccessor, vertexIndex);
        vertex.position = {positionPtr[0], positionPtr[1], positionPtr[2]};

        if (jointsInfo.componentType == 5121) {
          const uint8_t *jointPtr = AccessorElement<uint8_t>(
              binData, accessors, bufferViews, jointsAccessor, vertexIndex);
          for (int i = 0; i < 4; ++i) {
            vertex.jointIndices[static_cast<size_t>(i)] = jointPtr[i];
          }
        } else {
          const uint16_t *jointPtr = AccessorElement<uint16_t>(
              binData, accessors, bufferViews, jointsAccessor, vertexIndex);
          for (int i = 0; i < 4; ++i) {
            vertex.jointIndices[static_cast<size_t>(i)] = jointPtr[i];
          }
        }

        const float *weightPtr = AccessorElement<float>(
            binData, accessors, bufferViews, weightsAccessor, vertexIndex);
        for (int i = 0; i < 4; ++i) {
          vertex.weights[static_cast<size_t>(i)] = weightPtr[i];
        }

        const float sum = vertex.weights[0] + vertex.weights[1] +
                          vertex.weights[2] + vertex.weights[3];
        if (sum > 0.0001f && std::abs(sum - 1.0f) > 0.001f) {
          const float inv = 1.0f / sum;
          for (float &weight : vertex.weights) {
            weight *= inv;
          }
        }

        mesh.vertices.push_back(vertex);
      }

      if (indicesInfo.componentType == 5123) {
        for (int i = 0; i < indicesInfo.count; ++i) {
          const uint16_t *indexPtr = AccessorElement<uint16_t>(
              binData, accessors, bufferViews, indicesAccessor, i);
          mesh.indices.push_back(baseVertex + *indexPtr);
        }
      } else if (indicesInfo.componentType == 5125) {
        for (int i = 0; i < indicesInfo.count; ++i) {
          const uint32_t *indexPtr = AccessorElement<uint32_t>(
              binData, accessors, bufferViews, indicesAccessor, i);
          mesh.indices.push_back(baseVertex + *indexPtr);
        }
      } else {
        return false;
      }
    }
  }

  if (mesh.vertices.empty() || mesh.indices.empty()) {
    return false;
  }

  for (auto &vertex : mesh.vertices) {
    vertex.normal = {0.0f, 0.0f, 0.0f};
  }

  for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    auto &v0 = mesh.vertices[mesh.indices[i]];
    auto &v1 = mesh.vertices[mesh.indices[i + 1]];
    auto &v2 = mesh.vertices[mesh.indices[i + 2]];
    const SDK::Vector3 edge1 = v1.position - v0.position;
    const SDK::Vector3 edge2 = v2.position - v0.position;
    const SDK::Vector3 normal = Cross(edge1, edge2);
    v0.normal += normal;
    v1.normal += normal;
    v2.normal += normal;
  }

  for (auto &vertex : mesh.vertices) {
    Normalize(vertex.normal);
  }

  for (const auto &primitiveSource : primitiveSources) {
    if (primitiveSource.normalAccessor < 0) {
      continue;
    }
    for (size_t i = 0; i < primitiveSource.vertexCount; ++i) {
      const float *normalPtr = AccessorElement<float>(
          binData, accessors, bufferViews, primitiveSource.normalAccessor,
          static_cast<int>(i));
      mesh.vertices[primitiveSource.baseVertex + i].normal = {normalPtr[0],
                                                              normalPtr[1],
                                                              normalPtr[2]};
    }
  }

  mesh.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
  mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
  return true;
}

bool UploadMeshToGpu(ID3D11Device *device, SkinnedMesh &mesh) {
  if (!device || !mesh.IsLoaded()) {
    return false;
  }

  std::vector<GpuVertex> gpuVertices(mesh.vertices.size());
  for (size_t i = 0; i < mesh.vertices.size(); ++i) {
    const auto &src = mesh.vertices[i];
    auto &dst = gpuVertices[i];
    dst.position[0] = src.position.x;
    dst.position[1] = src.position.y;
    dst.position[2] = src.position.z;
    dst.normal[0] = src.normal.x;
    dst.normal[1] = src.normal.y;
    dst.normal[2] = src.normal.z;
    dst.boneIndices =
        (static_cast<uint32_t>(src.jointIndices[0]) & 0xFFu) |
        ((static_cast<uint32_t>(src.jointIndices[1]) & 0xFFu) << 8u) |
        ((static_cast<uint32_t>(src.jointIndices[2]) & 0xFFu) << 16u) |
        ((static_cast<uint32_t>(src.jointIndices[3]) & 0xFFu) << 24u);
    std::copy(src.weights.begin(), src.weights.end(), dst.weights);
  }

  D3D11_BUFFER_DESC vertexDesc{};
  vertexDesc.ByteWidth =
      static_cast<UINT>(gpuVertices.size() * sizeof(GpuVertex));
  vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
  vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  D3D11_SUBRESOURCE_DATA vertexData{};
  vertexData.pSysMem = gpuVertices.data();
  if (FAILED(device->CreateBuffer(&vertexDesc, &vertexData,
                                  mesh.vertexBuffer.GetAddressOf()))) {
    return false;
  }

  D3D11_BUFFER_DESC indexDesc{};
  indexDesc.ByteWidth =
      static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
  indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
  indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  D3D11_SUBRESOURCE_DATA indexData{};
  indexData.pSysMem = mesh.indices.data();
  return SUCCEEDED(device->CreateBuffer(&indexDesc, &indexData,
                                        mesh.indexBuffer.GetAddressOf()));
}

BoneMatrix3x4 MakeTranslationBoneMatrix(const SDK::Vector3 &position) {
  BoneMatrix3x4 matrix = BoneMatrix3x4::Identity();
  matrix.m[0][3] = position.x;
  matrix.m[1][3] = position.y;
  matrix.m[2][3] = position.z;
  return matrix;
}

bool IsFiniteVector(const SDK::Vector3 &value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

BoneMatrix3x4 MakeOrientedBoneMatrix(const GameBoneTransform &transform,
                                     bool &hasOrientation) {
  hasOrientation = false;
  BoneMatrix3x4 matrix = MakeTranslationBoneMatrix(transform.position);

  const float quatLengthSq = transform.qx * transform.qx + transform.qy * transform.qy +
                             transform.qz * transform.qz + transform.qw * transform.qw;
  if (!std::isfinite(quatLengthSq) || quatLengthSq < 0.35f || quatLengthSq > 1.65f) {
    return matrix;
  }

  const float invQuatLength = 1.0f / std::sqrt(quatLengthSq);
  const float qx = transform.qx * invQuatLength;
  const float qy = transform.qy * invQuatLength;
  const float qz = transform.qz * invQuatLength;
  const float qw = transform.qw * invQuatLength;
  const float scale = std::isfinite(transform.scale) && transform.scale > 0.001f &&
                              transform.scale < 16.0f
                          ? transform.scale
                          : 1.0f;

  const float xx = qx * qx;
  const float yy = qy * qy;
  const float zz = qz * qz;
  const float xy = qx * qy;
  const float xz = qx * qz;
  const float yz = qy * qz;
  const float wx = qw * qx;
  const float wy = qw * qy;
  const float wz = qw * qz;

  matrix.m[0][0] = (1.0f - 2.0f * (yy + zz)) * scale;
  matrix.m[0][1] = (2.0f * (xy - wz)) * scale;
  matrix.m[0][2] = (2.0f * (xz + wy)) * scale;

  matrix.m[1][0] = (2.0f * (xy + wz)) * scale;
  matrix.m[1][1] = (1.0f - 2.0f * (xx + zz)) * scale;
  matrix.m[1][2] = (2.0f * (yz - wx)) * scale;

  matrix.m[2][0] = (2.0f * (xz - wy)) * scale;
  matrix.m[2][1] = (2.0f * (yz + wx)) * scale;
  matrix.m[2][2] = (1.0f - 2.0f * (xx + yy)) * scale;

  hasOrientation = true;
  return matrix;
}

float DistanceSquared(const SDK::Vector3 &a, const SDK::Vector3 &b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  const float dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

class GpuRenderer {
public:
  bool Initialize(ID3D11Device *device, ID3D11DeviceContext *context);
  void Shutdown();
  bool UploadMesh(SkinnedMesh &mesh);
  void QueueDraw(SkinnedMesh *mesh,
                 const std::vector<BoneMatrix3x4> &combinedMatrices,
                 const float (&fillColor)[4], const float (&wireColor)[4],
                 float alpha, int renderMode, int materialType);
  void Flush(const SDK::Matrix4x4 &viewMatrix, const SDK::Vector3 &cameraPos,
             int screenWidth, int screenHeight);

private:
  bool CreateShaders();
  bool CreateConstantBuffers();
  bool CreatePipelineStates();
  void UpdateViewProjection(const SDK::Matrix4x4 &viewMatrix, int screenWidth,
                            int screenHeight);
  void UpdateBoneMatrices(std::span<const BoneMatrix3x4> matrices);
  void UpdateMaterial(const float (&fillColor)[4], const float (&wireColor)[4],
                      float alpha, int renderMode, int materialType,
                      const SDK::Vector3 &cameraPos);

  ID3D11Device *m_device = nullptr;
  ID3D11DeviceContext *m_context = nullptr;
  ComPtr<ID3D11VertexShader> m_vertexShader;
  ComPtr<ID3D11PixelShader> m_pixelShaderFill;
  ComPtr<ID3D11PixelShader> m_pixelShaderWire;
  ComPtr<ID3D11InputLayout> m_inputLayout;
  ComPtr<ID3D11Buffer> m_cbViewProjection;
  ComPtr<ID3D11Buffer> m_cbBones;
  ComPtr<ID3D11Buffer> m_cbMaterial;
  ComPtr<ID3D11RasterizerState> m_rsFill;
  ComPtr<ID3D11RasterizerState> m_rsWireframe;
  ComPtr<ID3D11BlendState> m_blendState;
  ComPtr<ID3D11DepthStencilState> m_depthState;
  std::vector<DrawCommand> m_pendingDraws;
  bool m_initialized = false;
};

bool GpuRenderer::Initialize(ID3D11Device *device, ID3D11DeviceContext *context) {
  if (m_initialized) {
    return true;
  }
  if (!device || !context) {
    return false;
  }

  m_device = device;
  m_context = context;
  return CreateShaders() && CreateConstantBuffers() && CreatePipelineStates() &&
         (m_initialized = true);
}

void GpuRenderer::Shutdown() {
  m_initialized = false;
  m_vertexShader.Reset();
  m_pixelShaderFill.Reset();
  m_pixelShaderWire.Reset();
  m_inputLayout.Reset();
  m_cbViewProjection.Reset();
  m_cbBones.Reset();
  m_cbMaterial.Reset();
  m_rsFill.Reset();
  m_rsWireframe.Reset();
  m_blendState.Reset();
  m_depthState.Reset();
  m_pendingDraws.clear();
  m_device = nullptr;
  m_context = nullptr;
}

bool GpuRenderer::UploadMesh(SkinnedMesh &mesh) {
  return UploadMeshToGpu(m_device, mesh);
}

void GpuRenderer::QueueDraw(SkinnedMesh *mesh,
                            const std::vector<BoneMatrix3x4> &combinedMatrices,
                            const float (&fillColor)[4],
                            const float (&wireColor)[4], float alpha,
                            int renderMode, int materialType) {
  if (!m_initialized || !mesh || !mesh->vertexBuffer || !mesh->indexBuffer ||
      combinedMatrices.empty()) {
    return;
  }

  DrawCommand command{};
  command.mesh = mesh;
  command.combinedMatrices = combinedMatrices;
  std::copy(std::begin(fillColor), std::end(fillColor), command.fillColor);
  std::copy(std::begin(wireColor), std::end(wireColor), command.wireColor);
  command.alpha = alpha;
  command.renderMode = renderMode;
  command.materialType = materialType;
  m_pendingDraws.push_back(std::move(command));
}

bool GpuRenderer::CreateShaders() {
  ComPtr<ID3DBlob> vsBlob;
  ComPtr<ID3DBlob> psFillBlob;
  ComPtr<ID3DBlob> psWireBlob;
  ComPtr<ID3DBlob> errorBlob;

  auto compile = [&](const char *entryPoint, const char *target,
                     ComPtr<ID3DBlob> &blob) {
    return SUCCEEDED(D3DCompile(
        kShaderSource, std::strlen(kShaderSource), nullptr, nullptr, nullptr,
        entryPoint, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errorBlob.GetAddressOf()));
  };

  if (!compile("VS_Skinning", "vs_5_0", vsBlob) ||
      !compile("PS_Fill", "ps_5_0", psFillBlob) ||
      !compile("PS_Wire", "ps_5_0", psWireBlob)) {
    return false;
  }

  if (FAILED(m_device->CreateVertexShader(vsBlob->GetBufferPointer(),
                                          vsBlob->GetBufferSize(), nullptr,
                                          m_vertexShader.GetAddressOf())) ||
      FAILED(m_device->CreatePixelShader(psFillBlob->GetBufferPointer(),
                                         psFillBlob->GetBufferSize(), nullptr,
                                         m_pixelShaderFill.GetAddressOf())) ||
      FAILED(m_device->CreatePixelShader(psWireBlob->GetBufferPointer(),
                                         psWireBlob->GetBufferSize(), nullptr,
                                         m_pixelShaderWire.GetAddressOf()))) {
    return false;
  }

  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0,
       D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
       D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  return SUCCEEDED(m_device->CreateInputLayout(
      layout, static_cast<UINT>(std::size(layout)), vsBlob->GetBufferPointer(),
      vsBlob->GetBufferSize(), m_inputLayout.GetAddressOf()));
}

bool GpuRenderer::CreateConstantBuffers() {
  D3D11_BUFFER_DESC desc{};
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  desc.ByteWidth = sizeof(CBViewProjection);
  if (FAILED(m_device->CreateBuffer(&desc, nullptr,
                                    m_cbViewProjection.GetAddressOf()))) {
    return false;
  }

  desc.ByteWidth = sizeof(CBBones);
  if (FAILED(m_device->CreateBuffer(&desc, nullptr, m_cbBones.GetAddressOf()))) {
    return false;
  }

  desc.ByteWidth = sizeof(CBMaterial);
  return SUCCEEDED(
      m_device->CreateBuffer(&desc, nullptr, m_cbMaterial.GetAddressOf()));
}

bool GpuRenderer::CreatePipelineStates() {
  D3D11_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.FillMode = D3D11_FILL_SOLID;
  rasterizerDesc.CullMode = D3D11_CULL_NONE;
  rasterizerDesc.DepthClipEnable = FALSE;
  if (FAILED(m_device->CreateRasterizerState(&rasterizerDesc,
                                             m_rsFill.GetAddressOf()))) {
    return false;
  }

  rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
  if (FAILED(m_device->CreateRasterizerState(&rasterizerDesc,
                                             m_rsWireframe.GetAddressOf()))) {
    return false;
  }

  D3D11_BLEND_DESC blendDesc{};
  blendDesc.RenderTarget[0].BlendEnable = TRUE;
  blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D11_COLOR_WRITE_ENABLE_ALL;
  if (FAILED(m_device->CreateBlendState(&blendDesc,
                                        m_blendState.GetAddressOf()))) {
    return false;
  }

  D3D11_DEPTH_STENCIL_DESC depthDesc{};
  depthDesc.DepthEnable = FALSE;
  depthDesc.StencilEnable = FALSE;
  return SUCCEEDED(m_device->CreateDepthStencilState(
      &depthDesc, m_depthState.GetAddressOf()));
}

void GpuRenderer::UpdateViewProjection(const SDK::Matrix4x4 &viewMatrix,
                                       int screenWidth, int screenHeight) {
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (FAILED(m_context->Map(m_cbViewProjection.Get(), 0, D3D11_MAP_WRITE_DISCARD,
                            0, &mapped))) {
    return;
  }

  auto *data = static_cast<CBViewProjection *>(mapped.pData);
  std::memcpy(data->viewProjection, viewMatrix.m, sizeof(viewMatrix.m));
  data->screenWidth = static_cast<float>(screenWidth);
  data->screenHeight = static_cast<float>(screenHeight);
  data->pad[0] = 0.0f;
  data->pad[1] = 0.0f;
  m_context->Unmap(m_cbViewProjection.Get(), 0);
}

void GpuRenderer::UpdateBoneMatrices(std::span<const BoneMatrix3x4> matrices) {
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (FAILED(m_context->Map(m_cbBones.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                            &mapped))) {
    return;
  }

  auto *data = static_cast<CBBones *>(mapped.pData);
  std::memset(data, 0, sizeof(CBBones));
  const size_t count = std::min(matrices.size(), static_cast<size_t>(kMaxBones));
  for (size_t i = 0; i < count; ++i) {
    matrices[i].To4x4RowMajor(data->bones[i]);
  }
  m_context->Unmap(m_cbBones.Get(), 0);
}

void GpuRenderer::UpdateMaterial(const float (&fillColor)[4],
                                 const float (&wireColor)[4], float alpha,
                                 int renderMode, int materialType,
                                 const SDK::Vector3 &cameraPos) {
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (FAILED(m_context->Map(m_cbMaterial.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                            &mapped))) {
    return;
  }

  auto *data = static_cast<CBMaterial *>(mapped.pData);
  std::copy(std::begin(fillColor), std::end(fillColor), data->fillColor);
  std::copy(std::begin(wireColor), std::end(wireColor), data->wireColor);
  data->renderMode = renderMode;
  data->alpha = alpha;
  data->materialType = materialType;
  data->rimPower = 2.5f;
  data->cameraPos[0] = cameraPos.x;
  data->cameraPos[1] = cameraPos.y;
  data->cameraPos[2] = cameraPos.z;
  data->pad = 0.0f;
  m_context->Unmap(m_cbMaterial.Get(), 0);
}

void GpuRenderer::Flush(const SDK::Matrix4x4 &viewMatrix,
                        const SDK::Vector3 &cameraPos, int screenWidth,
                        int screenHeight) {
  if (!m_initialized || m_pendingDraws.empty() || !m_context) {
    return;
  }

  RendererStateGuard guard(m_context);

  D3D11_VIEWPORT viewport{};
  viewport.Width = static_cast<float>(screenWidth);
  viewport.Height = static_cast<float>(screenHeight);
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  m_context->RSSetViewports(1, &viewport);

  m_context->IASetInputLayout(m_inputLayout.Get());
  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

  float blendFactor[4] = {0, 0, 0, 0};
  m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFFu);
  m_context->OMSetDepthStencilState(m_depthState.Get(), 0);

  UpdateViewProjection(viewMatrix, screenWidth, screenHeight);

  ID3D11Buffer *vsBuffers[3] = {m_cbViewProjection.Get(), m_cbBones.Get(),
                                m_cbMaterial.Get()};
  ID3D11Buffer *psBuffers[3] = {nullptr, nullptr, m_cbMaterial.Get()};
  m_context->VSSetConstantBuffers(0, 3, vsBuffers);
  m_context->PSSetConstantBuffers(0, 3, psBuffers);

  for (const auto &command : m_pendingDraws) {
    UpdateBoneMatrices(command.combinedMatrices);

    UINT stride = sizeof(GpuVertex);
    UINT offset = 0;
    ID3D11Buffer *vb = command.mesh->vertexBuffer.Get();
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    m_context->IASetIndexBuffer(command.mesh->indexBuffer.Get(),
                                DXGI_FORMAT_R32_UINT, 0);

    if (command.renderMode == 0 || command.renderMode == 2) {
      UpdateMaterial(command.fillColor, command.wireColor, command.alpha, 0,
                     command.materialType, cameraPos);
      m_context->RSSetState(m_rsFill.Get());
      m_context->PSSetShader(m_pixelShaderFill.Get(), nullptr, 0);
      m_context->DrawIndexed(command.mesh->indexCount, 0, 0);
    }

    if (command.renderMode == 1 || command.renderMode == 2) {
      UpdateMaterial(command.fillColor, command.wireColor, command.alpha, 1,
                     command.materialType, cameraPos);
      m_context->RSSetState(m_rsWireframe.Get());
      m_context->PSSetShader(m_pixelShaderWire.Get(), nullptr, 0);
      m_context->DrawIndexed(command.mesh->indexCount, 0, 0);
    }
  }

  m_pendingDraws.clear();
}

} // namespace

class Chams::Impl {
public:
  bool Warmup() {
    if (ready) {
      return true;
    }
    if (failed) {
      return false;
    }

    auto *device = Render::Renderer::GetDevice();
    auto *context = Render::Renderer::GetContext();
    if (!gpuRenderer.Initialize(device, context)) {
      failed = true;
      Utils::Logger::Error("Chams warmup failed: renderer device/context unavailable");
      return false;
    }

    if (!LoadMesh("tm_phoenix.glb", tMesh) || !LoadMesh("ctm_sas.glb", ctMesh)) {
      gpuRenderer.Shutdown();
      failed = true;
      return false;
    }

    ready = true;
    failed = false;
    Utils::Logger::Info("Chams warmup completed");
    return true;
  }

  void Shutdown() {
    ready = false;
    failed = false;
    tMesh.Reset();
    ctMesh.Reset();
    previousBones.clear();
    gpuRenderer.Shutdown();
  }

  void ResetFrameState() { previousLocalPawn = 0; }
  bool IsReady() const { return ready; }

  void Render(const std::shared_ptr<const Core::GameSnapshot> &snapshot,
              const ChamsSnapshot &settings) {
    if (!ready || !snapshot) {
      return;
    }

    if (snapshot->localPawn != previousLocalPawn) {
      previousBones.clear();
      previousLocalPawn = snapshot->localPawn;
    }

    const auto offsets = SDK::Offsets::GetCopy();
    if (offsets.m_pGameSceneNode == 0 || offsets.m_boneArrayOffset == 0) {
      return;
    }

    const int screenWidth = Render::Overlay::GetGameWidth();
    const int screenHeight = Render::Overlay::GetGameHeight();
    if (screenWidth <= 0 || screenHeight <= 0) {
      return;
    }

    for (const auto &player : snapshot->players) {
      if (!player.IsValid() || !player.IsAlive()) {
        continue;
      }
      if (player.isTeammate && !settings.showTeammates) {
        continue;
      }

      SkinnedMesh *mesh = SelectMesh(player.team);
      if (!mesh || !mesh->indexBuffer || mesh->gltfToGameBoneMap.empty()) {
        continue;
      }

      auto gameBones =
          ReadGameBones(player.address, offsets,
                        static_cast<int>(mesh->gltfToGameBoneMap.size()));
      if (gameBones.empty()) {
        continue;
      }

      RefreshBoneState(player.address, gameBones);

      std::vector<BoneMatrix3x4> combined;
      combined.resize(std::min<size_t>(mesh->inverseBindMatrices.size(), kMaxBones));
      for (size_t i = 0; i < combined.size(); ++i) {
        const int mappedIndex =
            i < mesh->gltfToGameBoneMap.size() ? mesh->gltfToGameBoneMap[i] : -1;
        if (mappedIndex >= 0 &&
            mappedIndex < static_cast<int>(gameBones.size()) &&
            gameBones[static_cast<size_t>(mappedIndex)].matrix.IsValid()) {
          const LiveBonePose &pose = gameBones[static_cast<size_t>(mappedIndex)];
          if (pose.hasOrientation) {
            combined[i] = pose.matrix.Multiply(mesh->inverseBindMatrices[i]);
          } else {
            BoneMatrix3x4 currentApprox = mesh->bindPoseMatrices[i];
            currentApprox.m[0][3] = pose.matrix.m[0][3];
            currentApprox.m[1][3] = pose.matrix.m[1][3];
            currentApprox.m[2][3] = pose.matrix.m[2][3];
            combined[i] = currentApprox.Multiply(mesh->inverseBindMatrices[i]);
          }
        } else {
          combined[i] = BoneMatrix3x4::Identity();
        }
      }

      const bool hasAnyValidCombined = std::any_of(
          combined.begin(), combined.end(),
          [](const BoneMatrix3x4 &matrix) { return matrix.IsValid(); });
      if (!hasAnyValidCombined) {
        continue;
      }

      const float (&fillColor)[4] = player.isTeammate
                                        ? (settings.visibleCheck
                                               ? (player.isSpotted
                                                      ? settings.fillColorTeam
                                                      : settings.hiddenColorTeam)
                                               : settings.fillColorTeam)
                                        : (settings.visibleCheck
                                               ? (player.isSpotted ? settings.fillColor
                                                                   : settings.hiddenColor)
                                               : settings.fillColor);
      const int renderMode = settings.wireframe ? 2 : 0;
      gpuRenderer.QueueDraw(mesh, combined, fillColor, settings.wireColor,
                            settings.alpha, renderMode, settings.materialType);
    }

    gpuRenderer.Flush(snapshot->viewMatrix, snapshot->localEyePos, screenWidth,
                      screenHeight);
  }

private:
  bool LoadMesh(const std::string &filename, SkinnedMesh &mesh) {
    const auto path = FindAssetPath(filename);
    if (path.empty()) {
      Utils::Logger::Error("Chams mesh not found: %s", filename.c_str());
      return false;
    }

    mesh.Reset();
    if (!LoadGlbMesh(path, mesh)) {
      Utils::Logger::Error("Chams mesh load failed: %s", path.string().c_str());
      return false;
    }
    if (!gpuRenderer.UploadMesh(mesh)) {
      Utils::Logger::Error("Chams GPU upload failed: %s", path.string().c_str());
      return false;
    }
    return true;
  }

  SkinnedMesh *SelectMesh(int team) {
    if (team == 2) {
      return &tMesh;
    }
    if (team == 3) {
      return &ctMesh;
    }
    return nullptr;
  }

  std::vector<LiveBonePose> ReadGameBones(uintptr_t pawnAddress,
                                          const SDK::OffsetSet &offsets,
                                          int boneCount) {
    std::vector<LiveBonePose> bones;
    if (pawnAddress <= Core::Constants::MIN_VALID_ADDRESS || boneCount <= 0) {
      return bones;
    }

    const uintptr_t gameScene = Core::MemoryManager::Read<uintptr_t>(
        pawnAddress + offsets.m_pGameSceneNode);
    if (gameScene <= Core::Constants::MIN_VALID_ADDRESS) {
      return bones;
    }

    const uintptr_t boneArray = Core::MemoryManager::Read<uintptr_t>(
        gameScene + offsets.m_boneArrayOffset);
    if (boneArray <= Core::Constants::MIN_VALID_ADDRESS) {
      return bones;
    }

    std::vector<GameBoneTransform> rawBones(static_cast<size_t>(boneCount));
    if (!Core::MemoryManager::ReadRaw(boneArray, rawBones.data(),
                                      rawBones.size() * sizeof(GameBoneTransform))) {
      return {};
    }

    bones.resize(static_cast<size_t>(boneCount));
    for (int i = 0; i < boneCount; ++i) {
      const GameBoneTransform &transform = rawBones[static_cast<size_t>(i)];
      if (!IsFiniteVector(transform.position)) {
        return {};
      }
      bones[static_cast<size_t>(i)].matrix =
          MakeOrientedBoneMatrix(transform, bones[static_cast<size_t>(i)].hasOrientation);
    }
    return bones;
  }

  void RefreshBoneState(uintptr_t entityAddress,
                        std::vector<LiveBonePose> &bones) {
    auto &previous = previousBones[entityAddress];
    if (previous.size() != bones.size()) {
      previous = bones;
      return;
    }

    const float rootJump =
        DistanceSquared(previous.front().matrix.GetOrigin(), bones.front().matrix.GetOrigin());
    if (rootJump > kTeleportDistanceSq) {
      previous = bones;
      return;
    }

    // Keep the latest valid pose instead of interpolating bones frame-to-frame.
    // Interpolating skinning matrices causes visible trailing and "rubber" stretching
    // when animation state updates arrive unevenly.
    previous = bones;
  }

  GpuRenderer gpuRenderer;
  SkinnedMesh tMesh;
  SkinnedMesh ctMesh;
  std::unordered_map<uintptr_t, std::vector<LiveBonePose>> previousBones;
  uintptr_t previousLocalPawn = 0;
  bool ready = false;
  bool failed = false;
};

Chams::~Chams() {
  if (m_impl) {
    m_impl->Shutdown();
    delete m_impl;
    m_impl = nullptr;
  }
}

void Chams::Initialize() {
  if (!m_impl) {
    m_impl = new Impl();
  }
}

bool Chams::Warmup() {
  if (!m_impl) {
    m_impl = new Impl();
  }
  return m_impl->Warmup();
}

void Chams::OnDisable() {
  if (m_impl) {
    m_impl->ResetFrameState();
  }
}

void Chams::Update() {}

void Chams::Render(Render::DrawList &) {
  const ChamsSnapshot settings = SnapshotChams();
  if (!settings.enabled || !m_impl) {
    return;
  }

  const auto snapshot = Core::GameManager::GetSnapshot();
  if (!snapshot || snapshot->localPawn == 0) {
    return;
  }

  if (!m_impl->IsReady()) {
    return;
  }

  m_impl->Render(snapshot, settings);
}

void Chams::RenderUI() {}

} // namespace Features
