#pragma once
#include <windows.h>

// Borderless topmost window composed by DWM through DirectComposition.
// While passive it is click-through and never takes focus, so the game keeps all input.
class OverlayWindow {
 public:
  bool Create(HINSTANCE inst, WNDPROC proc, void* userData);
  void Destroy();

  // Passive: click-through, no activation. Interactive: normal hit-testing, can take focus.
  void SetInteractive(bool interactive);
  bool Interactive() const { return interactive_; }

  // Moves/resizes in screen coordinates. Returns true if the size changed.
  bool SetRect(const RECT& r);
  const RECT& Rect() const { return rect_; }
  int Width() const { return rect_.right - rect_.left; }
  int Height() const { return rect_.bottom - rect_.top; }

  void Show(bool show);
  void ReassertTopmost();
  HWND Hwnd() const { return hwnd_; }

 private:
  HWND hwnd_ = nullptr;
  RECT rect_{};
  bool interactive_ = false;
};
