#include "ui/menu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app_info.h"
#include "input/hotkeys.h"
#include "providers/clock.h"
#include "ui/anim.h"
#include "ui/theme.h"
#include "ui/watermark.h"
#include "ui/widgets.h"

using ui::S;
using ui::T;

namespace {

const char* const kTabs[] = {"Overlay", "Bar", "Music", "Game", "Theme", "Font", "Keybinds", "About"};
constexpr int kTabCount = 8;

// Scales every vertex added to `dl` since `start` around `center`. ImGui has no transforms, so the
// open/close zoom is applied to the finished geometry.
void ScaleVertices(ImDrawList* dl, int start, ImVec2 center, float scale) {
  for (int i = start; i < dl->VtxBuffer.Size; ++i) {
    ImDrawVert& v = dl->VtxBuffer[i];
    v.pos = center + (v.pos - center) * scale;
  }
}

void Gap(float h) { ImGui::Dummy(ImVec2(0, S(h))); }

void SubLabel(const char* text) {
  Gap(6);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
  ImGui::PushStyleColor(ImGuiCol_Text, T().subtext);
  ImGui::TextUnformatted(text);
  ImGui::PopStyleColor();
}

}  // namespace

void Menu::Draw(MenuContext& ctx, float openT, MenuEvents& ev) {
  const ImVec2 display = ImGui::GetIO().DisplaySize;
  const ImVec2 size(S(700), S(480));
  const float sidebarW = S(176);

  ImGui::SetNextWindowPos(display * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(size);
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, openT);
  ImGui::Begin("##qol-menu", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 wp = ImGui::GetWindowPos();
  const ImVec2 center = wp + size * 0.5f;

  // Sidebar
  dl->AddRectFilled(wp, wp + ImVec2(sidebarW, size.y), Col(T().crust), ImGui::GetStyle().WindowRounding,
                    ImDrawFlags_RoundCornersLeft);
  dl->AddLine(wp + ImVec2(sidebarW, 0), wp + ImVec2(sidebarW, size.y), Col(T().overlay, 0.35f));

  ImGui::SetCursorScreenPos(wp + ImVec2(S(22), S(22)));
  ImGui::PushFont(ui::g.bold, ui::g.fontSize * 1.5f * ui::g.scale);
  ImGui::PushStyleColor(ImGuiCol_Text, T().accent);
  ImGui::TextUnformatted(ctx.cfg->logoText.empty() ? "QOL" : ctx.cfg->logoText.c_str());
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::SameLine(0, S(6));
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + S(5));
  ImGui::PushStyleColor(ImGuiCol_Text, T().subtext);
  ImGui::TextUnformatted("overlay");
  ImGui::PopStyleColor();

  const float tabH = S(38), tabTop = wp.y + S(78);
  const float hl = anim::Spring(ImGui::GetID("##tab-hl"), static_cast<float>(tab_), 26.0f);
  const ImVec2 hlMin(wp.x + S(12), tabTop + hl * tabH);
  dl->AddRectFilled(hlMin, hlMin + ImVec2(sidebarW - S(24), tabH - S(4)), Col(T().surface0), S(8));
  dl->AddRectFilled(hlMin + ImVec2(0, S(9)), hlMin + ImVec2(S(3), tabH - S(13)), Col(T().accent), S(2));
  int newTab = tab_;
  for (int i = 0; i < kTabCount; ++i) {
    ImGui::SetCursorScreenPos(ImVec2(wp.x + S(12), tabTop + static_cast<float>(i) * tabH));
    ImGui::PushID(i);
    if (ImGui::InvisibleButton("##tab", ImVec2(sidebarW - S(24), tabH - S(4)))) newTab = i;
    const float hover = anim::Spring(ImGui::GetID("##tab-hover"), ImGui::IsItemHovered() ? 1.0f : 0.0f, 30.0f);
    ImGui::PopID();
    const float sel = std::max(0.0f, 1.0f - std::fabs(hl - static_cast<float>(i)));
    const ImVec2 ts = ImGui::CalcTextSize(kTabs[i]);
    dl->AddText(ImVec2(wp.x + S(28) + S(3) * hover, tabTop + static_cast<float>(i) * tabH + (tabH - S(4) - ts.y) * 0.5f),
                Col(Mix(Mix(T().subtext, T().text, hover * 0.6f), T().text, sel)), kTabs[i]);
  }
  const ImGuiID contentAnim = ImGui::GetID("##content-anim");
  if (newTab != tab_) {
    tab_ = newTab;
    anim::Set(contentAnim, 1.0f);
  }

  // Status at the bottom of the sidebar: what the overlay is tracking right now.
  {
    const char* text;
    ImVec4 color;
    char buf[96];
    if (ctx.etw == EtwStatus::NeedsAdmin) {
      text = "Needs admin for FPS";
      color = T().bad;
    } else if (ctx.etw != EtwStatus::Running) {
      text = "Tracing unavailable";
      color = T().bad;
    } else if (ctx.games->Pid()) {
      snprintf(buf, sizeof(buf), "%s", ctx.games->Exe().c_str());
      text = buf;
      color = T().good;
    } else {
      text = "Waiting for a game";
      color = T().warn;
    }
    const ImVec2 p(wp.x + S(24), wp.y + size.y - S(38));
    dl->AddCircleFilled(p + ImVec2(S(4), ImGui::GetTextLineHeight() * 0.5f), S(4), Col(color));
    dl->PushClipRect(p, ImVec2(wp.x + sidebarW - S(12), p.y + S(30)), true);
    dl->AddText(p + ImVec2(S(14), 0), Col(T().subtext), text);
    dl->PopClipRect();
  }

  // Content: slides up and fades in on tab change.
  const float slide = anim::Spring(contentAnim, 0.0f, 24.0f);
  ImGui::SetCursorScreenPos(wp + ImVec2(sidebarW + S(26), S(24) + slide * S(12)));
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, openT * (1.0f - slide));
  ImGui::BeginChild("##content", ImVec2(size.x - sidebarW - S(46), size.y - S(40)), ImGuiChildFlags_None,
                    ImGuiWindowFlags_NoBackground);
  ImDrawList* contentDl = ImGui::GetWindowDrawList();
  const int contentStart = contentDl->VtxBuffer.Size;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(S(8), S(4)));
  switch (tab_) {
    case 0: TabOverlay(ctx, ev); break;
    case 1: TabBar(ctx, ev); break;
    case 2: TabMusic(ctx, ev); break;
    case 3: TabGame(ctx, ev); break;
    case 4: TabTheme(ctx, ev); break;
    case 5: TabFont(ctx, ev); break;
    case 6: TabKeybinds(ctx, ev); break;
    case 7: TabAbout(ctx, ev); break;
    default: break;
  }
  Gap(12);
  ImGui::PopStyleVar();
  // Fade the bottom edge while there's more to scroll to, instead of a hard cut.
  const float remaining = ImGui::GetScrollMaxY() - ImGui::GetScrollY();
  if (remaining > 1.0f) {  // drawn last in the child's own list so it sits on top of the content
    const ImVec2 cMin = ImGui::GetWindowPos(), cMax = cMin + ImGui::GetWindowSize();
    const float fade = S(28) * std::min(1.0f, remaining / S(28));
    const ImU32 clear = Col(T().base, 0.0f), solid = Col(T().base, openT);
    contentDl->AddRectFilledMultiColor(ImVec2(cMin.x, cMax.y - fade), cMax, clear, clear, solid, solid);
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();

  ImGui::End();
  ImGui::PopStyleVar();

  // Open/close zoom: 0.96 → 1.0 on the window and its child.
  const float scale = 0.96f + 0.04f * openT;
  if (scale < 0.9999f) {
    ScaleVertices(dl, 0, center, scale);
    ScaleVertices(contentDl, contentStart, center, scale);
  }
}

