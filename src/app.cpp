#include "app.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <shellapi.h>

#include <algorithm>
#include <cmath>

#include "app_info.h"
#include "providers/clock.h"
#include "ui/anim.h"
#include "ui/widgets.h"
#include "util/win.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static constexpr UINT kTrayMsg = WM_APP + 1;
static constexpr float kMenuFontSize = 15.0f;

int App::Run(HINSTANCE inst) {
  cfg_.Load();

  wchar_t user[256];
  DWORD userLen = 256;
  if (GetUserNameW(user, &userLen)) windowsUser_ = WideToUtf8(user);

  if (!window_.Create(inst, &App::WndProc, this)) return 1;
  if (!renderer_.Init(window_.Hwnd(), 1, 1)) {
    MessageBoxW(nullptr, L"Couldn't initialise Direct3D 11 / DirectComposition.", APP_NAME_W, MB_ICONERROR);
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;
  io.ConfigInputTextCursorBlink = false;  // a blinking caret would force continuous redraws
  ImGui_ImplWin32_Init(window_.Hwnd());
  ImGui_ImplDX11_Init(renderer_.Device(), renderer_.Context());
  fonts_.Load();
  theme_.Set(FindTheme(cfg_.theme), true);
  appliedTheme_ = cfg_.theme;

  hotkeys_.Init(window_.Hwnd());
  RegisterHotkeys();
  taskbarCreatedMsg_ = RegisterWindowMessageW(L"TaskbarCreated");
  TrayAdd();

  etw_.Start(&fps_, &ping_);
  ping_.Start();
  Tick(true);

  while (running_) {
    const bool busy = animating_ || framesPending_ > 0;
    if (!busy) MsgWaitForMultipleObjectsEx(0, nullptr, MsUntilNextTick(), QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) running_ = false;
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    if (!running_) break;
    Tick();  // no-op unless the second has changed
    if (needRedraw_ || busy) {
      needRedraw_ = false;
      if (framesPending_ > 0) --framesPending_;
      Render();
    }
  }

  ping_.Stop();
  etw_.Stop();
  hotkeys_.UnregisterAll();
  TrayRemove();
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
  renderer_.Shutdown();
  window_.Destroy();
  cfgDirty_ = true;
  SaveIfDirty();
  return 0;
}

DWORD App::MsUntilNextTick() const {
  SYSTEMTIME t;
  GetLocalTime(&t);
  return 1000u - t.wMilliseconds + 2u;  // wake just after the second flips, so the clock is exact
}

void App::Tick(bool force) {
  SYSTEMTIME t;
  GetLocalTime(&t);
  if (!force && t.wSecond == lastSecond_) return;
  lastSecond_ = t.wSecond;
  ++tickCount_;

  games_.Update(fps_, cfg_.pinnedExe, window_.Hwnd());
  const DWORD pid = games_.Pid();
  const bool wantPing = cfg_.showPing && pid != 0;
  etw_.EnableNetwork(wantPing);
  ping_.SetTarget(wantPing ? pid : 0);

  WatermarkData d;
  d.logo = cfg_.logoText;
  d.showFps = cfg_.showFps;
  d.showLow = cfg_.showLow;
  d.showPing = cfg_.showPing;
  d.showTime = cfg_.showTime;
  d.showUser = cfg_.showUser;
  d.fps = pid ? fps_.Sample(pid) : FpsSample{};
  const EtwStatus etw = etw_.Status();
  if (etw == EtwStatus::NeedsAdmin) d.fpsProblem = "needs admin";
  else if (etw != EtwStatus::Running) d.fpsProblem = "unavailable";
  else if (!pid) d.fpsProblem = "no game";
  else if (!d.fps.valid) d.fpsProblem = "paused";
  d.ping = cfg_.showPing ? ping_.Sample() : PingSample{};
  if (cfg_.showPing && !pid) d.ping.state = PingState::NoGame;
  d.time = ClockText(cfg_.clock24h, cfg_.clockSeconds);
  d.user = cfg_.username.empty() ? windowsUser_ : cfg_.username;

  HMONITOR mon = games_.Monitor() ? games_.Monitor() : MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFOEXW mi{};
  mi.cbSize = sizeof(mi);
  DEVMODEW dm{};
  dm.dmSize = sizeof(dm);
  if (GetMonitorInfoW(mon, &mi) && EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm))
    d.refreshHz = static_cast<float>(dm.dmDisplayFrequency);
  data_ = std::move(d);

  const std::string sig = data_.Signature();
  if (sig != lastSignature_) {
    lastSignature_ = sig;
    needRedraw_ = true;
  }
  // Borderless games sometimes push themselves above other topmost windows; take the spot back.
  if (tickCount_ % 2 == 0 && shown_ && !menuOpen_) window_.ReassertTopmost();
}

