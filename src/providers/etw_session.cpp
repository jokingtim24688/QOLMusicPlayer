#include "providers/etw_session.h"

#include <cstring>
#include <vector>

#include "providers/fps.h"
#include "providers/ping.h"

// Provider GUIDs (public manifests, same ones PresentMon and Windows Performance Recorder use).
static constexpr GUID kDxgi = {0xCA11C036, 0x0102, 0x4A2D, {0xA6, 0xAD, 0xF0, 0x3C, 0xFE, 0xD5, 0xD3, 0xC9}};
static constexpr GUID kD3D9 = {0x783ACA0A, 0x790E, 0x4D7F, {0x84, 0x51, 0xAA, 0x85, 0x05, 0x11, 0xC6, 0xB9}};
static constexpr GUID kKernelNetwork = {0x7DD42A49, 0x5329, 0x4832, {0x8D, 0xFD, 0x43, 0xD9, 0x79, 0x15, 0x3A, 0x88}};

// Event IDs we consume. Everything else is filtered out by ETW before it reaches us.
static constexpr USHORT kDxgiPresentIds[] = {42 /*Present_Start*/, 55 /*PresentMultiplaneOverlay_Start*/};
static constexpr USHORT kD3D9PresentIds[] = {1 /*Present_Start*/};
static constexpr USHORT kNetSendIds[] = {10 /*TCPv4 send*/, 26 /*TCPv6 send*/, 42 /*UDPv4 send*/, 58 /*UDPv6 send*/};
static constexpr ULONGLONG kNetKeywordIpv4 = 0x10, kNetKeywordIpv6 = 0x20;

// EVENT_FILTER_EVENT_ID / EVENT_FILTER_TYPE_EVENT_ID from evntprov.h (Windows 8.1+). Declared here because
// older/mingw SDK headers don't ship them; the layout is fixed by the OS ABI.
struct EventIdFilter {
  BOOLEAN FilterIn;
  UCHAR Reserved;
  USHORT Count;
  USHORT Events[1];
};
static constexpr ULONG kFilterTypeEventId = 0x80000200;

static constexpr wchar_t kSessionName[] = L"QOLOverlay-Trace";

static FpsProvider* g_fps = nullptr;
static PingProvider* g_ping = nullptr;

static std::vector<BYTE> MakeProperties() {
  std::vector<BYTE> buf(sizeof(EVENT_TRACE_PROPERTIES) + sizeof(kSessionName) * 2, 0);
  auto* p = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buf.data());
  p->Wnode.BufferSize = static_cast<ULONG>(buf.size());
  p->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
  p->Wnode.ClientContext = 1;  // QPC timestamps
  p->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
  p->BufferSize = 16;  // KB; small buffers keep memory low, we only need a few thousand events/s
  p->MinimumBuffers = 4;
  p->MaximumBuffers = 16;
  p->FlushTimer = 1;  // seconds
  p->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
  return buf;
}

bool EtwSession::Start(FpsProvider* fps, PingProvider* ping) {
  g_fps = fps;
  g_ping = ping;

  auto props = MakeProperties();
  ULONG rc = StartTraceW(&session_, kSessionName, reinterpret_cast<EVENT_TRACE_PROPERTIES*>(props.data()));
  if (rc == ERROR_ALREADY_EXISTS) {  // left over from a crash: stop it and start fresh
    auto stop = MakeProperties();
    ControlTraceW(0, kSessionName, reinterpret_cast<EVENT_TRACE_PROPERTIES*>(stop.data()), EVENT_TRACE_CONTROL_STOP);
    props = MakeProperties();
    rc = StartTraceW(&session_, kSessionName, reinterpret_cast<EVENT_TRACE_PROPERTIES*>(props.data()));
  }
  if (rc != ERROR_SUCCESS) {
    status_ = rc == ERROR_ACCESS_DENIED ? EtwStatus::NeedsAdmin : EtwStatus::Failed;
    session_ = 0;
    return false;
  }

  EnableProvider(kDxgi, 0, kDxgiPresentIds, 2, true);
  EnableProvider(kD3D9, 0, kD3D9PresentIds, 1, true);

  EVENT_TRACE_LOGFILEW log{};
  log.LoggerName = const_cast<LPWSTR>(kSessionName);
  log.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
  log.EventRecordCallback = &EtwSession::OnEvent;
  trace_ = OpenTraceW(&log);
  if (trace_ == INVALID_PROCESSTRACE_HANDLE) {
    Stop();
    status_ = EtwStatus::Failed;
    return false;
  }
  status_ = EtwStatus::Running;
  thread_ = std::thread([this] {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    ProcessTrace(&trace_, 1, nullptr, nullptr);  // blocks until the session stops
  });
  return true;
}

void EtwSession::Stop() {
  if (session_) {
    auto props = MakeProperties();
    ControlTraceW(session_, nullptr, reinterpret_cast<EVENT_TRACE_PROPERTIES*>(props.data()), EVENT_TRACE_CONTROL_STOP);
    session_ = 0;
  }
  if (trace_ && trace_ != INVALID_PROCESSTRACE_HANDLE) CloseTrace(trace_);
  trace_ = 0;
  if (thread_.joinable()) thread_.join();
  status_ = EtwStatus::Stopped;
}

void EtwSession::EnableNetwork(bool enable) {
  if (!session_ || networkEnabled_ == enable) return;
  networkEnabled_ = enable;
  EnableProvider(kKernelNetwork, kNetKeywordIpv4 | kNetKeywordIpv6, kNetSendIds, 4, enable);
}

void EtwSession::EnableProvider(const GUID& guid, ULONGLONG keywords, const USHORT* ids, USHORT count, bool enable) {
  if (!enable) {
    EnableTraceEx2(session_, &guid, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    return;
  }
  std::vector<BYTE> filter(sizeof(EventIdFilter) + sizeof(USHORT) * count, 0);
  auto* f = reinterpret_cast<EventIdFilter*>(filter.data());
  f->FilterIn = TRUE;
  f->Count = count;
  std::memcpy(f->Events, ids, sizeof(USHORT) * count);

  EVENT_FILTER_DESCRIPTOR desc{};
  desc.Ptr = reinterpret_cast<ULONGLONG>(filter.data());
  desc.Size = static_cast<ULONG>(filter.size());
  desc.Type = kFilterTypeEventId;

  ENABLE_TRACE_PARAMETERS params{};
  params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
  params.EnableFilterDesc = &desc;
  params.FilterDescCount = 1;
  EnableTraceEx2(session_, &guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, keywords, 0, 0, &params);
}

void WINAPI EtwSession::OnEvent(PEVENT_RECORD rec) {
  const EVENT_HEADER& h = rec->EventHeader;
  const USHORT id = h.EventDescriptor.Id;
  if (IsEqualGUID(h.ProviderId, kDxgi)) {
    if (id == 42 || id == 55) g_fps->OnPresent(h.ProcessId, h.TimeStamp.QuadPart);
  } else if (IsEqualGUID(h.ProviderId, kD3D9)) {
    if (id == 1) g_fps->OnPresent(h.ProcessId, h.TimeStamp.QuadPart);
  } else if (IsEqualGUID(h.ProviderId, kKernelNetwork)) {
    g_ping->OnNetworkSend(rec);
  }
}
