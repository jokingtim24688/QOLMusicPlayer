#pragma once
#include <windows.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

enum class UpdateState { Idle, Checking, UpToDate, Downloading, Ready, Failed };

struct UpdateStatus {
  UpdateState state = UpdateState::Idle;
  std::string latest;  // "0.3.1" once known
  std::string error;
};

// Checks GitHub Releases for a newer version (at start-up, then every 6 hours or on request), downloads the
// installer, and verifies it against the SHA-256 published next to it. The app decides when to install:
// automatically only while no game is running, so an update never interrupts a match.
class Updater {
 public:
  static constexpr UINT kReadyMsg = WM_APP + 2;  // posted to the window when an installer is verified

  void Start(HWND notify);
  void Stop();
  void CheckNow();
  UpdateStatus Status();

  // Runs the verified installer silently (it closes this app, updates, and starts it again).
  bool Install();

 private:
  void Worker();
  bool CheckAndDownload();

  HWND notify_ = nullptr;
  std::thread thread_;
  std::mutex mu_;
  std::condition_variable cv_;
  bool running_ = false;
  bool checkRequested_ = false;
  UpdateStatus status_;
  std::wstring installer_;
};

// "0.3.1" > "0.3.0"? Accepts a leading 'v'. Missing parts count as 0.
bool IsNewerVersion(const std::string& candidate, const std::string& current);
