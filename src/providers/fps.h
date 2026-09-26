#pragma once
#include <windows.h>

#include <array>
#include <mutex>
#include <unordered_map>
#include <vector>

struct FpsSample {
  bool valid = false;  // false when the process hasn't presented recently
  float fps = 0.0f;    // average over the last second
  float low1 = 0.0f;   // 1% low over the last 10 seconds
};

struct PresentingProcess {
  DWORD pid = 0;
  float fps = 0.0f;
};

// Frame times from DXGI/D3D9 present events (the PresentMon method). No injection, no hooks.
class FpsProvider {
 public:
  void OnPresent(DWORD pid, LONGLONG qpc);  // ETW thread
  FpsSample Sample(DWORD pid);
  bool IsPresenting(DWORD pid);
  std::vector<PresentingProcess> Presenting();  // also prunes processes that stopped presenting

 private:
  static constexpr size_t kRing = 4096;  // >10 s of history at 400 fps
  struct Stats {
    std::array<float, kRing> frameMs{};
    size_t next = 0, count = 0;
    LONGLONG lastQpc = 0;
  };
  static float AverageFps(const Stats& s, double windowMs, size_t* frames);
  bool Fresh(const Stats& s, LONGLONG now) const;

  std::mutex mu_;
  std::unordered_map<DWORD, Stats> procs_;
};