void Menu::TabOverlay(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  ui::Heading("Overlay");
  ui::Note("Where the bar sits and how it looks. Pick what's on it in the Bar tab.");
  Gap(8);

  SubLabel("Position");
  int corner = static_cast<int>(c.corner);
  const char* corners[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
  if (ui::Segmented("corner", &corner, corners, 4)) {
    c.corner = static_cast<Corner>(corner);
    ev.changed = true;
  }
  Gap(8);
  ev.changed |= ui::Slider("Background opacity", &c.opacity, Config::kMinOpacity, 1.0f, "%.2f");

  SubLabel("Formats");
  ev.changed |= ui::Toggle("1% low next to FPS", &c.showLow);
  int fmt = c.clock24h ? 1 : 0;
  const char* clockItems[] = {"12-hour clock", "24-hour clock"};
  if (ui::Segmented("clockfmt", &fmt, clockItems, 2)) {
    c.clock24h = fmt == 1;
    ev.changed = true;
  }
  Gap(2);
  ev.changed |= ui::Toggle("Show seconds", &c.clockSeconds);
  Gap(2);
  if (ui::Segmented("datefmt", &c.dateFormat, kDateFormats, kDateFormatCount)) ev.changed = true;

  Gap(8);
  ev.changed |= ui::TextField("Logo text", &c.logoText, "QOL");
  Gap(6);
  ev.changed |= ui::TextField("Display name", &c.username, "Windows account name");
}

namespace {

enum class Glyph { Up, Down, Remove, Add };

// Small square icon button for list rows. Returns true when clicked.
bool GlyphButton(const char* id, ImVec2 pos, float size, Glyph g, bool enabled) {
  ImGui::SetCursorScreenPos(pos);
  const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size)) && enabled;
  const float hover = anim::Spring(ImGui::GetID(id), enabled && ImGui::IsItemHovered() ? 1.0f : 0.0f, 30.0f);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 c = pos + ImVec2(size, size) * 0.5f;
  if (hover > 0.01f) dl->AddRectFilled(pos, pos + ImVec2(size, size), Col(T().surface1, hover), S(6));
  const ImU32 col = !enabled ? Col(T().subtext, 0.3f)
                    : g == Glyph::Remove ? Col(Mix(T().subtext, T().bad, hover))
                    : g == Glyph::Add    ? Col(T().accent)
                                         : Col(Mix(T().subtext, T().text, hover));
  const float r = size * 0.18f, t = S(1.8f);
  switch (g) {
    case Glyph::Up:
      dl->AddLine(ImVec2(c.x - r, c.y + r * 0.5f), ImVec2(c.x, c.y - r * 0.5f), col, t);
      dl->AddLine(ImVec2(c.x, c.y - r * 0.5f), ImVec2(c.x + r, c.y + r * 0.5f), col, t);
      break;
    case Glyph::Down:
      dl->AddLine(ImVec2(c.x - r, c.y - r * 0.5f), ImVec2(c.x, c.y + r * 0.5f), col, t);
      dl->AddLine(ImVec2(c.x, c.y + r * 0.5f), ImVec2(c.x + r, c.y - r * 0.5f), col, t);
      break;
    case Glyph::Remove:
      dl->AddLine(c - ImVec2(r, r), c + ImVec2(r, r), col, t);
      dl->AddLine(c + ImVec2(-r, r), c + ImVec2(r, -r), col, t);
      break;
    case Glyph::Add:
      dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), col, t);
      dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), col, t);
      break;
  }
  return clicked;
}

}  // namespace

