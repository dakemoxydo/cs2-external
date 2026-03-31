#pragma once
#include <d3d11.h>
#include <windows.h>


namespace Render {
class Renderer {
public:
  static bool Init(HWND hwnd);
  static void Shutdown();

  static void BeginFrame();
  static void EndFrame();

  static ID3D11Device *GetDevice();
  static ID3D11DeviceContext *GetContext();

  static void SetVSync(bool enabled);
  static bool IsVSyncEnabled();

private:
  static ID3D11Device *pDevice;
  static ID3D11DeviceContext *pContext;
  static IDXGISwapChain *pSwapChain;
  static ID3D11RenderTargetView *pRenderTargetView;
  static bool s_vsyncEnabled;
};
} // namespace Render
