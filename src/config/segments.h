#pragma once
#include <algorithm>
#include <string>
#include <vector>

// Everything that can sit on the bar. The order of the list in the config is the order on screen.
// Adding one: an entry here, a data source in App::Tick, and a case in Watermark().
enum class Seg : int { Fps, FrameTime, Ping, Game, Session, Date, Time, Cpu, Gpu, Ram, User };

struct SegInfo {
  Seg id;
  const char* key;   // stored in config.ini
  const char* name;  // shown in the menu
  const char* desc;
};

inline const std::vector<SegInfo>& AllSegments() {
  static const std::vector<SegInfo> all = {
      {Seg::Fps, "fps", "Frame rate", "FPS of the game, with the 1% low"},
      {Seg::FrameTime, "frametime", "Frame time", "Average time per frame, in ms"},
      {Seg::Ping, "ping", "Ping", "Round trip to the game's server"},
      {Seg::Game, "game", "Game", "Name of the game being tracked"},
      {Seg::Session, "session", "Time in game", "How long the current game has been running"},
      {Seg::Date, "date", "Date", "Today's date"},
      {Seg::Time, "time", "Clock", "Local time"},
      {Seg::Cpu, "cpu", "CPU usage", "Total processor load"},
      {Seg::Gpu, "gpu", "GPU usage", "3D engine load, like Task Manager"},
      {Seg::Ram, "ram", "Memory", "RAM in use"},
      {Seg::User, "user", "Name", "Your display name"},
  };
  return all;
}

inline const SegInfo& InfoOf(Seg s) { return AllSegments()[static_cast<size_t>(s)]; }

inline bool HasSeg(const std::vector<Seg>& list, Seg s) { return std::find(list.begin(), list.end(), s) != list.end(); }

inline std::string SegmentsToString(const std::vector<Seg>& list) {
  std::string out;
  for (Seg s : list) out += (out.empty() ? "" : ",") + std::string(InfoOf(s).key);
  return out;
}

inline std::vector<Seg> SegmentsFromString(const std::string& text) {
  std::vector<Seg> out;
  size_t start = 0;
  while (start <= text.size()) {
    size_t end = text.find(',', start);
    if (end == std::string::npos) end = text.size();
    const std::string key = text.substr(start, end - start);
    for (const SegInfo& i : AllSegments())
      if (key == i.key && !HasSeg(out, i.id)) out.push_back(i.id);
    start = end + 1;
  }
  return out;
}