void Menu::TabBar(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  ui::Heading("Bar");
  ui::Note("Add things to the bar and put them in order. The bar updates as you go.");

  SubLabel("On the bar");
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const float w = ImGui::GetContentRegionAvail().x, rowH = S(42), btn = S(28);
  const ImVec2 top = ImGui::GetCursorScreenPos();
  const int n = static_cast<int>(c.segments.size());
  ImGui::Dummy(ImVec2(w, rowH * static_cast<float>(std::max(n, 1))));
  const ImVec2 after = ImGui::GetCursorScreenPos();
  if (n == 0) dl->AddText(top + ImVec2(S(10), S(10)), Col(T().subtext), "Only the logo is showing. Add something below.");

  int moveFrom = -1, moveTo = -1, removeAt = -1;
  for (int i = 0; i < n; ++i) {
    const Seg seg = c.segments[static_cast<size_t>(i)];
    ImGui::PushID(static_cast<int>(seg));
    // Rows glide to their new slot when reordered; a newly added row slides up into place.
    const float target = rowH * static_cast<float>(i);
    const float y = anim::Spring(ImGui::GetID("##y"), target, 22.0f, target + S(14));
    const ImVec2 a = top + ImVec2(0, y), b = a + ImVec2(w, rowH - S(4));
    ImGui::SetCursorScreenPos(a);
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("##row", b - a);
    const float hover = anim::Spring(ImGui::GetID("##hv"), ImGui::IsItemHovered() ? 1.0f : 0.0f, 30.0f);
    dl->AddRectFilled(a, b, Col(T().surface0, 0.45f + 0.35f * hover), S(8));
    const float cy = (a.y + b.y) * 0.5f;
    SegmentIcon(seg, dl, ImVec2(a.x + S(22), cy), S(14), Col(T().accent));
    dl->AddText(ImVec2(a.x + S(42), cy - ImGui::GetTextLineHeight() * 0.5f), Col(T().text), InfoOf(seg).name);

    const float by = cy - btn * 0.5f;
    if (GlyphButton("##rm", ImVec2(b.x - S(6) - btn, by), btn, Glyph::Remove, true)) removeAt = i;
    if (GlyphButton("##dn", ImVec2(b.x - S(8) - btn * 2, by), btn, Glyph::Down, i < n - 1)) {
      moveFrom = i;
      moveTo = i + 1;
    }
    if (GlyphButton("##up", ImVec2(b.x - S(10) - btn * 3, by), btn, Glyph::Up, i > 0)) {
      moveFrom = i;
      moveTo = i - 1;
    }
    ImGui::PopID();
  }
  ImGui::SetCursorScreenPos(after);
  if (moveFrom >= 0) {
    std::swap(c.segments[static_cast<size_t>(moveFrom)], c.segments[static_cast<size_t>(moveTo)]);
    ev.changed = true;
  } else if (removeAt >= 0) {
    c.segments.erase(c.segments.begin() + removeAt);
    ev.changed = true;
  }

  SubLabel("Add to the bar");
  bool any = false;
  for (const SegInfo& info : AllSegments()) {
    if (HasSeg(c.segments, info.id)) continue;
    any = true;
    ImGui::PushID(info.key);
    ImVec2 a, b;
    const float h = S(50);
    if (ui::SelectRow("##add", false, h, &a, &b)) {
      c.segments.push_back(info.id);
      ev.changed = true;
    }
    const float line = ImGui::GetTextLineHeight();
    SegmentIcon(info.id, dl, ImVec2(a.x + S(22), a.y + h * 0.5f), S(14), Col(T().subtext));
    dl->AddText(a + ImVec2(S(42), h * 0.5f - line - S(1)), Col(T().text), info.name);
    dl->AddText(a + ImVec2(S(42), h * 0.5f + S(1)), Col(T().subtext), info.desc);
    const ImVec2 plus(b.x - S(6) - S(28), a.y + (h - S(28)) * 0.5f);
    dl->AddRectFilled(plus, plus + ImVec2(S(28), S(28)), Col(T().surface1, 0.8f), S(6));
    const ImVec2 pc = plus + ImVec2(S(14), S(14));
    dl->AddLine(pc - ImVec2(S(5), 0), pc + ImVec2(S(5), 0), Col(T().accent), S(1.8f));
    dl->AddLine(pc - ImVec2(0, S(5)), pc + ImVec2(0, S(5)), Col(T().accent), S(1.8f));
    ImGui::PopID();
  }
  if (!any) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
    ui::Note("Everything is already on the bar.");
  }
}

