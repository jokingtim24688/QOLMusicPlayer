#include "providers/ping.h"

#include <iphlpapi.h>
#include <icmpapi.h>
#include <tdh.h>

#include <algorithm>
#include <cstring>
#include <vector>

void PingProvider::Start() {
  running_ = true;
  thread_ = std::thread([this] { Worker(); });
}

void PingProvider::Stop() {
  {
    std::lock_guard lock(cvMu_);
    running_ = false;
  }
  cv_.notify_all();
  if (thread_.joinable()) thread_.join();
}

void PingProvider::SetTarget(DWORD pid) {
  if (target_.exchange(pid) == pid) return;
  std::lock_guard lock(mu_);
  traffic_.clear();
  lastTraffic_.clear();
}

// Work out where PID and destination address live in the event payload, once per event ID, from the
// provider's own schema (TDH). Falls back to the documented layout: PID @0, size @4, daddr @8.
const PingProvider::Layout& PingProvider::LayoutFor(PEVENT_RECORD rec) {
  const USHORT id = rec->EventHeader.EventDescriptor.Id;
  Layout& l = layouts_[id % layouts_.size()];
  if (l.known) return l;
  l.known = true;
  l.addrLen = (id == 26 || id == 58) ? 16 : 4;

  ULONG size = 0;
  if (TdhGetEventInformation(rec, 0, nullptr, nullptr, &size) != ERROR_INSUFFICIENT_BUFFER) return l;
  std::vector<BYTE> buf(size);
  auto* info = reinterpret_cast<TRACE_EVENT_INFO*>(buf.data());
  if (TdhGetEventInformation(rec, 0, nullptr, info, &size) != ERROR_SUCCESS) return l;

  ULONG offset = 0;
  bool foundPid = false, foundAddr = false;
  for (ULONG i = 0; i < info->TopLevelPropertyCount; ++i) {
    const EVENT_PROPERTY_INFO& p = info->EventPropertyInfoArray[i];
    // Only fixed-size scalar fields can be summed into offsets; stop at anything else.
    if (p.Flags & (PropertyStruct | PropertyParamLength | PropertyParamCount)) break;
    const auto* name = reinterpret_cast<const wchar_t*>(buf.data() + p.NameOffset);
    if (_wcsicmp(name, L"PID") == 0) {
      l.pidOffset = offset;
      foundPid = true;
    } else if (_wcsicmp(name, L"daddr") == 0) {
      l.addrOffset = offset;
      l.addrLen = p.length;
      foundAddr = true;
    }
    if (p.length == 0) break;
    offset += p.length;
  }
  if (!foundPid || !foundAddr || (l.addrLen != 4 && l.addrLen != 16)) {
    l.pidOffset = 0;
    l.addrOffset = 8;
    l.addrLen = (id == 26 || id == 58) ? 16 : 4;
  }
  return l;
}

void PingProvider::OnNetworkSend(PEVENT_RECORD rec) {
  const DWORD target = target_.load(std::memory_order_relaxed);
  if (!target) return;
  const Layout& l = LayoutFor(rec);
  const BYTE* data = static_cast<const BYTE*>(rec->UserData);
  if (rec->UserDataLength < l.addrOffset + l.addrLen || rec->UserDataLength < l.pidOffset + 8) return;

  DWORD pid;
  std::memcpy(&pid, data + l.pidOffset, 4);
  if (pid != target) return;
  DWORD bytes;
  std::memcpy(&bytes, data + l.pidOffset + 4, 4);

  Endpoint ep;
  ep.family = l.addrLen == 16 ? AF_INET6 : AF_INET;
  std::memcpy(ep.addr.data(), data + l.addrOffset, l.addrLen);

  const USHORT id = rec->EventHeader.EventDescriptor.Id;
  std::lock_guard lock(mu_);
  Traffic& t = traffic_[ep];
  (id == 42 || id == 58 ? t.udpBytes : t.tcpBytes) += bytes;
}

static bool IsPrivate(int family, const BYTE* a) {
  if (family == AF_INET) {
    return a[0] == 0 || a[0] == 10 || a[0] == 127 || (a[0] == 169 && a[1] == 254) ||
           (a[0] == 172 && (a[1] & 0xF0) == 16) || (a[0] == 192 && a[1] == 168) || a[0] >= 224;
  }
  static const BYTE loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  return std::memcmp(a, loopback, 16) == 0 || (a[0] == 0xfe && (a[1] & 0xC0) == 0x80) || (a[0] & 0xFE) == 0xfc ||
         a[0] == 0xff;
}

