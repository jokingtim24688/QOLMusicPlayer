#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <evntcons.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>

enum class PingState { Off, NoGame, Searching, Ok, Blocked };

struct PingSample {
  PingState state = PingState::Off;
  int ms = 0;
  std::string endpoint;  // "155.133.252.10" — shown in the menu for transparency
};

// In-game ping, measured from outside the game:
//  1. Kernel-Network ETW send events tell us which remote address the game sends the most traffic to
//     (the game server or relay).
//  2. We ICMP-ping that address once a second and report the median of the last 5 replies.
class PingProvider {
 public:
  void Start();
  void Stop();
  void SetTarget(DWORD pid);           // 0 = no game
  void OnNetworkSend(PEVENT_RECORD rec);  // ETW thread
  PingSample Sample();

 private:
  struct Endpoint {
    int family = AF_INET;
    std::array<BYTE, 16> addr{};
    bool operator<(const Endpoint& o) const { return family != o.family ? family < o.family : addr < o.addr; }
    bool operator==(const Endpoint& o) const { return family == o.family && addr == o.addr; }
  };
  struct Traffic {
    ULONGLONG udpBytes = 0, tcpBytes = 0;
  };
  struct Layout {
    bool known = false;
    ULONG pidOffset = 0, addrOffset = 8, addrLen = 4;
  };

  void Worker();
  bool PickEndpoint(Endpoint* out);
  bool Echo(const Endpoint& ep, int* ms);
  const Layout& LayoutFor(PEVENT_RECORD rec);

  std::atomic<DWORD> target_{0};
  std::mutex mu_;
  std::map<Endpoint, Traffic> traffic_, lastTraffic_;
  std::array<Layout, 64> layouts_{};

  std::thread thread_;
  std::condition_variable cv_;
  std::mutex cvMu_;
  bool running_ = false;

  std::mutex resultMu_;
  PingSample result_;
  std::deque<int> history_;
  int failures_ = 0;
  Endpoint current_{};
  bool hasCurrent_ = false;
};