void Menu::TabMusic(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  const PlayerData& p = *ctx.player;
  ui::Heading("Music");
  if (!c.showPlayer) ui::Note("The player is off.");
  else if (p.active) {
    std::string now = (p.playing ? "Playing " : "Paused on ") + (p.title.empty() ? std::string("a track") : p.title);
    if (!p.artist.empty()) now += " by " + p.artist;
    ui::Note(now.c_str(), T().text);
  } else {
    ui::Note(c.playerSpotifyOnly ? "Spotify isn't open, or hasn't played anything yet."
                                 : "No app is reporting music right now.");
  }
  Gap(8);
  ev.changed |= ui::Toggle("Show music player", &c.showPlayer);
  if (c.showPlayer) {
    ev.changed |= ui::Toggle("Spotify only", &c.playerSpotifyOnly);
    ev.changed |= ui::Toggle("Hide when nothing is open", &c.playerHideWhenIdle);
    Gap(6);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
    ui::Note("Drag the player to move it while this menu is open. Its buttons work here too; outside the menu the "
             "overlay stays click-through so it never steals a click from your game.");
    Gap(8);
    ImVec2 a, b;
    if (ui::SelectRow("##reset-player", false, S(40), &a, &b)) {
      c.playerX = 0.0f;
      c.playerY = 1.0f;
      ev.changed = true;
    }
    ImGui::GetWindowDrawList()->AddRect(a, b, Col(T().overlay, 0.5f), S(8));
    ImGui::GetWindowDrawList()->AddText(a + ImVec2(S(18), (S(40) - ImGui::GetTextLineHeight()) * 0.5f),
                                        Col(T().text), "Reset position (bottom left)");
  }
  Gap(10);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
  ui::Note("Reads Windows' media controls (the same info as the volume flyout). No Spotify login needed.");
}

