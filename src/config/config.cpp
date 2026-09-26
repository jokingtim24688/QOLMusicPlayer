#include "config/config.h"

#include <shlobj.h>
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "util/win.h"

// Plain key=value file in %APPDATA%\QOLOverlay\config.ini. Unknown keys are ignored, missing keys keep defaults.

static std::wstring ConfigPath(bool createDir) {
  PWSTR appdata = nullptr;
  std::wstring path;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
    path = std::wstring(appdata) + L"\\QOLOverlay";
    CoTaskMemFree(appdata);
    if (createDir) CreateDirectoryW(path.c_str(), nullptr);
    path += L"\\config.ini";
  }
  return path;
}

static bool ToBool(const std::string& v) { return v == "1" || v == "true"; }

void Config::Load() {
  std::ifstream f(std::filesystem::path(ConfigPath(false)));
  if (!f) return;
  // v1 configs had one show_* flag per segment instead of an ordered list.
  int version = 1;
  bool hasSegments = false, oldFps = true, oldPing = true, oldTime = true, oldUser = true;
  std::string line;
  while (std::getline(f, line)) {
    auto eq = line.find('=');
    if (eq == std::string::npos || line[0] == '#') continue;
    std::string k = line.substr(0, eq), v = line.substr(eq + 1);
    if (!v.empty() && v.back() == '\r') v.pop_back();
    if (k == "theme") theme = v;
    else if (k == "font") font = v;
    else if (k == "font_size") fontSize = std::clamp(static_cast<float>(atof(v.c_str())), 11.0f, 24.0f);
    else if (k == "opacity") opacity = std::clamp(static_cast<float>(atof(v.c_str())), kMinOpacity, 1.0f);
    else if (k == "corner") corner = static_cast<Corner>(std::clamp(atoi(v.c_str()), 0, 3));
    else if (k == "logo") logoText = v;
    else if (k == "version") version = atoi(v.c_str());
    else if (k == "segments") {
      segments = SegmentsFromString(v);
      hasSegments = true;
    }
    else if (k == "show_fps") oldFps = ToBool(v);
    else if (k == "show_ping") oldPing = ToBool(v);
    else if (k == "show_time") oldTime = ToBool(v);
    else if (k == "show_user") oldUser = ToBool(v);
    else if (k == "show_low") showLow = ToBool(v);
    else if (k == "date_format") dateFormat = std::clamp(atoi(v.c_str()), 0, 4);
    else if (k == "clock_24h") clock24h = ToBool(v);
    else if (k == "clock_seconds") clockSeconds = ToBool(v);
    else if (k == "username") username = v;
    else if (k == "pinned_exe") pinnedExe = v;
    else if (k == "show_player") showPlayer = ToBool(v);
    else if (k == "player_spotify_only") playerSpotifyOnly = ToBool(v);
    else if (k == "player_hide_idle") playerHideWhenIdle = ToBool(v);
    else if (k == "player_x") playerX = std::clamp(static_cast<float>(atof(v.c_str())), 0.0f, 1.0f);
    else if (k == "player_y") playerY = std::clamp(static_cast<float>(atof(v.c_str())), 0.0f, 1.0f);
    else if (k == "menu_vk") menuKey.vk = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "menu_mods") menuKey.mods = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "toggle_vk") toggleKey.vk = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "toggle_mods") toggleKey.mods = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "overlay_visible") overlayVisible = ToBool(v);
    else if (k == "auto_update") autoUpdate = ToBool(v);
    else if (k == "last_version") lastVersion = v;
  }
  if (!hasSegments) {  // upgrade from v1: keep what was shown, and add the date before the clock
    segments.clear();
    if (oldFps) segments.push_back(Seg::Fps);
    if (oldPing) segments.push_back(Seg::Ping);
    segments.push_back(Seg::Date);
    if (oldTime) segments.push_back(Seg::Time);
    if (oldUser) segments.push_back(Seg::User);
  }
  if (version < 2 && menuKey.vk == 0x2D /*Insert*/ && menuKey.mods == 0) menuKey = Hotkey{0x2E /*Delete*/, 0};
  if (menuKey.vk == 0) menuKey = Hotkey{0x2E, 0};
  if (toggleKey.vk == 0) toggleKey = Hotkey{0x23, 0};
}

void Config::Save() const {
  std::wstring path = ConfigPath(true);
  if (path.empty()) return;
  std::ostringstream o;
  o << "version=2\n"
    << "theme=" << theme << "\n"
    << "font=" << font << "\n"
    << "font_size=" << fontSize << "\n"
    << "opacity=" << opacity << "\n"
    << "corner=" << static_cast<int>(corner) << "\n"
    << "logo=" << logoText << "\n"
    << "segments=" << SegmentsToString(segments) << "\n"
    << "show_low=" << showLow << "\n"
    << "date_format=" << dateFormat << "\n"
    << "clock_24h=" << clock24h << "\n"
    << "clock_seconds=" << clockSeconds << "\n"
    << "username=" << username << "\n"
    << "pinned_exe=" << pinnedExe << "\n"
    << "show_player=" << showPlayer << "\n"
    << "player_spotify_only=" << playerSpotifyOnly << "\n"
    << "player_hide_idle=" << playerHideWhenIdle << "\n"
    << "player_x=" << playerX << "\n"
    << "player_y=" << playerY << "\n"
    << "menu_vk=" << menuKey.vk << "\n"
    << "menu_mods=" << menuKey.mods << "\n"
    << "toggle_vk=" << toggleKey.vk << "\n"
    << "toggle_mods=" << toggleKey.mods << "\n"
    << "overlay_visible=" << overlayVisible << "\n"
    << "auto_update=" << autoUpdate << "\n"
    << "last_version=" << lastVersion << "\n";
  std::ofstream f(std::filesystem::path(path), std::ios::trunc);
  f << o.str();
}
