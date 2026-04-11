#pragma once
#include <d3d11.h>
#include <windows.h>
#include <wrl/client.h>

namespace Render {
class Renderer {
public:
  static bool Init(HWND hwnd);
  static void Shutdown();
  static bool HandleResize(int width, int height);

  static void BeginFrame();
  static void EndFrame();

  static ID3D11Device *GetDevice();
  static ID3D11DeviceContext *GetContext();

  static void SetVSync(bool enabled);
  static bool IsVSyncEnabled();

private:
  static bool CreateRenderTarget();
  static void ReleaseRenderTarget();

  static Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
  static Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
  static Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
  static Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView;
  static bool s_vsyncEnabled;
};
} // namespace Render