void Menu::TabGame(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  const GameSelector& games = *ctx.games;
  ui::Heading("Game");
  if (ctx.etw == EtwStatus::NeedsAdmin) {
    ui::Note("FPS and ping need admin rights. Close the overlay and run it as administrator.", T().bad);
    Gap(8);
  } else if (ctx.etw == EtwStatus::Failed) {
    ui::Note("Windows event tracing couldn't start, so FPS and ping are unavailable.", T().bad);
    Gap(8);
  }
  if (games.Pid()) {
    char buf[128];
    if (ctx.fps.valid)
      snprintf(buf, sizeof(buf), "Tracking %s at %d FPS.", games.Exe().c_str(), static_cast<int>(ctx.fps.fps));
    else
      snprintf(buf, sizeof(buf), "Tracking %s.", games.Exe().c_str());
    ui::Note(buf, T().text);
  } else {
    ui::Note("No game detected. Start a game in borderless or windowed mode; exclusive fullscreen hides every overlay.");
  }
  Gap(10);

  ImVec2 a, b;
  const float rowH = S(50);
  const float line = ImGui::GetTextLineHeight();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  if (ui::SelectRow("##auto", c.pinnedExe.empty(), rowH, &a, &b) && !c.pinnedExe.empty()) {
    c.pinnedExe.clear();
    ev.changed = true;
  }
  dl->AddText(a + ImVec2(S(18), S(8)), Col(T().text), "Detect automatically");
  dl->AddText(a + ImVec2(S(18), S(10) + line), Col(T().subtext), "Follows the game in the foreground");

  SubLabel("Or pick a running game");
  std::vector<GameEntry> list = games.Candidates();
  bool pinnedRunning = false;
  for (const GameEntry& g : list) pinnedRunning |= _stricmp(g.exe.c_str(), c.pinnedExe.c_str()) == 0;
  if (!c.pinnedExe.empty() && !pinnedRunning) list.insert(list.begin(), GameEntry{0, c.pinnedExe, -1.0f});
  if (list.empty()) {
    Gap(4);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
    ui::Note("Nothing is drawing frames right now.");
  }
  for (const GameEntry& g : list) {
    ImGui::PushID(g.exe.c_str());
    const bool pinned = _stricmp(g.exe.c_str(), c.pinnedExe.c_str()) == 0;
    if (ui::SelectRow("##game", pinned, S(40), &a, &b) && !pinned) {
      c.pinnedExe = g.exe;
      ev.changed = true;
    }
    dl->AddText(a + ImVec2(S(18), (S(40) - line) * 0.5f), Col(T().text), g.exe.c_str());
    char fps[32];
    if (g.fps < 0) snprintf(fps, sizeof(fps), "not running");
    else snprintf(fps, sizeof(fps), "%d FPS", static_cast<int>(g.fps));
    const ImVec2 fs = ImGui::CalcTextSize(fps);
    dl->AddText(ImVec2(b.x - S(14) - fs.x, a.y + (S(40) - line) * 0.5f), Col(T().subtext), fps);
    ImGui::PopID();
  }

  if (HasSeg(c.segments, Seg::Ping) && ctx.ping.state != PingState::Off) {
    SubLabel("Ping");
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
    char buf[160];
    switch (ctx.ping.state) {
      case PingState::Ok:
        snprintf(buf, sizeof(buf), "%d ms to %s (the game's server or relay).", ctx.ping.ms, ctx.ping.endpoint.c_str());
        break;
      case PingState::Blocked:
        snprintf(buf, sizeof(buf), "%s doesn't answer ping requests, so latency can't be measured from outside the game.",
                 ctx.ping.endpoint.c_str());
        break;
      case PingState::Searching: snprintf(buf, sizeof(buf), "Looking for the game's server..."); break;
      default: snprintf(buf, sizeof(buf), "Starts when a game is detected."); break;
    }
    ui::Note(buf);
  }
}

