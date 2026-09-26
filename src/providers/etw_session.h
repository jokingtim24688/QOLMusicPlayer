#pragma once
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>

#include <atomic>
#include <thread>

class FpsProvider;
class PingProvider;

enum class EtwStatus { Stopped, Running, NeedsAdmin, Failed };

// One real-time ETW session shared by the FPS and ping providers.
// Only system-wide present and network events are consumed; the game process is never opened.
class EtwSession {
 public:
  bool Start(FpsProvider* fps, PingProvider* ping);
  void Stop();
  // Network events are only enabled while the ping segment is shown.
  void EnableNetwork(bool enable);
  EtwStatus Status() const { return status_.load(); }

 private:
  static void WINAPI OnEvent(PEVENT_RECORD rec);
  void EnableProvider(const GUID& guid, ULONGLONG keywords, const USHORT* ids, USHORT count, bool enable);

  TRACEHANDLE session_ = 0;
  TRACEHANDLE trace_ = 0;
  std::thread thread_;
  std::atomic<EtwStatus> status_{EtwStatus::Stopped};
  bool networkEnabled_ = false;
};
