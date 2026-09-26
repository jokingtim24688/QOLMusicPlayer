#include "overlay/window.h"

#include "app_info.h"

static constexpr wchar_t kClassName[] = L"QOLOverlayWindow";
static constexpr DWORD kPassiveEx = WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;

bool OverlayWindow::Create(HINSTANCE inst, WNDPROC proc, void* userData) {
  WNDCLASSEXW wc{sizeof(wc)};
  wc.lpfnWndProc = proc;
  wc.hInstance = inst;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = kClassName;
  RegisterClassExW(&wc);

  // WS_EX_NOREDIRECTIONBITMAP: DWM takes our pixels straight from the DirectComposition swap chain.
  // WS_EX_LAYERED + WS_EX_TRANSPARENT: mouse input passes through to the game.
  // WS_EX_TOOLWINDOW: no taskbar button (the tray icon is the visible handle).
  DWORD ex = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | kPassiveEx;
  hwnd_ = CreateWindowExW(ex, kClassName, APP_NAME_W, WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, inst, userData);
  if (!hwnd_) return false;
  SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);
  rect_ = RECT{0, 0, 1, 1};
  return true;
}

void OverlayWindow::Destroy() {
  if (hwnd_) DestroyWindow(hwnd_);
  hwnd_ = nullptr;
}

void OverlayWindow::SetInteractive(bool interactive) {
  if (interactive_ == interactive) return;
  interactive_ = interactive;
  LONG_PTR ex = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
  ex = interactive ? (ex & ~static_cast<LONG_PTR>(kPassiveEx)) : (ex | kPassiveEx);
  SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, ex);
  SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

bool OverlayWindow::SetRect(const RECT& r) {
  bool sizeChanged = (r.right - r.left) != Width() || (r.bottom - r.top) != Height();
  if (!sizeChanged && r.left == rect_.left && r.top == rect_.top) return false;
  rect_ = r;
  SetWindowPos(hwnd_, HWND_TOPMOST, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE);
  return sizeChanged;
}

void OverlayWindow::Show(bool show) { ShowWindow(hwnd_, show ? SW_SHOWNOACTIVATE : SW_HIDE); }

void OverlayWindow::ReassertTopmost() {
  SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}
