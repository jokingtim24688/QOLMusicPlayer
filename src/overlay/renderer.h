#pragma once
#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_2.h>

// D3D11 swap chain bound to the window through DirectComposition (per-pixel alpha, premultiplied).
class Renderer {
 public:
  bool Init(HWND hwnd, int width, int height);
  void Shutdown();
  void Resize(int width, int height);

  void BeginFrame();                // binds + clears the back buffer to fully transparent
  void EndFrame(bool vsync);        // Present; vsync while animating, immediate otherwise

  // BGRA image → shader resource view for ImGui (caller releases). nullptr on failure.
  ID3D11ShaderResourceView* CreateTexture(const void* bgra, int width, int height);

  ID3D11Device* Device() const { return device_; }
  ID3D11DeviceContext* Context() const { return context_; }

 private:
  void CreateTarget();
  void ReleaseTarget();

  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* context_ = nullptr;
  IDXGISwapChain1* swapChain_ = nullptr;
  ID3D11RenderTargetView* rtv_ = nullptr;
  IDCompositionDevice* dcomp_ = nullptr;
  IDCompositionTarget* dcompTarget_ = nullptr;
  IDCompositionVisual* dcompVisual_ = nullptr;
  int width_ = 0, height_ = 0;
};
