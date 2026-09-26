#pragma once
#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct MediaArt {
  int width = 0, height = 0;
  std::vector<uint8_t> bgra;  // straight alpha, top-down rows
};

struct MediaState {
  bool active = false;   // a matching media session exists
  bool playing = false;
  std::string title, artist;
  double positionSec = 0, durationSec = 0;  // position at `sampledQpc`
  LONGLONG sampledQpc = 0;
  unsigned artVersion = 0;  // bumps whenever the cover changes
  std::shared_ptr<const MediaArt> art;
};

enum class MediaCommand { None, PlayPause, Next, Previous };

// Now playing + transport controls through Windows' System Media Transport Controls: the same API the
// volume flyout uses. Spotify publishes to it, so no Spotify login or Web API is involved.
// Polls once a second on its own thread (MTA); does nothing while disabled.
class MediaProvider {
 public:
  void Start();
  void Stop();
  void Configure(bool enabled, bool spotifyOnly);
  void Send(MediaCommand cmd);
  MediaState Snapshot();

 private:
  void Worker();

  std::thread thread_;
  std::mutex mu_;
  std::condition_variable cv_;
  bool running_ = false;
  bool enabled_ = false;
  bool spotifyOnly_ = true;
  MediaCommand pending_ = MediaCommand::None;
  MediaState state_;
};
