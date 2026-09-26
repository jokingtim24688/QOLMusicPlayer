#pragma once
#include <string>
#include <vector>

#include "config/segments.h"

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

  // Bar contents, in order (edited in the menu's Bar tab)
  std::string logoText = "QOL";
  std::vector<Seg> segments{Seg::Fps, Seg::Ping, Seg::Date, Seg::Time, Seg::User};
  bool showLow = true;  // 1% low next to FPS
  int dateFormat = 0;   // index into kDateFormats (src/providers/clock.h)
  bool clock24h = false;
  bool clockSeconds = false;
  std::string username;  // empty = Windows account name

  // Music player (Spotify via Windows media controls)
  bool showPlayer = true;
  bool playerSpotifyOnly = true;  // false = any app that reports media (browser, YouTube Music, ...)
  bool playerHideWhenIdle = true;  // hide when Spotify (or the media app) isn't open
  float playerX = 0.0f, playerY = 1.0f;  // position as a fraction of the free space on the monitor

  // Game selection: empty = automatic (foreground window)
  std::string pinnedExe;

  // Keybinds
  Hotkey menuKey{0x2E /*VK_DELETE*/, 0};
  Hotkey toggleKey{0x23 /*VK_END*/, 0};
  bool overlayVisible = true;

  // Updates
  bool autoUpdate = true;
  std::string lastVersion;  // version that last ran, to say "updated" once

  static constexpr float kMinOpacity = 0.70f;

  void Load();
  void Save() const;
};
