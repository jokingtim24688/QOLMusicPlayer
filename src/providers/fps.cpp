#include "providers/fps.h"

#include <algorithm>

#include "util/win.h"

// No present for this long = not presenting (paused, minimized). ETW flushes about once a second, so leave slack.
static constexpr double kStaleSeconds = 2.5;

void FpsProvider::OnPresent(DWORD pid, LONGLONG qpc) {
  std::lock_guard lock(mu_);
  Stats& s = procs_[pid];
  if (s.lastQpc && qpc > s.lastQpc) {
    double ms = QpcSeconds(qpc - s.lastQpc) * 1000.0;
    if (ms < 1000.0) {  // a gap longer than 1 s is a pause, not a frame
      s.frameMs[s.next] = static_cast<float>(ms);
      s.next = (s.next + 1) % kRing;
      s.count = std::min(s.count + 1, kRing);
    }
  }
  s.lastQpc = qpc;
}

bool FpsProvider::Fresh(const Stats& s, LONGLONG now) const {
  return s.lastQpc && QpcSeconds(now - s.lastQpc) < kStaleSeconds;
}

float FpsProvider::AverageFps(const Stats& s, double windowMs, size_t* frames) {
  double sum = 0;
  size_t n = 0;
  while (n < s.count && sum < windowMs) {
    sum += s.frameMs[(s.next + kRing - 1 - n) % kRing];
    ++n;
  }
  if (frames) *frames = n;
  return sum > 0 ? static_cast<float>(n * 1000.0 / sum) : 0.0f;
}

FpsSample FpsProvider::Sample(DWORD pid) {
  std::lock_guard lock(mu_);
  auto it = procs_.find(pid);
  if (it == procs_.end() || !Fresh(it->second, QpcNow()) || it->second.count < 2) return {};
  const Stats& s = it->second;

  FpsSample out;
  out.valid = true;
  out.fps = AverageFps(s, 1000.0, nullptr);

  // 1% low: mean of the slowest 1% of frames over the last 10 s.
  size_t n = 0;
  AverageFps(s, 10000.0, &n);
  std::vector<float> window(n);
  for (size_t i = 0; i < n; ++i) window[i] = s.frameMs[(s.next + kRing - 1 - i) % kRing];
  size_t worst = std::max<size_t>(1, n / 100);
  std::nth_element(window.begin(), window.begin() + static_cast<long>(worst - 1), window.end(), std::greater<float>());
  double sum = 0;
  for (size_t i = 0; i < worst; ++i) sum += window[i];
  out.low1 = sum > 0 ? static_cast<float>(1000.0 * worst / sum) : 0.0f;
  return out;
}

bool FpsProvider::IsPresenting(DWORD pid) {
  std::lock_guard lock(mu_);
  auto it = procs_.find(pid);
  return it != procs_.end() && Fresh(it->second, QpcNow()) && it->second.count >= 10;
}

std::vector<PresentingProcess> FpsProvider::Presenting() {
  std::lock_guard lock(mu_);
  LONGLONG now = QpcNow();
  std::vector<PresentingProcess> out;
  const DWORD self = GetCurrentProcessId();
  for (auto it = procs_.begin(); it != procs_.end();) {
    if (QpcSeconds(now - it->second.lastQpc) > 10.0) {
      it = procs_.erase(it);
      continue;
    }
    if (it->first != self && Fresh(it->second, now) && it->second.count >= 10)
      out.push_back({it->first, AverageFps(it->second, 1000.0, nullptr)});
    ++it;
  }
  std::sort(out.begin(), out.end(), [](auto& a, auto& b) { return a.fps > b.fps; });
  return out;
}
