#pragma once
#include <windows.h>

// CPU, GPU and RAM load from the same public counters Task Manager reads. No drivers.
class SystemStats {
 public:
  ~SystemStats();
  // Call once a second. GPU counters are only opened while the GPU segment is on (they cost the most).
  void Update(bool wantGpu);
  float Cpu() const { return cpu_; }   // 0..100, -1 until the second sample
  float Gpu() const { return gpu_; }   // 0..100, -1 when unavailable
  float Ram() const { return ram_; }   // 0..100

 private:
  void CloseGpu();

  ULONGLONG lastIdle_ = 0, lastTotal_ = 0;
  float cpu_ = -1.0f, gpu_ = -1.0f, ram_ = 0.0f;
  void* gpuQuery_ = nullptr;    // PDH_HQUERY
  void* gpuCounter_ = nullptr;  // PDH_HCOUNTER
};
