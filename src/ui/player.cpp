#include "ui/player.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "ui/anim.h"

std::string PlayerData::Signature() const {
  char buf[64];
  snprintf(buf, sizeof(buf), "%d%d|%d|%d|%llu|", active, playing, static_cast<int>(position),
           static_cast<int>(duration), static_cast<unsigned long long>(art));
  return buf + title + '|' + artist;
}

namespace {

struct Metrics {
  float fs, w, h, pad, art, btn, gap;
};

Metrics Measure(float fs) {
  Metrics m;
  m.fs = fs;
  m.h = std::round(fs * 4.4f);
  m.w = std::round(fs * 21.0f);
  m.pad = std::round(fs * 0.6f);
  m.art = m.h - m.pad * 2;
  m.btn = std::round(fs * 1.5f);
  m.gap = std::round(fs * 0.15f);
  return m;
}

std::string FormatTime(float sec) {
  const int s = std::max(0, static_cast<int>(sec));
  char buf[16];
  if (s >= 3600) snprintf(buf, sizeof(buf), "%d:%02d:%02d", s / 3600, (s / 60) % 60, s % 60);
  else snprintf(buf, sizeof(buf), "%d:%02d", s / 60, s % 60);
  return buf;
}

// Trims to fit with "..." — no marquee, because scrolling text would need constant redraws.
std::string Fit(ImFont* font, float size, const std::string& text, float maxW) {
  if (font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str()).x <= maxW) return text;
  const float dots = font->CalcTextSizeA(size, FLT_MAX, 0.0f, "...").x;
  std::string s = text;
  while (!s.empty() && font->CalcTextSizeA(size, FLT_MAX, 0.0f, s.c_str()).x + dots > maxW) {
    s.pop_back();
    while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) s.pop_back();  // whole UTF-8 chars
  }
  return s + "...";
}

void IconPrev(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
  dl->AddRectFilled(ImVec2(c.x - s * 0.38f, c.y - s * 0.32f), ImVec2(c.x - s * 0.24f, c.y + s * 0.32f), col, 1.0f);
  dl->AddTriangleFilled(ImVec2(c.x - s * 0.22f, c.y), ImVec2(c.x + s * 0.36f, c.y - s * 0.34f),
                        ImVec2(c.x + s * 0.36f, c.y + s * 0.34f), col);
}

void IconNext(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
  dl->AddTriangleFilled(ImVec2(c.x + s * 0.22f, c.y), ImVec2(c.x - s * 0.36f, c.y + s * 0.34f),
                        ImVec2(c.x - s * 0.36f, c.y - s * 0.34f), col);
  dl->AddRectFilled(ImVec2(c.x + s * 0.24f, c.y - s * 0.32f), ImVec2(c.x + s * 0.38f, c.y + s * 0.32f), col, 1.0f);
}

void IconPlayPause(ImDrawList* dl, ImVec2 c, float s, ImU32 col, bool playing) {
  if (playing) {
    dl->AddRectFilled(ImVec2(c.x - s * 0.26f, c.y - s * 0.3f), ImVec2(c.x - s * 0.07f, c.y + s * 0.3f), col, 1.0f);
    dl->AddRectFilled(ImVec2(c.x + s * 0.07f, c.y - s * 0.3f), ImVec2(c.x + s * 0.26f, c.y + s * 0.3f), col, 1.0f);
  } else {
    dl->AddTriangleFilled(ImVec2(c.x - s * 0.2f, c.y - s * 0.34f), ImVec2(c.x + s * 0.34f, c.y),
                          ImVec2(c.x - s * 0.2f, c.y + s * 0.34f), col);
  }
}

