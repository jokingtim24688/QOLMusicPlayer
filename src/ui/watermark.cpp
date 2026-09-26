#include "ui/watermark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

std::string WatermarkData::Signature() const {
  char buf[256];
  snprintf(buf, sizeof(buf), "%d%d%d%d%d|%d|%.0f|%.0f|%s|%d|%d|%s|%s|%s|%s", showFps, showLow, showPing, showTime,
           showUser, fps.valid, fps.fps, fps.low1, fpsProblem.c_str(), static_cast<int>(ping.state), ping.ms,
           time.c_str(), user.c_str(), logo.c_str(), ping.endpoint.c_str());
  return buf;
}

namespace {

constexpr float kPi = 3.14159265f;

// Draws text left to right; digits sit in fixed-width cells (tabular figures) so numbers don't jitter.
struct Pen {
  ImDrawList* dl;
  bool draw;
  float x, y, h;  // h = row height; text is vertically centered in it

  float DigitCell(ImFont* f, float size) {
    ImFontBaked* baked = f->GetFontBaked(size);
    float w = 0;
    for (char c = '0'; c <= '9'; ++c) w = std::max(w, baked->GetCharAdvance(static_cast<ImWchar>(c)));
    return w;
  }

  // minDigits reserves width so e.g. 99 → 100 fps doesn't resize the bar every second.
  void Text(ImFont* f, float size, ImU32 col, const char* text, bool tabular = false, int minDigits = 0) {
    const float top = y + (h - size) * 0.5f;
    if (!tabular) {
      ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
      if (draw) dl->AddText(f, size, ImVec2(x, top), col, text);
      x += ts.x;
      return;
    }
    const float cell = DigitCell(f, size);
    int digits = 0;
    for (const char* c = text; *c; ++c) digits += (*c >= '0' && *c <= '9');
    x += cell * static_cast<float>(std::max(0, minDigits - digits));  // right-align within reserved width
    char one[2] = {0, 0};
    for (const char* c = text; *c; ++c) {
      one[0] = *c;
      float adv = f->CalcTextSizeA(size, FLT_MAX, 0.0f, one).x;
      if (*c >= '0' && *c <= '9') {
        if (draw) dl->AddText(f, size, ImVec2(x + (cell - adv) * 0.5f, top), col, one);
        x += cell;
      } else {
        if (draw) dl->AddText(f, size, ImVec2(x, top), col, one);
        x += adv;
      }
    }
  }
  void Space(float w) { x += w; }
};

void IconBars(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {  // FPS
  const float bw = s * 0.2f, gap = s * 0.12f;
  const float heights[3] = {0.45f, 0.7f, 1.0f};
  float x = c.x - (bw * 3 + gap * 2) * 0.5f;
  for (float hf : heights) {
    dl->AddRectFilled(ImVec2(x, c.y + s * 0.5f - s * hf), ImVec2(x + bw, c.y + s * 0.5f), col, bw * 0.4f);
    x += bw + gap;
  }
}

void IconSignal(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {  // ping
  const ImVec2 o(c.x, c.y + s * 0.4f);
  dl->AddCircleFilled(o, s * 0.1f, col);
  for (float r : {s * 0.42f, s * 0.75f}) {
    dl->PathArcTo(o, r, -kPi * 0.75f, -kPi * 0.25f, 12);
    dl->PathStroke(col, s * 0.13f);
  }
}

void IconClock(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
  const float r = s * 0.46f, t = s * 0.12f;
  dl->AddCircle(c, r, col, 20, t);
  dl->AddLine(c, ImVec2(c.x, c.y - r * 0.55f), col, t);
  dl->AddLine(c, ImVec2(c.x + r * 0.45f, c.y), col, t);
}

void IconUser(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
  dl->AddCircleFilled(ImVec2(c.x, c.y - s * 0.2f), s * 0.24f, col, 16);
  dl->PathArcTo(ImVec2(c.x, c.y + s * 0.5f), s * 0.42f, kPi, 2 * kPi, 14);
  dl->PathFillConvex(col);
}

}  // namespace

ImVec2 Watermark(ImDrawList* dl, ImVec2 pos, const WatermarkData& d, const WatermarkStyle& st, bool draw) {
  const Theme& t = *st.theme;
  const float fs = st.size, small = fs * 0.8f;
  const float height = std::round(fs * 2.1f), padX = std::round(fs * 0.85f), gap = std::round(fs * 0.75f);
  const float icon = fs * 0.85f;
  const float radius = std::round(fs * 0.45f);

  // Pass 1 measures, pass 2 draws on top of the background that needs the measured width.
  auto layout = [&](bool doDraw) -> float {
    Pen pen{dl, doDraw, pos.x + padX, pos.y, height};
    bool first = true;
    auto divider = [&] {
      if (doDraw) {
        const float x = std::round(pen.x + gap);
        dl->AddLine(ImVec2(x, pos.y + height * 0.28f), ImVec2(x, pos.y + height * 0.72f), Col(t.overlay, 0.55f),
                    1.0f);
      }
      pen.Space(gap * 2 + 1);
    };
    auto segmentIcon = [&](void (*fn)(ImDrawList*, ImVec2, float, ImU32)) {
      if (!first) divider();
      first = false;
      if (doDraw) fn(dl, ImVec2(pen.x + icon * 0.5f, pos.y + height * 0.5f), icon, Col(t.accent));
      pen.Space(icon + fs * 0.45f);
    };

    if (!d.logo.empty()) {
      const float x0 = pen.x;
      pen.Text(st.bold, fs * 1.05f, Col(t.accent), d.logo.c_str());
      if (doDraw) {  // accent underline that fades out: the one structural gradient
        const float y = pos.y + height - 3.0f;
        dl->AddRectFilledMultiColor(ImVec2(x0, y), ImVec2(pen.x + gap, y + 2.0f), Col(t.accent, 0.9f),
                                    Col(t.accent, 0.0f), Col(t.accent, 0.0f), Col(t.accent, 0.9f));
      }
      first = false;
    }

    if (d.showFps) {
      segmentIcon(IconBars);
      if (!d.fps.valid) {
        pen.Text(st.regular, small, Col(t.subtext), d.fpsProblem.empty() ? "no game" : d.fpsProblem.c_str());
      } else {
        const float hz = d.refreshHz > 0 ? d.refreshHz : 60.0f;
        const ImVec4& c = d.fps.fps >= hz * 0.95f ? t.good : d.fps.fps >= hz * 0.6f ? t.warn : t.bad;
        char num[16];
        snprintf(num, sizeof(num), "%d", static_cast<int>(std::lround(d.fps.fps)));
        pen.Text(st.bold, fs, Col(c), num, true, 3);
        pen.Space(fs * 0.25f);
        pen.Text(st.regular, small, Col(t.subtext), "FPS");
        if (d.showLow) {
          pen.Space(fs * 0.6f);
          snprintf(num, sizeof(num), "%d", static_cast<int>(std::lround(d.fps.low1)));
          pen.Text(st.regular, fs, Col(t.text), num, true, 3);
          pen.Space(fs * 0.25f);
          pen.Text(st.regular, small, Col(t.subtext), "1% low");
        }
      }
    }

    if (d.showPing && d.ping.state != PingState::Off) {
      segmentIcon(IconSignal);
      if (d.ping.state == PingState::Ok) {
        const ImVec4& c = d.ping.ms < 50 ? t.good : d.ping.ms < 100 ? t.warn : t.bad;
        char num[16];
        snprintf(num, sizeof(num), "%d", d.ping.ms);
        pen.Text(st.bold, fs, Col(c), num, true, 2);
        pen.Space(fs * 0.25f);
        pen.Text(st.regular, small, Col(t.subtext), "ms");
      } else {
        const char* msg = d.ping.state == PingState::Blocked   ? "server blocks ping"
                          : d.ping.state == PingState::NoGame ? "no game"
                                                               : "finding server";
        pen.Text(st.regular, small, Col(t.subtext), msg);
      }
    }

    if (d.showTime) {
      segmentIcon(IconClock);
      pen.Text(st.regular, fs, Col(t.text), d.time.c_str(), true);
    }

    if (d.showUser && !d.user.empty()) {
      segmentIcon(IconUser);
      pen.Text(st.regular, fs, Col(t.text), d.user.c_str());
    }
    return pen.x + padX - pos.x;
  };

  const float width = std::round(layout(false));
  const ImVec2 size(width, height);
  if (!draw) return size;

  Panel(dl, pos, size, st, radius, 0.0f);
  layout(true);
  return size;
}

void Panel(ImDrawList* dl, ImVec2 pos, ImVec2 size, const WatermarkStyle& st, float radius, float highlight) {
  const Theme& t = *st.theme;
  // Soft shadow (layered, cheap), translucent fill, hairline border and a faint top highlight.
  for (int i = 3; i >= 1; --i) {
    const float e = static_cast<float>(i) * 2.0f;
    dl->AddRectFilled(pos - ImVec2(e, e - 2.0f), pos + size + ImVec2(e, e + 1.0f), IM_COL32(0, 0, 0, 14 + (3 - i) * 10),
                      radius + e);
  }
  dl->AddRectFilled(pos, pos + size, Col(t.crust, st.opacity), radius);
  dl->AddRect(pos, pos + size, Col(Mix(t.overlay, t.accent, highlight), 0.35f + 0.65f * highlight), radius,
              1.0f + highlight);
  dl->AddLine(ImVec2(pos.x + radius, pos.y + 1.0f), ImVec2(pos.x + size.x - radius, pos.y + 1.0f), Col(t.text, 0.07f),
              1.0f);
}
