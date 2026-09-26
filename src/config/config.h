#pragma once
#include <string>

enum class Corner { TopLeft, TopRight, BottomLeft, BottomRight };

struct Hotkey {
  unsigned vk = 0;
  unsigned mods = 0;  // MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN
};

struct Config {
  // Appearance
  std::string theme = "Catppuccin Mocha";
  std::string font = "Geist";
  float fontSize = 15.0f;
  float opacity = 0.86f;  // watermark fill opacity; UI clamps to kMinOpacity for readability
  Corner corner = Corner::TopRight;

  // Watermark segments
  std::string logoText = "QOL";
  bool showFps = true;
  bool showLow = true;
  bool showPing = true;
  bool showTime = true;
  bool showUser = true;
  bool clock24h = false;
  bool clockSeconds = false;
  std::string username;  // empty = Windows account name

  // Game selection: empty = automatic (foreground window)
  std::string pinnedExe;

  // Keybinds
  Hotkey menuKey{0x2D /*VK_INSERT*/, 0};
  Hotkey toggleKey{0x23 /*VK_END*/, 0};
  bool overlayVisible = true;

  static constexpr float kMinOpacity = 0.70f;

  void Load();
  void Save() const;
};
