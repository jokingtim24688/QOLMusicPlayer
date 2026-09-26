#include "providers/system_stats.h"

#include <pdh.h>
#include <pdhmsg.h>

#include <algorithm>
#include <vector>

static ULONGLONG ToU64(const FILETIME& f) { return (static_cast<ULONGLONG>(f.dwHighDateTime) << 32) | f.dwLowDateTime; }

SystemStats::~SystemStats() { CloseGpu(); }

void SystemStats::CloseGpu() {
  if (gpuQuery_) PdhCloseQuery(static_cast<PDH_HQUERY>(gpuQuery_));
  gpuQuery_ = gpuCounter_ = nullptr;
  gpu_ = -1.0f;
}

void SystemStats::Update(bool wantGpu) {
  // CPU: share of non-idle time since the last call (kernel time includes idle time).
  FILETIME idle, kernel, user;
  if (GetSystemTimes(&idle, &kernel, &user)) {
    const ULONGLONG i = ToU64(idle), total = ToU64(kernel) + ToU64(user);
    if (lastTotal_ && total > lastTotal_)
      cpu_ = std::clamp(100.0f * (1.0f - static_cast<float>(i - lastIdle_) / static_cast<float>(total - lastTotal_)),
                        0.0f, 100.0f);
    lastIdle_ = i;
    lastTotal_ = total;
  }

  MEMORYSTATUSEX mem{sizeof(mem)};
  if (GlobalMemoryStatusEx(&mem)) ram_ = static_cast<float>(mem.dwMemoryLoad);

  // GPU: sum of every process's 3D engine utilisation, the number Task Manager shows as "3D".
  if (!wantGpu) {
    if (gpuQuery_) CloseGpu();
    return;
  }
  if (!gpuQuery_) {
    PDH_HQUERY q = nullptr;
    PDH_HCOUNTER c = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &q) != ERROR_SUCCESS) return;
    if (PdhAddEnglishCounterW(q, L"\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &c) != ERROR_SUCCESS) {
      PdhCloseQuery(q);
      return;
    }
    gpuQuery_ = q;
    gpuCounter_ = c;
    PdhCollectQueryData(q);  // rate counters need two samples; the first real value comes next second
    return;
  }
  if (PdhCollectQueryData(static_cast<PDH_HQUERY>(gpuQuery_)) != ERROR_SUCCESS) return;
  DWORD size = 0, count = 0;
  const auto counter = static_cast<PDH_HCOUNTER>(gpuCounter_);
  if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, nullptr) != static_cast<PDH_STATUS>(PDH_MORE_DATA))
    return;
  std::vector<BYTE> buf(size);
  auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
  if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, items) != ERROR_SUCCESS) return;
  double sum = 0;
  for (DWORD k = 0; k < count; ++k)
    if (items[k].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || items[k].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA)
      sum += items[k].FmtValue.doubleValue;
  gpu_ = static_cast<float>(std::clamp(sum, 0.0, 100.0));
}