void IconNote(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
  const float r = s * 0.16f;
  dl->AddCircleFilled(ImVec2(c.x - s * 0.22f, c.y + s * 0.26f), r, col);
  dl->AddCircleFilled(ImVec2(c.x + s * 0.26f, c.y + s * 0.18f), r, col);
  dl->AddRectFilled(ImVec2(c.x - s * 0.22f + r - 2.0f, c.y - s * 0.34f), ImVec2(c.x - s * 0.22f + r, c.y + s * 0.26f),
                    col);
  dl->AddRectFilled(ImVec2(c.x + s * 0.26f + r - 2.0f, c.y - s * 0.42f), ImVec2(c.x + s * 0.26f + r, c.y + s * 0.18f),
                    col);
  dl->AddQuadFilled(ImVec2(c.x - s * 0.22f + r - 2.0f, c.y - s * 0.34f), ImVec2(c.x + s * 0.26f + r, c.y - s * 0.42f),
                    ImVec2(c.x + s * 0.26f + r, c.y - s * 0.28f), ImVec2(c.x - s * 0.22f + r - 2.0f, c.y - s * 0.2f),
                    col);
}

struct Hover {
  float card = 0, prev = 0, play = 0, next = 0;
};

// Button centers, right-aligned on the title row: prev, play/pause, next.
void ButtonCenters(const Metrics& m, ImVec2 pos, ImVec2 out[3]) {
  const float cy = pos.y + m.pad + m.btn * 0.5f - m.fs * 0.1f;
  float cx = pos.x + m.w - m.pad - m.btn * 0.5f;
  for (int i = 2; i >= 0; --i) {
    out[i] = ImVec2(cx, cy);
    cx -= m.btn + m.gap;
  }
}

void Draw(ImDrawList* dl, ImVec2 pos, const PlayerData& d, const WatermarkStyle& st, const Metrics& m, const Hover& h) {
  const Theme& t = *st.theme;
  const float radius = std::round(m.fs * 0.55f);
  Panel(dl, pos, ImVec2(m.w, m.h), st, radius, h.card);

  // Cover art (or a placeholder note).
  const ImVec2 a0(pos.x + m.pad, pos.y + m.pad), a1 = a0 + ImVec2(m.art, m.art);
  if (d.art != ImTextureID_Invalid) {
    dl->AddImageRounded(ImTextureRef(d.art), a0, a1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, radius * 0.7f);
  } else {
    dl->AddRectFilled(a0, a1, Col(t.surface0), radius * 0.7f);
    IconNote(dl, (a0 + a1) * 0.5f, m.art * 0.5f, Col(t.subtext, 0.8f));
  }

  const float x0 = a1.x + std::round(m.fs * 0.75f), x1 = pos.x + m.w - m.pad;
  ImVec2 btn[3];
  ButtonCenters(m, pos, btn);
  const float titleMax = btn[0].x - m.btn * 0.5f - m.fs * 0.4f - x0;

  const float titleSize = m.fs, subSize = std::round(m.fs * 0.82f);
  const float row1 = pos.y + m.pad + m.fs * 0.05f;
  const float row2 = row1 + titleSize + m.fs * 0.3f;

  if (!d.active) {
    dl->AddText(st.bold, titleSize, ImVec2(x0, row1), Col(t.text), Fit(st.bold, titleSize, "Spotify isn't open", titleMax).c_str());
    dl->AddText(st.regular, subSize, ImVec2(x0, row2), Col(t.subtext),
                Fit(st.regular, subSize, "Play something and it shows up here", x1 - x0).c_str());
  } else {
    dl->AddText(st.bold, titleSize, ImVec2(x0, row1), Col(t.text),
                Fit(st.bold, titleSize, d.title.empty() ? "Unknown track" : d.title, titleMax).c_str());
    const std::string time = FormatTime(d.position) + " / " + FormatTime(d.duration);
    const float timeW = st.regular->CalcTextSizeA(subSize, FLT_MAX, 0.0f, time.c_str()).x;
    if (d.duration > 0) dl->AddText(st.regular, subSize, ImVec2(x1 - timeW, row2), Col(t.subtext), time.c_str());
    dl->AddText(st.regular, subSize, ImVec2(x0, row2), Col(t.subtext),
                Fit(st.regular, subSize, d.artist, x1 - x0 - (d.duration > 0 ? timeW + m.fs * 0.6f : 0.0f)).c_str());

    // Progress
    const float by = pos.y + m.h - m.pad - 3.0f;
    const float frac = d.duration > 0 ? std::clamp(d.position / d.duration, 0.0f, 1.0f) : 0.0f;
    dl->AddRectFilled(ImVec2(x0, by), ImVec2(x1, by + 3.0f), Col(t.surface1), 1.5f);
    if (frac > 0) dl->AddRectFilled(ImVec2(x0, by), ImVec2(x0 + (x1 - x0) * frac, by + 3.0f), Col(t.accent), 1.5f);
  }

  // Transport buttons. Play/pause is the filled one; hover (menu open) turns it accent.
  const float hs[3] = {h.prev, h.play, h.next};
  for (int i = 0; i < 3; ++i) {
    const float r = m.btn * 0.5f;
    if (i == 1) dl->AddCircleFilled(btn[i], r, Col(Mix(t.surface1, t.accent, hs[i])));
    else if (hs[i] > 0.01f) dl->AddCircleFilled(btn[i], r, Col(t.surface0, hs[i]));
  }
  const ImU32 idle = Col(d.active ? t.text : t.subtext);
  IconPrev(dl, btn[0], m.btn * 0.5f, idle);
  IconPlayPause(dl, btn[1], m.btn * 0.55f, Col(Mix(d.active ? t.text : t.subtext, t.crust, h.play)), d.playing);
  IconNext(dl, btn[2], m.btn * 0.5f, idle);
}

}  // namespace

