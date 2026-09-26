#include "providers/game_select.h"

#include <tlhelp32.h>

#include "util/win.h"

// Exe names that present frames but are never "the game".
static bool IsExcluded(const std::string& exe) {
  static const char* kSkip[] = {"dwm.exe", "explorer.exe", "discord.exe", "steamwebhelper.exe", "obs64.exe",
                                "chrome.exe", "msedge.exe", "firefox.exe", "nvcontainer.exe", "textinputhost.exe"};
  for (const char* s : kSkip)
    if (_stricmp(exe.c_str(), s) == 0) return true;
  return false;
}

void GameSelector::RefreshNames() {
  names_.clear();
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE) return;
  PROCESSENTRY32W pe{sizeof(pe)};
  for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe))
    names_[pe.th32ProcessID] = WideToUtf8(pe.szExeFile);
  CloseHandle(snap);
}

const std::string& GameSelector::NameOf(DWORD pid) {
  auto it = names_.find(pid);
  if (it == names_.end()) {  // new process since the last snapshot
    RefreshNames();
    it = names_.find(pid);
    if (it == names_.end()) it = names_.emplace(pid, "pid " + std::to_string(pid)).first;
  }
  return it->second;
}

struct FindWindowCtx {
  DWORD pid;
  HWND best;
  LONG bestArea;
};

static BOOL CALLBACK FindLargestWindow(HWND hwnd, LPARAM lp) {
  auto* ctx = reinterpret_cast<FindWindowCtx*>(lp);
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid != ctx->pid || !IsWindowVisible(hwnd)) return TRUE;
  RECT r;
  GetWindowRect(hwnd, &r);
  LONG area = (r.right - r.left) * (r.bottom - r.top);
  if (area > ctx->bestArea) {
    ctx->bestArea = area;
    ctx->best = hwnd;
  }
  return TRUE;
}

void GameSelector::Update(FpsProvider& fps, const std::string& pinnedExe, HWND ownWindow) {
  candidates_.clear();
  for (const PresentingProcess& p : fps.Presenting()) {
    const std::string& exe = NameOf(p.pid);
    if (!IsExcluded(exe)) candidates_.push_back({p.pid, exe, p.fps});
  }

  DWORD next = 0;
  if (!pinnedExe.empty()) {
    for (const GameEntry& g : candidates_)
      if (_stricmp(g.exe.c_str(), pinnedExe.c_str()) == 0) next = g.pid;
  } else {
    DWORD fg = 0;
    HWND fgWnd = GetForegroundWindow();
    if (fgWnd && fgWnd != ownWindow) GetWindowThreadProcessId(fgWnd, &fg);
    for (const GameEntry& g : candidates_)
      if (g.pid == fg) next = fg;
    if (!next)  // alt-tabbed out: keep the current game while it's still presenting
      for (const GameEntry& g : candidates_)
        if (g.pid == pid_) next = pid_;
  }

  if (next != pid_) {
    pid_ = next;
    exe_ = next ? NameOf(next) : std::string();
    monitor_ = nullptr;
    window_ = nullptr;
    sessionStart_ = next ? QpcNow() : 0;
    if (next) {
      FindWindowCtx ctx{next, nullptr, 0};
      EnumWindows(FindLargestWindow, reinterpret_cast<LPARAM>(&ctx));
      window_ = ctx.best;
      if (ctx.best) monitor_ = MonitorFromWindow(ctx.best, MONITOR_DEFAULTTOPRIMARY);
    }
  }
}

std::string GameSelector::Title() const {
  if (!pid_) return {};
  wchar_t buf[128];
  // GetWindowText on another process's window returns the cached title; nothing is sent to the game.
  if (window_ && IsWindow(window_) && GetWindowTextW(window_, buf, 128) > 0) return WideToUtf8(buf);
  std::string name = exe_;
  if (name.size() > 4 && _stricmp(name.c_str() + name.size() - 4, ".exe") == 0) name.resize(name.size() - 4);
  return name;
}

double GameSelector::SessionSeconds() const { return sessionStart_ ? QpcSeconds(QpcNow() - sessionStart_) : 0.0; }
