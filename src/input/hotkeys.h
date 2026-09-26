#pragma once
#include <windows.h>

#include <string>

#include "config/config.h"

enum HotkeyId : int { kHotkeyMenu = 1, kHotkeyToggle = 2 };

// System-wide hotkeys via RegisterHotKey: no keyboard hooks, nothing an anti-cheat has to look at.
class Hotkeys {
 public:
  void Init(HWND hwnd) { hwnd_ = hwnd; }
  // Returns false if another program already owns the combination.
  bool Register(HotkeyId id, const Hotkey& key);
  void Unregister(HotkeyId id);
  void UnregisterAll();

 private:
  HWND hwnd_ = nullptr;
};

std::string HotkeyName(const Hotkey& key);
