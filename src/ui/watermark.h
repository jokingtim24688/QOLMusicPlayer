#pragma once
#include <imgui.h>

#include <string>
#include <vector>

#include "config/segments.h"
#include "providers/fps.h"
#include "providers/ping.h"
#include "ui/theme.h"

struct WatermarkData {
  std::string logo;
  std::vector<Seg> segments;  // what to show, in order
  bool showLow = true;
  FpsSample fps;
  std::string fpsProblem;   // e.g. "no game", "needs admin" — shown instead of a number
  float refreshHz = 60.0f;  // FPS color thresholds follow the monitor refresh rate
  PingSample ping;
  std::string time, date, user, game, session;
  float cpu = -1, gpu = -1, ram = -1;  // percent, -1 = not available yet

  std::string Signature() const;  // changes whenever the drawn output would change
};

struct WatermarkStyle {
  const Theme* theme;
  ImFont* regular;
  ImFont* bold;
  float size;     // text size in pixels (already DPI-scaled)
  float opacity;  // fill opacity
};

// The shared translucent panel (shadow, fill, border). `highlight` 0..1 tints the border with the accent.
void Panel(ImDrawList* dl, ImVec2 pos, ImVec2 size, const WatermarkStyle& st, float radius, float highlight);

// The small glyph used for a segment on the bar (also shown in the menu's Bar tab).
void SegmentIcon(Seg seg, ImDrawList* dl, ImVec2 center, float size, ImU32 col);

// Measures (draw=false) or draws the watermark with its top-left corner at `pos`. Returns its size.
ImVec2 Watermark(ImDrawList* dl, ImVec2 pos, const WatermarkData& d, const WatermarkStyle& s, bool draw);