// Busiest public UDP destination wins (game traffic); TCP only if the game sends no UDP;
// a private address (LAN server) only if there's nothing public.
bool PingProvider::PickEndpoint(Endpoint* out) {
  std::lock_guard lock(mu_);
  std::map<Endpoint, Traffic> merged = lastTraffic_;
  for (auto& [ep, t] : traffic_) {
    merged[ep].udpBytes += t.udpBytes;
    merged[ep].tcpBytes += t.tcpBytes;
  }
  auto score = [](const Endpoint& ep, const Traffic& t) -> ULONGLONG {
    ULONGLONG s = t.udpBytes ? (1ull << 40) + t.udpBytes : t.tcpBytes;
    if (!IsPrivate(ep.family, ep.addr.data())) s += 1ull << 50;
    return s;
  };
  ULONGLONG best = 0;
  for (auto& [ep, t] : merged) {
    ULONGLONG s = score(ep, t);
    if (s > best && (t.udpBytes || t.tcpBytes)) {
      best = s;
      *out = ep;
    }
  }
  return best > 0;
}

bool PingProvider::Echo(const Endpoint& ep, int* ms) {
  char payload[8] = "qolping";
  if (ep.family == AF_INET) {
    HANDLE h = IcmpCreateFile();
    if (h == INVALID_HANDLE_VALUE) return false;
    BYTE reply[sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 8 + 32];
    IPAddr addr;
    std::memcpy(&addr, ep.addr.data(), 4);
    DWORD n = IcmpSendEcho(h, addr, payload, sizeof(payload), nullptr, reply, sizeof(reply), 1000);
    IcmpCloseHandle(h);
    auto* r = reinterpret_cast<ICMP_ECHO_REPLY*>(reply);
    if (n == 0 || r->Status != IP_SUCCESS) return false;
    *ms = static_cast<int>(r->RoundTripTime);
    return true;
  }
  HANDLE h = Icmp6CreateFile();
  if (h == INVALID_HANDLE_VALUE) return false;
  sockaddr_in6 src{}, dst{};
  src.sin6_family = AF_INET6;
  dst.sin6_family = AF_INET6;
  std::memcpy(&dst.sin6_addr, ep.addr.data(), 16);
  BYTE reply[sizeof(ICMPV6_ECHO_REPLY) + sizeof(payload) + 8 + 32];
  DWORD n = Icmp6SendEcho2(h, nullptr, nullptr, nullptr, &src, &dst, payload, sizeof(payload), nullptr, reply,
                           sizeof(reply), 1000);
  IcmpCloseHandle(h);
  if (n == 0) return false;
  auto* r = reinterpret_cast<ICMPV6_ECHO_REPLY*>(reply);
  if (r->Status != IP_SUCCESS) return false;
  *ms = static_cast<int>(r->RoundTripTime);
  return true;
}

static std::string EndpointText(int family, const BYTE* addr) {
  char buf[INET6_ADDRSTRLEN]{};
  inet_ntop(family, addr, buf, sizeof(buf));
  return buf;
}

void PingProvider::Worker() {
  int tick = 0;
  for (;;) {
    {
      std::unique_lock lock(cvMu_);
      if (cv_.wait_for(lock, std::chrono::seconds(1), [this] { return !running_; })) return;
    }
    // Rotate the traffic window every 5 s so a server change is picked up quickly.
    if (++tick % 5 == 0) {
      std::lock_guard lock(mu_);
      lastTraffic_ = std::move(traffic_);
      traffic_.clear();
    }

    PingSample next;
    if (!target_.load()) {
      next.state = PingState::NoGame;
      hasCurrent_ = false;
      history_.clear();
    } else {
      Endpoint ep;
      if (!PickEndpoint(&ep)) {
        next.state = PingState::Searching;
      } else {
        if (!hasCurrent_ || !(ep == current_)) {
          current_ = ep;
          hasCurrent_ = true;
          history_.clear();
          failures_ = 0;
        }
        next.endpoint = EndpointText(ep.family, ep.addr.data());
        int ms = 0;
        // A server that ignores ICMP gets re-checked every 5 s instead of every second.
        const bool attempt = failures_ < 3 || tick % 5 == 0;
        if (!attempt) {
        } else if (Echo(ep, &ms)) {
          failures_ = 0;
          history_.push_back(ms);
          if (history_.size() > 5) history_.pop_front();
        } else if (++failures_ >= 3) {
          history_.clear();
        }
        if (!history_.empty()) {
          std::vector<int> sorted(history_.begin(), history_.end());
          std::nth_element(sorted.begin(), sorted.begin() + static_cast<long>(sorted.size() / 2), sorted.end());
          next.state = PingState::Ok;
          next.ms = sorted[sorted.size() / 2];
        } else {
          next.state = failures_ >= 3 ? PingState::Blocked : PingState::Searching;
        }
      }
    }
    std::lock_guard lock(resultMu_);
    result_ = next;
  }
}

PingSample PingProvider::Sample() {
  std::lock_guard lock(resultMu_);
  return result_;
}
