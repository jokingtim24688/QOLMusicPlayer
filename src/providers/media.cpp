#include "providers/media.h"

#include <algorithm>
#include <chrono>
#include <cwctype>

#include "util/win.h"

#if defined(_MSC_VER)
// C++/WinRT ships with the Windows SDK (MSVC). Classic COM headers first so winrt interops with IStream/WIC.
#include <unknwn.h>
#include <shcore.h>
#include <wincodec.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#define QOL_HAS_SMTC 1
#endif

void MediaProvider::Start() {
  running_ = true;
  thread_ = std::thread([this] { Worker(); });
}

void MediaProvider::Stop() {
  {
    std::lock_guard lock(mu_);
    running_ = false;
  }
  cv_.notify_all();
  if (thread_.joinable()) thread_.join();
}

void MediaProvider::Configure(bool enabled, bool spotifyOnly) {
  std::lock_guard lock(mu_);
  if (enabled_ == enabled && spotifyOnly_ == spotifyOnly) return;
  enabled_ = enabled;
  spotifyOnly_ = spotifyOnly;
  cv_.notify_all();
}

void MediaProvider::Send(MediaCommand cmd) {
  std::lock_guard lock(mu_);
  pending_ = cmd;
  cv_.notify_all();
}

MediaState MediaProvider::Snapshot() {
  std::lock_guard lock(mu_);
  return state_;
}

#if QOL_HAS_SMTC

namespace wmc = winrt::Windows::Media::Control;

// Decodes the cover into a centered square, 128 px, BGRA.
static std::shared_ptr<MediaArt> DecodeArt(const winrt::Windows::Storage::Streams::IRandomAccessStreamReference& ref) {
  if (!ref) return nullptr;
  try {
    auto stream = ref.OpenReadAsync().get();
    winrt::com_ptr<IStream> istream;
    winrt::check_hresult(CreateStreamOverRandomAccessStream(winrt::get_unknown(stream), IID_PPV_ARGS(istream.put())));
    winrt::com_ptr<IWICImagingFactory> wic;
    winrt::check_hresult(
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.put())));
    winrt::com_ptr<IWICBitmapDecoder> decoder;
    winrt::check_hresult(
        wic->CreateDecoderFromStream(istream.get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.put()));
    winrt::com_ptr<IWICBitmapFrameDecode> frame;
    winrt::check_hresult(decoder->GetFrame(0, frame.put()));

    UINT w = 0, h = 0;
    winrt::check_hresult(frame->GetSize(&w, &h));
    const UINT side = std::min(w, h);
    WICRect crop{static_cast<INT>((w - side) / 2), static_cast<INT>((h - side) / 2), static_cast<INT>(side),
                 static_cast<INT>(side)};
    winrt::com_ptr<IWICBitmapClipper> clipper;
    winrt::check_hresult(wic->CreateBitmapClipper(clipper.put()));
    winrt::check_hresult(clipper->Initialize(frame.get(), &crop));

    constexpr UINT kSize = 128;
    winrt::com_ptr<IWICBitmapScaler> scaler;
    winrt::check_hresult(wic->CreateBitmapScaler(scaler.put()));
    winrt::check_hresult(scaler->Initialize(clipper.get(), kSize, kSize, WICBitmapInterpolationModeHighQualityCubic));
    winrt::com_ptr<IWICFormatConverter> conv;
    winrt::check_hresult(wic->CreateFormatConverter(conv.put()));
    winrt::check_hresult(conv->Initialize(scaler.get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr,
                                          0.0, WICBitmapPaletteTypeCustom));
    auto art = std::make_shared<MediaArt>();
    art->width = art->height = static_cast<int>(kSize);
    art->bgra.resize(kSize * kSize * 4);
    winrt::check_hresult(conv->CopyPixels(nullptr, kSize * 4, static_cast<UINT>(art->bgra.size()), art->bgra.data()));
    return art;
  } catch (...) {
    return nullptr;  // no cover is fine; the widget shows a placeholder
  }
}

static bool IsSpotify(const wmc::GlobalSystemMediaTransportControlsSession& s) {
  std::wstring id{s.SourceAppUserModelId()};
  std::transform(id.begin(), id.end(), id.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
  return id.find(L"spotify") != std::wstring::npos;
}

void MediaProvider::Worker() {
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  wmc::GlobalSystemMediaTransportControlsSessionManager manager{nullptr};
  std::string lastKey;

  for (;;) {
    bool enabled, spotifyOnly;
    MediaCommand cmd;
    {
      std::unique_lock lock(mu_);
      cv_.wait_for(lock, std::chrono::seconds(1), [this] { return !running_ || pending_ != MediaCommand::None; });
      if (!running_) break;
      enabled = enabled_;
      spotifyOnly = spotifyOnly_;
      cmd = pending_;
      pending_ = MediaCommand::None;
    }
    if (!enabled) {
      std::lock_guard lock(mu_);
      state_ = MediaState{};
      lastKey.clear();
      continue;
    }

    MediaState next;
    try {
      if (!manager) manager = wmc::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
      wmc::GlobalSystemMediaTransportControlsSession session{nullptr};
      for (const auto& s : manager.GetSessions())
        if (IsSpotify(s)) {
          session = s;
          break;
        }
      if (!session && !spotifyOnly) session = manager.GetCurrentSession();

      if (session) {
        if (cmd == MediaCommand::PlayPause) session.TryTogglePlayPauseAsync().get();
        else if (cmd == MediaCommand::Next) session.TrySkipNextAsync().get();
        else if (cmd == MediaCommand::Previous) session.TrySkipPreviousAsync().get();

        auto props = session.TryGetMediaPropertiesAsync().get();
        auto info = session.GetPlaybackInfo();
        auto timeline = session.GetTimelineProperties();
        using seconds = std::chrono::duration<double>;

        next.active = true;
        next.title = winrt::to_string(props.Title());
        next.artist = winrt::to_string(props.Artist());
        next.playing = info.PlaybackStatus() == wmc::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        next.durationSec = seconds(timeline.EndTime() - timeline.StartTime()).count();
        double pos = seconds(timeline.Position() - timeline.StartTime()).count();
        if (next.playing) pos += seconds(winrt::clock::now() - timeline.LastUpdatedTime()).count();
        next.positionSec = std::clamp(pos, 0.0, std::max(next.durationSec, 0.0));
        next.sampledQpc = QpcNow();

        // Only fetch and decode the cover when the track changes.
        const std::string key = next.title + '\x1f' + next.artist;
        std::lock_guard lock(mu_);
        if (key != lastKey) {
          lastKey = key;
          next.art = DecodeArt(props.Thumbnail());
          next.artVersion = state_.artVersion + 1;
        } else {
          next.art = state_.art;
          next.artVersion = state_.artVersion;
        }
        state_ = std::move(next);
        continue;
      }
    } catch (...) {
      manager = nullptr;  // SMTC can go away (e.g. explorer restart); request it again next tick
    }
    std::lock_guard lock(mu_);
    next.artVersion = state_.artVersion;
    state_ = std::move(next);
    lastKey.clear();
  }
  winrt::uninit_apartment();
}

#else  // No C++/WinRT (mingw compile checks): the player just reports "nothing playing".

void MediaProvider::Worker() {
  std::unique_lock lock(mu_);
  while (running_) cv_.wait(lock);
}

#endif
