#include "renderer.h"
#include <dxgi.h>

namespace Render {

using Microsoft::WRL::ComPtr;

ComPtr<ID3D11Device> Renderer::pDevice;
ComPtr<ID3D11DeviceContext> Renderer::pContext;
ComPtr<IDXGISwapChain> Renderer::pSwapChain;
ComPtr<ID3D11RenderTargetView> Renderer::pRenderTargetView;
bool Renderer::s_vsyncEnabled = false;

bool Renderer::CreateRenderTarget() {
  if (!pSwapChain || !pDevice) {
    return false;
  }

  ComPtr<ID3D11Texture2D> backBuffer;
  if (FAILED(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
    return false;
  }

  return SUCCEEDED(
      pDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &pRenderTargetView));
}

void Renderer::ReleaseRenderTarget() {
  pRenderTargetView.Reset();
}

bool Renderer::Init(HWND hwnd) {
  Shutdown();

  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 1;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hwnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT createDeviceFlags = 0;
  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_0,
  };

  if (FAILED(D3D11CreateDeviceAndSwapChain(
          nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
          featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice,
          &featureLevel, &pContext))) {
    Shutdown();
    return false;
  }

  if (!CreateRenderTarget()) {
    Shutdown();
    return false;
  }

  return true;
}

bool Renderer::HandleResize(int width, int height) {
  if (!pSwapChain || !pContext || width <= 0 || height <= 0) {
    return false;
  }

  pContext->OMSetRenderTargets(0, nullptr, nullptr);
  ReleaseRenderTarget();

  if (FAILED(pSwapChain->ResizeBuffers(0, static_cast<UINT>(width),
                                       static_cast<UINT>(height),
                                       DXGI_FORMAT_UNKNOWN, 0))) {
    CreateRenderTarget();
    return false;
  }

  return CreateRenderTarget();
}

void Renderer::Shutdown() {
  ReleaseRenderTarget();

  if (pContext) {
    pContext->ClearState();
    pContext->Flush();
  }

  pSwapChain.Reset();
  pContext.Reset();
  pDevice.Reset();
}

void Renderer::BeginFrame() {
  if (!pContext || !pRenderTargetView) {
    return;
  }

  const float clearColorWithAlpha[4] = {0.f, 0.f, 0.f, 0.f};
  ID3D11RenderTargetView *renderTarget = pRenderTargetView.Get();
  pContext->OMSetRenderTargets(1, &renderTarget, nullptr);
  pContext->ClearRenderTargetView(renderTarget, clearColorWithAlpha);
}

void Renderer::EndFrame() {
  if (!pSwapChain) {
    return;
  }

  pSwapChain->Present(s_vsyncEnabled ? 1 : 0, 0);
}

ID3D11Device *Renderer::GetDevice() {
  return pDevice.Get();
}

ID3D11DeviceContext *Renderer::GetContext() {
  return pContext.Get();
}

void Renderer::SetVSync(bool enabled) {
  s_vsyncEnabled = enabled;
}

bool Renderer::IsVSyncEnabled() {
  return s_vsyncEnabled;
}

} // namespace Render