void Menu::TabTheme(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  ui::Heading("Theme");
  ui::Note("Colors for the watermark and this menu.");
  Gap(10);

  const auto& themes = Themes();
  const int cols = 3;
  const float gap = S(10);
  const float w = (ImGui::GetContentRegionAvail().x - gap * (cols - 1)) / cols, h = S(86);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  for (size_t i = 0; i < themes.size(); ++i) {
    const Theme& th = themes[i];
    if (i % cols) ImGui::SameLine(0, gap);
    ImGui::PushID(static_cast<int>(i));
    const ImVec2 p = ImGui::GetCursorScreenPos();
    if (ImGui::InvisibleButton("##theme", ImVec2(w, h)) && c.theme != th.name) {
      c.theme = th.name;
      ev.changed = true;
    }
    const float hover = anim::Spring(ImGui::GetID("##h"), ImGui::IsItemHovered() ? 1.0f : 0.0f, 28.0f);
    const float sel = anim::Spring(ImGui::GetID("##s"), c.theme == th.name ? 1.0f : 0.0f, 22.0f);
    ImGui::PopID();

    const ImVec2 lift(0, -S(2) * hover);
    const ImVec2 a = p + lift, b = p + ImVec2(w, h) + lift;
    dl->AddRectFilled(a + ImVec2(0, S(3)), b + ImVec2(0, S(3)), IM_COL32(0, 0, 0, static_cast<int>(40 * hover)), S(10));
    dl->AddRectFilled(a, b, Col(th.base), S(10));
    dl->AddRectFilled(a, ImVec2(b.x, a.y + S(34)), Col(th.crust), S(10), ImDrawFlags_RoundCornersTop);
    // Mini watermark pill in the theme's own colors.
    const ImVec2 pill(a.x + S(10), a.y + S(9));
    dl->AddRectFilled(pill, pill + ImVec2(w - S(20), S(16)), Col(th.surface0), S(5));
    dl->AddRectFilled(pill + ImVec2(S(6), S(5)), pill + ImVec2(S(22), S(11)), Col(th.accent), S(2));
    dl->AddRectFilled(pill + ImVec2(S(28), S(5)), pill + ImVec2(S(44), S(11)), Col(th.good), S(2));
    dl->AddRectFilled(pill + ImVec2(S(50), S(6)), pill + ImVec2(w - S(28), S(10)), Col(th.subtext, 0.6f), S(2));
    dl->AddText(ui::g.regular, ui::g.fontSize * 0.95f * ui::g.scale, ImVec2(a.x + S(10), a.y + S(42)), Col(th.text),
                th.name);
    const ImVec4* dots[] = {&th.accent, &th.good, &th.warn, &th.bad};
    for (int d = 0; d < 4; ++d)
      dl->AddCircleFilled(ImVec2(a.x + S(15) + S(14) * static_cast<float>(d), b.y - S(14)), S(4.5f), Col(*dots[d]));
    const ImVec4 border = Mix(th.overlay, T().accent, sel);
    dl->AddRect(a, b, Col(border, 0.5f + 0.5f * sel), S(10), S(1) + S(1) * sel);
  }
}

