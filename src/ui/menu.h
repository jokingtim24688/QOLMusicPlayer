#pragma once
#include <imgui.h>

#include <string>

#include "config/config.h"
#include "providers/etw_session.h"
#include "providers/game_select.h"
#include "providers/ping.h"
#include "ui/fonts.h"
#include "ui/player.h"
#include "update/updater.h"

enum class Capture { None, MenuKey, ToggleKey };

struct MenuContext {
  Config* cfg;
  Fonts* fonts;
  const GameSelector* games;
  PingSample ping;
  FpsSample fps;
  EtwStatus etw;
  Capture capture;
  std::string keyError;
  const PlayerData* player;
  UpdateStatus update;
};

struct MenuEvents {
  bool changed = false;               // config edited (save + redraw)
  Capture startCapture = Capture::None;
  bool cancelCapture = false;
  bool checkUpdates = false;
  bool installUpdate = false;
};

class Menu {
 public:
  // openT: 0..1 open animation progress (drives fade + scale).
  void Draw(MenuContext& ctx, float openT, MenuEvents& ev);
  void SelectTab(int tab) { tab_ = tab; }
  int Tab() const { return tab_; }

 private:
  void TabOverlay(MenuContext& ctx, MenuEvents& ev);
  void TabBar(MenuContext& ctx, MenuEvents& ev);
  void TabMusic(MenuContext& ctx, MenuEvents& ev);
  void TabGame(MenuContext& ctx, MenuEvents& ev);
  void TabTheme(MenuContext& ctx, MenuEvents& ev);
  void TabFont(MenuContext& ctx, MenuEvents& ev);
  void TabKeybinds(MenuContext& ctx, MenuEvents& ev);
  void TabAbout(MenuContext& ctx, MenuEvents& ev);

  int tab_ = 0;
};
