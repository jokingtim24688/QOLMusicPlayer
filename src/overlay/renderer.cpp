#include "overlay/renderer.h"

#include <algorithm>

template <class T>
static void SafeRelease(T*& p) {
  if (p) p->Release();
  p = nullptr;
}

bool Renderer::Init(HWND hwnd, int width, int height) {
  width_ = std::max(width, 1);
  height_ = std::max(height, 1);

  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, 2, D3D11_SDK_VERSION,
                                 &device_, nullptr, &context_);
  if (FAILED(hr))  // no usable GPU driver: fall back to WARP so the overlay still works
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, levels, 2, D3D11_SDK_VERSION, &device_,
                           nullptr, &context_);
  if (FAILED(hr)) return false;

  IDXGIDevice* dxgiDevice = nullptr;
  IDXGIAdapter* adapter = nullptr;
  IDXGIFactory2* factory = nullptr;
  device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
  // The overlay is a background helper: yield GPU time to the game.
  dxgiDevice->SetGPUThreadPriority(-7);
  dxgiDevice->GetAdapter(&adapter);
  adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory));

  DXGI_SWAP_CHAIN_DESC1 desc{};
  desc.Width = static_cast<UINT>(width_);
  desc.Height = static_cast<UINT>(height_);
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = 2;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
  hr = factory->CreateSwapChainForComposition(device_, &desc, nullptr, &swapChain_);

  if (SUCCEEDED(hr)) hr = DCompositionCreateDevice(dxgiDevice, __uuidof(IDCompositionDevice),
                                                   reinterpret_cast<void**>(&dcomp_));
  if (SUCCEEDED(hr)) hr = dcomp_->CreateTargetForHwnd(hwnd, TRUE, &dcompTarget_);
  if (SUCCEEDED(hr)) hr = dcomp_->CreateVisual(&dcompVisual_);
  if (SUCCEEDED(hr)) hr = dcompVisual_->SetContent(swapChain_);
  if (SUCCEEDED(hr)) hr = dcompTarget_->SetRoot(dcompVisual_);
  if (SUCCEEDED(hr)) hr = dcomp_->Commit();

  SafeRelease(factory);
  SafeRelease(adapter);
  SafeRelease(dxgiDevice);
  if (FAILED(hr)) return false;
  CreateTarget();
  return true;
}

void Renderer::Shutdown() {
  ReleaseTarget();
  SafeRelease(dcompVisual_);
  SafeRelease(dcompTarget_);
  SafeRelease(dcomp_);
  SafeRelease(swapChain_);
  SafeRelease(context_);
  SafeRelease(device_);
}

void Renderer::CreateTarget() {
  ID3D11Texture2D* back = nullptr;
  swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back));
  if (back) device_->CreateRenderTargetView(back, nullptr, &rtv_);
  SafeRelease(back);
}

void Renderer::ReleaseTarget() { SafeRelease(rtv_); }

void Renderer::Resize(int width, int height) {
  width = std::max(width, 1);
  height = std::max(height, 1);
  if (width == width_ && height == height_) return;
  width_ = width;
  height_ = height;
  ReleaseTarget();
  context_->OMSetRenderTargets(0, nullptr, nullptr);
  context_->Flush();
  swapChain_->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0);
  CreateTarget();
}

void Renderer::BeginFrame() {
  const float clear[4] = {0, 0, 0, 0};
  context_->OMSetRenderTargets(1, &rtv_, nullptr);
  context_->ClearRenderTargetView(rtv_, clear);
}

void Renderer::EndFrame(bool vsync) { swapChain_->Present(vsync ? 1 : 0, 0); }
