#pragma once
#include <windows.h>

#include <string>

#include "config/config.h"
#include "input/hotkeys.h"
#include "overlay/renderer.h"
#include "overlay/window.h"
#include "providers/etw_session.h"
#include "providers/fps.h"
#include "providers/game_select.h"
#include "providers/ping.h"
#include "ui/fonts.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/watermark.h"

class App {
 public:
  int Run(HINSTANCE inst);

 private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
  LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

  void Tick(bool force = false);  // once per second: game, FPS, ping, clock
  void Render();  // one frame; only called when something changed or is animating
  DWORD MsUntilNextTick() const;

  void OpenMenu();
  void CloseMenu();
  void ToggleOverlay();
  void RegisterHotkeys();
  void StartCapture(Capture which);
  void FinishCapture(unsigned vk);
  void SaveIfDirty();

  void TrayAdd();
  void TrayRemove();
  void TrayMenu();

  Config cfg_;
  OverlayWindow window_;
  Renderer renderer_;
  Hotkeys hotkeys_;
  FpsProvider fps_;
  PingProvider ping_;
  EtwSession etw_;
  GameSelector games_;
  Fonts fonts_;
  ThemeAnimator theme_;
  Menu menu_;

  WatermarkData data_;
  std::string lastSignature_;
  std::string windowsUser_;
  std::string appliedTheme_;

  bool running_ = true;
  bool needRedraw_ = true;
  bool animating_ = false;
  int framesPending_ = 2;
  bool shown_ = false;
  bool cfgDirty_ = false;
  bool menuOpen_ = false;
  float menuT_ = 0.0f;
  float scale_ = 1.0f;
  HWND prevForeground_ = nullptr;
  Capture capture_ = Capture::None;
  std::string keyError_;
  int tickCount_ = 0;
  int lastSecond_ = -1;
  UINT taskbarCreatedMsg_ = 0;
};