ImVec2 PlayerSize(float fontSize) {
  const Metrics m = Measure(fontSize);
  return ImVec2(m.w, m.h);
}

void Player(ImVec2* pos, const PlayerData& d, const WatermarkStyle& st, bool interactive, ImVec2 boundsMin,
            ImVec2 boundsMax, PlayerEvents* ev) {
  const Metrics m = Measure(st.size);
  if (!interactive) {
    Draw(ImGui::GetBackgroundDrawList(), *pos, d, st, m, Hover{});
    return;
  }

  ImGui::SetNextWindowPos(*pos);
  ImGui::SetNextWindowSize(ImVec2(m.w, m.h));
  ImGui::Begin("##player", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImGuiIO& io = ImGui::GetIO();

  // Whole card = drag handle; buttons sit on top of it.
  ImGui::SetCursorScreenPos(*pos);
  ImGui::SetNextItemAllowOverlap();
  ImGui::InvisibleButton("##drag", ImVec2(m.w, m.h));
  const bool dragging = ImGui::IsItemActive();
  const bool cardHovered = ImGui::IsItemHovered();
  if (dragging && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)) {
    ImVec2 p = *pos + io.MouseDelta;
    p.x = std::clamp(p.x, boundsMin.x, std::max(boundsMin.x, boundsMax.x - m.w));
    p.y = std::clamp(p.y, boundsMin.y, std::max(boundsMin.y, boundsMax.y - m.h));
    *pos = p;
    ev->moved = true;
  }
  if (ImGui::IsItemDeactivated()) ev->released = true;

  ImVec2 btn[3];
  ButtonCenters(m, *pos, btn);
  const MediaCommand cmds[3] = {MediaCommand::Previous, MediaCommand::PlayPause, MediaCommand::Next};
  float hover[3];
  bool anyButtonHovered = false;
  for (int i = 0; i < 3; ++i) {
    ImGui::PushID(i);
    ImGui::SetCursorScreenPos(btn[i] - ImVec2(m.btn, m.btn) * 0.5f);
    if (ImGui::InvisibleButton("##btn", ImVec2(m.btn, m.btn)) && d.active) ev->command = cmds[i];
    const bool hv = ImGui::IsItemHovered() && d.active;
    anyButtonHovered |= hv;
    hover[i] = anim::Spring(ImGui::GetID("##hv"), hv ? 1.0f : 0.0f, 30.0f);
    ImGui::PopID();
  }
  Hover h;
  h.card = anim::Spring(ImGui::GetID("##card"), (cardHovered || dragging) && !anyButtonHovered ? 1.0f : 0.0f, 26.0f);
  h.prev = hover[0];
  h.play = hover[1];
  h.next = hover[2];
  if ((cardHovered || dragging) && !anyButtonHovered) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  else if (anyButtonHovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

  Draw(dl, *pos, d, st, m, h);
  ImGui::End();
}
