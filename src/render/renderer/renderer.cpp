#include "renderer.h"
#include <iostream>

#pragma comment(lib, "d3d11.lib")

namespace Render {
ID3D11Device *Renderer::pDevice = nullptr;
ID3D11DeviceContext *Renderer::pContext = nullptr;
IDXGISwapChain *Renderer::pSwapChain = nullptr;
ID3D11RenderTargetView *Renderer::pRenderTargetView = nullptr;

bool Renderer::Init(HWND hwnd) {
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
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
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_0,
  };

  if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                    createDeviceFlags, featureLevelArray, 2,
                                    D3D11_SDK_VERSION, &sd, &pSwapChain,
                                    &pDevice, &featureLevel, &pContext) != S_OK)
    return false;

  ID3D11Texture2D *pBackBuffer;
  pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
  pBackBuffer->Release();

  return true;
}

void Renderer::Shutdown() {
  if (pRenderTargetView) {
    pRenderTargetView->Release();
    pRenderTargetView = nullptr;
  }
  if (pSwapChain) {
    pSwapChain->Release();
    pSwapChain = nullptr;
  }
  if (pContext) {
    pContext->Release();
    pContext = nullptr;
  }
  if (pDevice) {
    pDevice->Release();
    pDevice = nullptr;
  }
}

void Renderer::BeginFrame() {
  const float clear_color_with_alpha[4] = {0.f, 0.f, 0.f, 0.f};
  pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
  pContext->ClearRenderTargetView(pRenderTargetView, clear_color_with_alpha);
}

void Renderer::EndFrame() { pSwapChain->Present(1, 0); }

ID3D11Device *Renderer::GetDevice() { return pDevice; }

ID3D11DeviceContext *Renderer::GetContext() { return pContext; }
} // namespace Render
