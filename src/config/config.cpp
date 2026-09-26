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
    else if (k == "show_fps") showFps = ToBool(v);
    else if (k == "show_low") showLow = ToBool(v);
    else if (k == "show_ping") showPing = ToBool(v);
    else if (k == "show_time") showTime = ToBool(v);
    else if (k == "show_user") showUser = ToBool(v);
    else if (k == "clock_24h") clock24h = ToBool(v);
    else if (k == "clock_seconds") clockSeconds = ToBool(v);
    else if (k == "username") username = v;
    else if (k == "pinned_exe") pinnedExe = v;
    else if (k == "menu_vk") menuKey.vk = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "menu_mods") menuKey.mods = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "toggle_vk") toggleKey.vk = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "toggle_mods") toggleKey.mods = static_cast<unsigned>(atoi(v.c_str()));
    else if (k == "overlay_visible") overlayVisible = ToBool(v);
  }
  if (menuKey.vk == 0) menuKey = Hotkey{0x2D, 0};
  if (toggleKey.vk == 0) toggleKey = Hotkey{0x23, 0};
}

void Config::Save() const {
  std::wstring path = ConfigPath(true);
  if (path.empty()) return;
  std::ostringstream o;
  o << "theme=" << theme << "\n"
    << "font=" << font << "\n"
    << "font_size=" << fontSize << "\n"
    << "opacity=" << opacity << "\n"
    << "corner=" << static_cast<int>(corner) << "\n"
    << "logo=" << logoText << "\n"
    << "show_fps=" << showFps << "\n"
    << "show_low=" << showLow << "\n"
    << "show_ping=" << showPing << "\n"
    << "show_time=" << showTime << "\n"
    << "show_user=" << showUser << "\n"
    << "clock_24h=" << clock24h << "\n"
    << "clock_seconds=" << clockSeconds << "\n"
    << "username=" << username << "\n"
    << "pinned_exe=" << pinnedExe << "\n"
    << "menu_vk=" << menuKey.vk << "\n"
    << "menu_mods=" << menuKey.mods << "\n"
    << "toggle_vk=" << toggleKey.vk << "\n"
    << "toggle_mods=" << toggleKey.mods << "\n"
    << "overlay_visible=" << overlayVisible << "\n";
  std::ofstream f(std::filesystem::path(path), std::ios::trunc);
  f << o.str();
}
