#include "input/hotkeys.h"

#include "util/win.h"

bool Hotkeys::Register(HotkeyId id, const Hotkey& key) {
  UnregisterHotKey(hwnd_, id);
  return RegisterHotKey(hwnd_, id, key.mods | MOD_NOREPEAT, key.vk) != 0;
}

void Hotkeys::Unregister(HotkeyId id) { UnregisterHotKey(hwnd_, id); }

void Hotkeys::UnregisterAll() {
  Unregister(kHotkeyMenu);
  Unregister(kHotkeyToggle);
}

static std::string KeyName(unsigned vk) {
  switch (vk) {
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "Page Up";
    case VK_NEXT: return "Page Down";
    case VK_PAUSE: return "Pause";
    case VK_SCROLL: return "Scroll Lock";
    case VK_SNAPSHOT: return "Print Screen";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_XBUTTON1: return "Mouse 4";
    case VK_XBUTTON2: return "Mouse 5";
    default: break;
  }
  if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
  if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return "Num " + std::to_string(vk - VK_NUMPAD0);
  UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
  wchar_t buf[64]{};
  if (scan && GetKeyNameTextW(static_cast<LONG>(scan << 16), buf, 64) > 0) return WideToUtf8(buf);
  return "Key " + std::to_string(vk);
}

std::string HotkeyName(const Hotkey& key) {
  std::string s;
  if (key.mods & MOD_CONTROL) s += "Ctrl + ";
  if (key.mods & MOD_ALT) s += "Alt + ";
  if (key.mods & MOD_SHIFT) s += "Shift + ";
  if (key.mods & MOD_WIN) s += "Win + ";
  return s + KeyName(key.vk);
}