void App::Render() {
  const bool menuVisible = menuOpen_ || menuT_ > 0.0f;
  if (!menuVisible && !cfg_.overlayVisible) {
    if (shown_) window_.Show(false);
    shown_ = false;
    animating_ = false;
    return;
  }

  HMONITOR mon = games_.Monitor() ? games_.Monitor() : MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO mi{sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  const RECT m = mi.rcMonitor;
  scale_ = ImGui_ImplWin32_GetDpiScaleForMonitor(mon);

  const FontFace& face = fonts_.Find(cfg_.font);
  const Theme& theme = theme_.Current();
  const WatermarkStyle ws{&theme, face.medium, face.bold, std::round(cfg_.fontSize * scale_),
                          std::max(cfg_.opacity, Config::kMinOpacity)};

  auto newFrame = [] {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
  };
  newFrame();

  // Work out where the watermark goes on the game's monitor.
  const ImVec2 wm = Watermark(nullptr, ImVec2(0, 0), data_, ws, false);
  const float margin = std::round(14.0f * scale_), pad = std::round(12.0f * scale_);
  const bool right = cfg_.corner == Corner::TopRight || cfg_.corner == Corner::BottomRight;
  const bool bottom = cfg_.corner == Corner::BottomLeft || cfg_.corner == Corner::BottomRight;
  const float wx = right ? static_cast<float>(m.right) - margin - wm.x : static_cast<float>(m.left) + margin;
  const float wy = bottom ? static_cast<float>(m.bottom) - margin - wm.y : static_cast<float>(m.top) + margin;

  // Passive mode: the window hugs the watermark, so DWM only composites a small rectangle over the game.
  // Width is rounded up to 32 px so small number changes don't resize it every second.
  RECT want = m;
  if (!menuVisible) {
    const LONG w = (static_cast<LONG>(wm.x + pad * 2) + 31) / 32 * 32;
    const LONG h = static_cast<LONG>(wm.y + pad * 2);
    want.left = right ? static_cast<LONG>(wx + wm.x + pad) - w : static_cast<LONG>(wx - pad);
    want.top = bottom ? static_cast<LONG>(wy + wm.y + pad) - h : static_cast<LONG>(wy - pad);
    want.right = want.left + w;
    want.bottom = want.top + h;
  }
  if (window_.SetRect(want)) {
    renderer_.Resize(window_.Width(), window_.Height());
    ImGui::EndFrame();
    newFrame();
  }
  if (!shown_) {
    window_.Show(true);
    shown_ = true;
  }

  ImGuiIO& io = ImGui::GetIO();
  anim::BeginFrame(io.DeltaTime);
  const bool themeMoving = theme_.Update(std::min(io.DeltaTime, 1.0f / 30.0f));
  menuT_ = anim::Spring(ImGui::GetID("##menu-open"), menuOpen_ ? 1.0f : 0.0f, 18.0f);
  ApplyImGuiStyle(theme, scale_);
  ui::g = ui::Context{&theme, scale_, face.medium, face.bold, kMenuFontSize};
  ImGui::PushFont(face.medium, kMenuFontSize * scale_);

  ImDrawList* bg = ImGui::GetBackgroundDrawList();
  const ImVec2 origin(static_cast<float>(window_.Rect().left), static_cast<float>(window_.Rect().top));
  if (menuVisible) bg->AddRectFilled(ImVec2(0, 0), io.DisplaySize, Col(theme.crust, 0.45f * menuT_));
  if (cfg_.overlayVisible) Watermark(bg, ImVec2(std::round(wx), std::round(wy)) - origin, data_, ws, true);

  if (menuVisible) {
    MenuContext ctx{&cfg_, &fonts_, &games_, data_.ping, data_.fps, etw_.Status(), capture_, keyError_};
    MenuEvents ev;
    menu_.Draw(ctx, menuT_, ev);
    if (ev.startCapture != Capture::None) StartCapture(ev.startCapture);
    else if (ev.cancelCapture) FinishCapture(VK_ESCAPE);
    if (ev.changed) {
      cfgDirty_ = true;
      Tick(true);  // rebuild watermark data with the new settings right away
    }
    // Click outside the menu closes it, like any popover.
    if (menuOpen_ && capture_ == Capture::None && ImGui::IsMouseClicked(0) &&
        !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
      CloseMenu();
  }
  if (cfg_.theme != appliedTheme_) {
    appliedTheme_ = cfg_.theme;
    theme_.Set(FindTheme(cfg_.theme), false);
  }

  ImGui::PopFont();
  ImGui::Render();
  renderer_.BeginFrame();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  animating_ = anim::Active() || themeMoving || (menuVisible && menuT_ != (menuOpen_ ? 1.0f : 0.0f));
  // vsync while animating paces the loop to the display; idle frames present immediately and we go back to sleep.
  renderer_.EndFrame(animating_ || framesPending_ > 0);
  if (!menuOpen_ && menuT_ == 0.0f && menuVisible) needRedraw_ = true;  // shrink back to passive mode
}

void App::OpenMenu() {
  if (menuOpen_) return;
  prevForeground_ = GetForegroundWindow();
  menuOpen_ = true;
  window_.SetInteractive(true);
  if (!shown_) {
    window_.Show(true);
    shown_ = true;
  }
  SetForegroundWindow(window_.Hwnd());
  needRedraw_ = true;
  framesPending_ = 3;
}

void App::CloseMenu() {
  if (!menuOpen_) return;
  if (capture_ != Capture::None) FinishCapture(VK_ESCAPE);
  menuOpen_ = false;
  window_.SetInteractive(false);
  // Hand focus straight back to the game; the fade-out plays while you're already playing.
  if (prevForeground_ && IsWindow(prevForeground_)) SetForegroundWindow(prevForeground_);
  prevForeground_ = nullptr;
  SaveIfDirty();
  needRedraw_ = true;
  framesPending_ = 2;
}

void App::ToggleOverlay() {
  cfg_.overlayVisible = !cfg_.overlayVisible;
  cfgDirty_ = true;
  SaveIfDirty();
  needRedraw_ = true;
}

void App::RegisterHotkeys() {
  std::string err;
  if (!hotkeys_.Register(kHotkeyMenu, cfg_.menuKey))
    err = HotkeyName(cfg_.menuKey) + " is already used by another program. Pick another key for the menu.";
  if (!hotkeys_.Register(kHotkeyToggle, cfg_.toggleKey)) {
    if (!err.empty()) err += " ";
    err += HotkeyName(cfg_.toggleKey) + " is already used by another program. Pick another key for show/hide.";
  }
  keyError_ = err;
}

void App::StartCapture(Capture which) {
  hotkeys_.UnregisterAll();  // so pressing the current bind reaches us as a normal key
  capture_ = which;
  keyError_.clear();
}

void App::FinishCapture(unsigned vk) {
  if (capture_ == Capture::None) return;
  switch (vk) {  // wait for a real key when only a modifier is down
    case VK_SHIFT: case VK_CONTROL: case VK_MENU: case VK_LWIN: case VK_RWIN:
    case VK_LSHIFT: case VK_RSHIFT: case VK_LCONTROL: case VK_RCONTROL: case VK_LMENU: case VK_RMENU:
      return;
    default: break;
  }
  const Capture which = capture_;
  capture_ = Capture::None;
  if (vk != VK_ESCAPE) {
    Hotkey k{vk, 0};
    if (GetKeyState(VK_CONTROL) < 0) k.mods |= MOD_CONTROL;
    if (GetKeyState(VK_MENU) < 0) k.mods |= MOD_ALT;
    if (GetKeyState(VK_SHIFT) < 0) k.mods |= MOD_SHIFT;
    Hotkey& slot = which == Capture::MenuKey ? cfg_.menuKey : cfg_.toggleKey;
    const Hotkey& other = which == Capture::MenuKey ? cfg_.toggleKey : cfg_.menuKey;
    if (k.vk == other.vk && k.mods == other.mods) {
      RegisterHotkeys();
      keyError_ = HotkeyName(k) + " is already your other bind.";
      return;
    }
    const Hotkey old = slot;
    slot = k;
    if (!hotkeys_.Register(which == Capture::MenuKey ? kHotkeyMenu : kHotkeyToggle, slot)) {
      slot = old;
      RegisterHotkeys();
      keyError_ = HotkeyName(k) + " is already used by another program. Try a different key.";
      return;
    }
    cfgDirty_ = true;
  }
  RegisterHotkeys();
  needRedraw_ = true;
}

void App::SaveIfDirty() {
  if (!cfgDirty_) return;
  cfg_.Save();
  cfgDirty_ = false;
}

void App::TrayAdd() {
  NOTIFYICONDATAW nid{};
  nid.cbSize = sizeof(nid);
  nid.hWnd = window_.Hwnd();
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = kTrayMsg;
  nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
  wcscpy_s(nid.szTip, APP_NAME_W);
  Shell_NotifyIconW(NIM_ADD, &nid);
}

void App::TrayRemove() {
  NOTIFYICONDATAW nid{};
  nid.cbSize = sizeof(nid);
  nid.hWnd = window_.Hwnd();
  nid.uID = 1;
  Shell_NotifyIconW(NIM_DELETE, &nid);
}

void App::TrayMenu() {
  enum { kOpen = 1, kToggle, kExit };
  HMENU menu = CreatePopupMenu();
  const std::wstring open = L"Open menu\t" + Utf8ToWide(HotkeyName(cfg_.menuKey));
  const std::wstring toggle =
      std::wstring(cfg_.overlayVisible ? L"Hide overlay\t" : L"Show overlay\t") + Utf8ToWide(HotkeyName(cfg_.toggleKey));
  AppendMenuW(menu, MF_STRING, kOpen, open.c_str());
  AppendMenuW(menu, MF_STRING, kToggle, toggle.c_str());
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(menu, MF_STRING, kExit, L"Exit");
  POINT pt;
  GetCursorPos(&pt);
  SetForegroundWindow(window_.Hwnd());  // required for the menu to close when clicking elsewhere
  const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, window_.Hwnd(),
                                 nullptr);
  DestroyMenu(menu);
  if (cmd == kOpen) OpenMenu();
  else if (cmd == kToggle) ToggleOverlay();
  else if (cmd == kExit) PostQuitMessage(0);
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_NCCREATE) {
    auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
  }
  auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  return app ? app->HandleMessage(hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT App::HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (menuOpen_ && ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || (msg >= WM_KEYFIRST && msg <= WM_KEYLAST))) {
    needRedraw_ = true;
    framesPending_ = std::max(framesPending_, 2);  // ImGui settles hover/active state over a couple of frames
  }
  if (capture_ != Capture::None && (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)) {
    FinishCapture(static_cast<unsigned>(wp));
    return 0;
  }
  if (menuOpen_ && msg == WM_KEYDOWN && wp == VK_ESCAPE) {
    CloseMenu();
    return 0;
  }
  if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return 1;

  if (msg == taskbarCreatedMsg_ && msg != 0) {  // Explorer restarted: the tray icon is gone, add it back
    TrayAdd();
    return 0;
  }
  switch (msg) {
    case WM_HOTKEY:
      if (wp == kHotkeyMenu) menuOpen_ ? CloseMenu() : OpenMenu();
      else if (wp == kHotkeyToggle) ToggleOverlay();
      return 0;
    case WM_MOUSEACTIVATE:
      if (!window_.Interactive()) return MA_NOACTIVATE;
      break;
    case kTrayMsg:
      if (LOWORD(lp) == WM_LBUTTONUP) OpenMenu();
      else if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_CONTEXTMENU) TrayMenu();
      return 0;
    case WM_DISPLAYCHANGE:
    case WM_DPICHANGED:
      needRedraw_ = true;
      return 0;
    case WM_CLOSE:
      PostQuitMessage(0);
      return 0;
    default: break;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}
