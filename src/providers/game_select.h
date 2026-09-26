#pragma once
#include <windows.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "providers/fps.h"

struct GameEntry {
  DWORD pid = 0;
  std::string exe;
  float fps = 0.0f;
};

// Decides which process is "the game".
// Auto: the foreground process if it's presenting frames; kept while you alt-tab, dropped when it stops.
// Pinned: the presenting process whose exe name matches (e.g. "cs2.exe").
// Names come from a Toolhelp snapshot, so the game process is never opened.
class GameSelector {
 public:
  void Update(FpsProvider& fps, const std::string& pinnedExe, HWND ownWindow);
  DWORD Pid() const { return pid_; }
  const std::string& Exe() const { return exe_; }
  const std::vector<GameEntry>& Candidates() const { return candidates_; }
  HMONITOR Monitor() const { return monitor_; }
  // Window title of the game ("Counter-Strike 2"), falling back to the exe name without ".exe".
  std::string Title() const;
  double SessionSeconds() const;  // how long the current game has been tracked

 private:
  const std::string& NameOf(DWORD pid);
  void RefreshNames();

  DWORD pid_ = 0;
  std::string exe_;
  HMONITOR monitor_ = nullptr;
  HWND window_ = nullptr;
  LONGLONG sessionStart_ = 0;
  std::vector<GameEntry> candidates_;
  std::unordered_map<DWORD, std::string> names_;
};