void Menu::TabFont(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  ui::Heading("Font");
  ui::Note("Used by the watermark and this menu. Numbers keep a fixed width so they don't jitter.");
  Gap(8);
  ev.changed |= ui::Slider("Watermark text size", &c.fontSize, 12.0f, 22.0f, "%.0f px");
  Gap(8);

  ImDrawList* dl = ImGui::GetWindowDrawList();
  const float rowH = S(58);
  for (FontFace& f : ctx.fonts->Faces()) {
    ImGui::PushID(f.name);
    ImVec2 a, b;
    const bool selected = c.font == f.name;
    if (ui::SelectRow("##font", selected, rowH, &a, &b) && !selected) {
      c.font = f.name;
      ev.changed = true;
    }
    const float nameSize = ui::g.fontSize * 1.05f * ui::g.scale;
    const float previewSize = ui::g.fontSize * 0.95f * ui::g.scale;
    dl->AddText(f.bold, nameSize, a + ImVec2(S(18), S(9)), Col(T().text), f.name);
    dl->AddText(f.medium, previewSize, a + ImVec2(S(18), S(13) + nameSize), Col(T().subtext),
                "219 FPS   167 1% low   23 ms   11:44 AM");
    ImGui::PopID();
  }
}

void Menu::TabKeybinds(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  ui::Heading("Keybinds");
  ui::Note("Click a bind, then press the new key. Modifiers (Ctrl, Alt, Shift) work. Esc cancels.");
  Gap(10);
  auto bind = [&](const char* label, const Hotkey& key, Capture which) {
    if (!ui::KeybindButton(label, HotkeyName(key), ctx.capture == which)) return;
    if (ctx.capture == which) ev.cancelCapture = true;  // clicking the waiting bind again cancels
    else ev.startCapture = which;
  };
  bind("Open this menu", c.menuKey, Capture::MenuKey);
  Gap(4);
  bind("Show / hide overlay", c.toggleKey, Capture::ToggleKey);
  if (ctx.capture != Capture::None && ev.startCapture == Capture::None && ImGui::IsMouseClicked(0) &&
      !ImGui::IsAnyItemHovered())
    ev.cancelCapture = true;
  if (!ctx.keyError.empty()) {
    Gap(8);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
    ui::Note(ctx.keyError.c_str(), T().bad);
  }
  Gap(14);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
  ui::Note("Binds are registered with Windows as normal hotkeys. Nothing hooks your keyboard.");
}

void Menu::TabAbout(MenuContext& ctx, MenuEvents& ev) {
  Config& c = *ctx.cfg;
  ui::Heading("About");
  ui::Note("QOL Overlay " QOL_VERSION, T().text);
  Gap(10);
  ev.changed |= ui::Toggle("Install updates automatically", &c.autoUpdate);
  Gap(4);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
  ui::Note("Checks GitHub every few hours. Updates download in the background, are checked against their published "
           "SHA-256, and only install while no game is running. The overlay restarts by itself afterwards.");
  Gap(10);

  const UpdateStatus& u = ctx.update;
  std::string status;
  ImVec4 color = T().subtext;
  switch (u.state) {
    case UpdateState::Idle: status = "Hasn't checked yet."; break;
    case UpdateState::Checking: status = "Checking for updates..."; break;
    case UpdateState::UpToDate: status = "You're on the latest version."; color = T().good; break;
    case UpdateState::Downloading: status = "Downloading " + u.latest + "..."; break;
    case UpdateState::Ready:
      status = u.latest + " is ready. " +
               (c.autoUpdate ? std::string("It installs next time no game is running.") : std::string("Install it below."));
      color = T().accent;
      break;
    case UpdateState::Failed: status = "Update check failed: " + u.error; color = T().warn; break;
  }
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + S(10));
  ui::Note(status.c_str(), color);
  Gap(8);

  const bool busy = u.state == UpdateState::Checking || u.state == UpdateState::Downloading;
  const bool ready = u.state == UpdateState::Ready;
  ImVec2 a, b;
  if (ui::SelectRow("##update", false, S(40), &a, &b) && !busy) {
    if (ready) ev.installUpdate = true;
    else ev.checkUpdates = true;
  }
  const std::string label = ready ? "Install " + u.latest + " now" : busy ? "Working..." : "Check for updates";
  ImGui::GetWindowDrawList()->AddRect(a, b, Col(ready ? T().accent : T().overlay, ready ? 0.9f : 0.5f), S(8));
  ImGui::GetWindowDrawList()->AddText(a + ImVec2(S(18), (S(40) - ImGui::GetTextLineHeight()) * 0.5f),
                                      Col(busy ? T().subtext : T().text), label.c_str());
}
