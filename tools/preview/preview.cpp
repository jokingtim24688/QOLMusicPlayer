// Offline preview: renders the real watermark + menu code with a tiny software rasterizer and writes PPM images.
// Lets the UI be checked without Windows. Build: see tools/preview/CMakeLists.txt.
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#define private public  // preview only: lets us fill GameSelector with fake running games
#include "providers/game_select.h"
#undef private
#include "config/config.h"
#include "input/hotkeys.h"
#include "ui/anim.h"
#include "ui/fonts.h"
#include "ui/menu.h"
#include "ui/player.h"
#include "ui/theme.h"
#include "ui/watermark.h"
#include "ui/widgets.h"

// --- stand-ins for the Windows-only pieces the UI links against ---
std::string HotkeyName(const Hotkey& k) { return k.vk == 0x2D ? "Insert" : k.vk == 0x23 ? "End" : "Key"; }

static std::vector<std::vector<char>> g_fontData;
static ImFont* LoadFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  g_fontData.emplace_back((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  ImFontConfig cfg;
  cfg.FontDataOwnedByAtlas = false;
  auto& d = g_fontData.back();
  return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(d.data(), static_cast<int>(d.size()), 16.0f, &cfg);
}
void Fonts::Load() {
  const std::string dir = FONT_DIR "/";
  faces_ = {{"Geist", 0, 0}, {"Geist Mono", 0, 0}, {"JetBrains Mono", 0, 0},
            {"IBM Plex Sans", 0, 0}, {"Space Grotesk", 0, 0}, {"Manrope", 0, 0}};
  const char* files[][2] = {{"Geist-Medium", "Geist-Bold"},         {"GeistMono-Medium", "GeistMono-Bold"},
                            {"JetBrainsMono-Medium", "JetBrainsMono-Bold"}, {"IBMPlexSans-Medium", "IBMPlexSans-Bold"},
                            {"SpaceGrotesk-Medium", "SpaceGrotesk-Bold"}, {"Manrope-Medium", "Manrope-Bold"}};
  g_fontData.reserve(32);
  for (size_t i = 0; i < faces_.size(); ++i) {
    faces_[i].medium = LoadFile(dir + files[i][0] + ".ttf");
    faces_[i].bold = LoadFile(dir + files[i][1] + ".ttf");
  }
}
const FontFace& Fonts::Find(const std::string& name) const {
  for (const FontFace& f : faces_)
    if (name == f.name) return f;
  return faces_.front();
}

// --- software rasterizer ---
struct Canvas {
  int w, h;
  std::vector<float> px;  // RGB
  Canvas(int w_, int h_) : w(w_), h(h_), px(static_cast<size_t>(w_ * h_ * 3)) {}

  // A stand-in "game frame": dark scene with a bright sky band, so contrast problems show up.
  void Background() {
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x) {
        float t = static_cast<float>(y) / static_cast<float>(h);
        float sky = std::max(0.0f, 1.0f - t * 3.0f);
        float* p = &px[static_cast<size_t>((y * w + x) * 3)];
        p[0] = 0.10f + 0.75f * sky + 0.05f * std::sin(x * 0.02f);
        p[1] = 0.11f + 0.72f * sky;
        p[2] = 0.13f + 0.60f * sky + 0.1f * t;
      }
  }

  void Draw(ImDrawData* dd) {
    for (ImDrawList* cl : dd->CmdLists) {
      for (const ImDrawCmd& cmd : cl->CmdBuffer) {
        if (cmd.UserCallback) continue;
        auto* tex = reinterpret_cast<ImTextureData*>(cmd.GetTexID());
        const int cx0 = std::max(0, static_cast<int>(cmd.ClipRect.x)), cy0 = std::max(0, static_cast<int>(cmd.ClipRect.y));
        const int cx1 = std::min(w, static_cast<int>(std::ceil(cmd.ClipRect.z)));
        const int cy1 = std::min(h, static_cast<int>(std::ceil(cmd.ClipRect.w)));
        for (unsigned i = 0; i < cmd.ElemCount; i += 3) {
          const ImDrawVert* v[3];
          for (int k = 0; k < 3; ++k) v[k] = &cl->VtxBuffer[cmd.VtxOffset + cl->IdxBuffer[cmd.IdxOffset + i + k]];
          Tri(v, tex, cx0, cy0, cx1, cy1);
        }
      }
    }
  }

  void Tri(const ImDrawVert* v[3], ImTextureData* tex, int cx0, int cy0, int cx1, int cy1) {
    const ImVec2 a = v[0]->pos, b = v[1]->pos, c = v[2]->pos;
    const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::fabs(area) < 1e-8f) return;
    int x0 = std::max(cx0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
    int x1 = std::min(cx1, static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
    int y0 = std::max(cy0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
    int y1 = std::min(cy1, static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));
    for (int y = y0; y < y1; ++y)
      for (int x = x0; x < x1; ++x) {
        const float px_ = x + 0.5f, py = y + 0.5f;
        float w0 = ((b.x - px_) * (c.y - py) - (b.y - py) * (c.x - px_)) / area;
        float w1 = ((c.x - px_) * (a.y - py) - (c.y - py) * (a.x - px_)) / area;
        float w2 = 1.0f - w0 - w1;
        if (w0 < -1e-4f || w1 < -1e-4f || w2 < -1e-4f) continue;
        float col[4];
        for (int k = 0; k < 4; ++k) {
          auto ch = [&](const ImDrawVert* vv) { return ((vv->col >> (8 * k)) & 0xFF) / 255.0f; };
          col[k] = ch(v[0]) * w0 + ch(v[1]) * w1 + ch(v[2]) * w2;
        }
        if (tex && tex->Pixels) {
          float u = v[0]->uv.x * w0 + v[1]->uv.x * w1 + v[2]->uv.x * w2;
          float t = v[0]->uv.y * w0 + v[1]->uv.y * w1 + v[2]->uv.y * w2;
          int tx = std::clamp(static_cast<int>(u * tex->Width), 0, tex->Width - 1);
          int ty = std::clamp(static_cast<int>(t * tex->Height), 0, tex->Height - 1);
          if (tex->Format == ImTextureFormat_RGBA32) {
            const unsigned char* s = tex->Pixels + (ty * tex->Width + tx) * 4;
            for (int k = 0; k < 4; ++k) col[k] *= s[k] / 255.0f;
          } else {
            col[3] *= tex->Pixels[ty * tex->Width + tx] / 255.0f;
          }
        }
        float* p = &px[static_cast<size_t>((y * w + x) * 3)];
        for (int k = 0; k < 3; ++k) p[k] = col[k] * col[3] + p[k] * (1.0f - col[3]);
      }
  }

  void Save(const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb");
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (float v : px) fputc(static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f), f);
    fclose(f);
  }
};

static void ServiceTextures() {
  for (ImTextureData* tex : ImGui::GetPlatformIO().Textures) {
    if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates) {
      tex->SetTexID(reinterpret_cast<ImTextureID>(tex));
      tex->SetStatus(ImTextureStatus_OK);
    } else if (tex->Status == ImTextureStatus_WantDestroy) {
      tex->SetTexID(ImTextureID_Invalid);
      tex->SetStatus(ImTextureStatus_Destroyed);
    }
  }
}

static WatermarkData SampleData(const Config& c) {
  WatermarkData d;
  d.logo = c.logoText;
  d.fps = {true, 219.0f, 167.0f};
  d.refreshHz = 240.0f;
  d.ping = {PingState::Ok, 23, "155.133.252.10"};
  d.time = "11:44 AM";
  d.date = "Sat, Sep 26";
  d.user = "jokingtim";
  d.game = "Counter-Strike 2";
  d.session = "1:12:08";
  d.cpu = 34;
  d.gpu = 59;
  d.ram = 48;
  d.segments = c.segments;
  d.showLow = c.showLow;
  return d;
}

// Fake album cover: a warm diagonal gradient with a soft circle, as an RGBA ImTextureData the rasterizer can sample.
static ImTextureData* FakeCover() {
  auto* tex = new ImTextureData();
  tex->Create(ImTextureFormat_RGBA32, 64, 64);
  for (int y = 0; y < 64; ++y)
    for (int x = 0; x < 64; ++x) {
      auto* p = static_cast<unsigned char*>(tex->GetPixelsAt(x, y));
      float t = (x + y) / 126.0f, d = std::hypot(x - 40.0f, y - 24.0f);
      float glow = std::max(0.0f, 1.0f - d / 22.0f);
      p[0] = static_cast<unsigned char>(std::min(255.0f, 40 + 200 * t + 60 * glow));
      p[1] = static_cast<unsigned char>(std::min(255.0f, 30 + 70 * t + 90 * glow));
      p[2] = static_cast<unsigned char>(std::min(255.0f, 90 - 40 * t + 40 * glow));
      p[3] = 255;
    }
  tex->SetTexID(reinterpret_cast<ImTextureID>(tex));
  tex->SetStatus(ImTextureStatus_OK);
  return tex;
}

static PlayerData SamplePlayer(ImTextureID art) {
  PlayerData p;
  p.active = true;
  p.playing = true;
  p.title = "Midnight City";
  p.artist = "M83";
  p.position = 83;
  p.duration = 243;
  p.art = art;
  return p;
}

int main(int argc, char** argv) {
  const std::string out = argc > 1 ? argv[1] : ".";
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
  Fonts fonts;
  fonts.Load();

  const ImTextureID cover = reinterpret_cast<ImTextureID>(FakeCover());
  GameSelector games;
  games.pid_ = 4242;
  games.exe_ = "cs2.exe";
  games.candidates_ = {{4242, "cs2.exe", 219.0f}, {5120, "VALORANT-Win64-Shipping.exe", 144.0f}};

  auto frame = [&](int w, int h, auto&& body) {
    io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
    io.DeltaTime = 1.0f / 60.0f;
    io.AddMousePosEvent(-1000, -1000);
    ImGui::NewFrame();
    anim::BeginFrame(io.DeltaTime);
    body();
    ImGui::Render();
    ServiceTextures();
  };

  // 0. Interaction regression test: pressing a tab or a player button must not count as a click outside
  //    the menu (that bug closed the menu on every button press), and the buttons must fire.
  {
    Config c;
    const Theme& theme = FindTheme(c.theme);
    Menu menu;
    const PlayerData player = SamplePlayer(cover);
    const ImVec2 display(1100, 640);
    bool outside = false;
    MediaCommand command = MediaCommand::None;
    auto step = [&](ImVec2 mouse, int button) {  // button: -1 = no change, 0 = release, 1 = press
      io.AddMousePosEvent(mouse.x, mouse.y);
      if (button >= 0) io.AddMouseButtonEvent(0, button == 1);
      io.DisplaySize = display;
      io.DeltaTime = 1.0f / 60.0f;
      ImGui::NewFrame();
      anim::BeginFrame(io.DeltaTime);
      const FontFace& face = fonts.Find(c.font);
      ApplyImGuiStyle(theme, 1.0f);
      ui::g = ui::Context{&theme, 1.0f, face.medium, face.bold, 15.0f};
      ImGui::PushFont(face.medium, 15.0f);
      WatermarkStyle ws{&theme, face.medium, face.bold, 15.0f, c.opacity};
      ImVec2 ppos(20, 6);  // clear of the menu window
      PlayerEvents pev;
      Player(&ppos, player, ws, true, ImVec2(0, 0), display, &pev);
      if (pev.command != MediaCommand::None) command = pev.command;
      MenuContext ctx{&c, &fonts, &games, {}, {}, EtwStatus::Running, Capture::None, "", &player, {}};
      MenuEvents ev;
      menu.Draw(ctx, 1.0f, ev);
      outside |= ui::ClickedOutsideWindows();
      ImGui::PopFont();
      ImGui::Render();
      ServiceTextures();
    };
    auto click = [&](ImVec2 p) {
      for (int i = 0; i < 3; ++i) step(p, -1);
      step(p, 1);
      step(p, -1);
      step(p, 0);
      for (int i = 0; i < 3; ++i) step(p, -1);
    };
    for (int i = 0; i < 5; ++i) step(ImVec2(-100, -100), -1);  // let windows settle

    // Menu is 700x480 centered; sidebar tab i is at y = top + 78 + i*38 (+ half a row).
    const ImVec2 menuPos = (display - ImVec2(700, 480)) * 0.5f;
    click(menuPos + ImVec2(88, 78 + 2 * 38 + 17));  // "Music"
    const bool tabOk = menu.Tab() == 2 && !outside;
    printf("tab click:    tab=%d outside=%d -> %s\n", menu.Tab(), outside, tabOk ? "OK" : "FAIL");

    outside = false;
    // Play button: second of the three buttons on the card's title row.
    const float m = 15.0f, w = std::round(m * 21), pad = std::round(m * 0.6f), btn = std::round(m * 1.5f);
    const float gap = std::round(m * 0.15f);
    const ImVec2 card(20, 6);
    const ImVec2 play(card.x + w - pad - btn * 0.5f - (btn + gap), card.y + pad + btn * 0.5f - m * 0.1f);
    click(play);
    const bool playOk = command == MediaCommand::PlayPause && !outside;
    printf("player click: command=%d outside=%d -> %s\n", static_cast<int>(command), outside, playOk ? "OK" : "FAIL");

    outside = false;
    click(ImVec2(1050, 40));  // empty space
    printf("empty click:  outside=%d -> %s\n", outside, outside ? "OK" : "FAIL");
    if (!tabOk || !playOk || !outside) return 1;
  }

  // 1. Watermark in several themes over a bright/dark "game" background.
  {
    const char* themes[] = {"Neverlose", "Catppuccin Mocha", "Midnight", "Catppuccin Latte", "Nord", "Gruvbox Dark"};
    Canvas cv(1240, 620);
    for (int f = 0; f < 3; ++f) {
      frame(cv.w, cv.h, [&] {
        float y = 16;
        for (const char* tn : themes) {
          Config c;
          c.theme = tn;
          const FontFace& face = fonts.Find(c.font);
          WatermarkStyle ws{&FindTheme(tn), face.medium, face.bold, 15.0f, c.opacity};
          Watermark(ImGui::GetBackgroundDrawList(), ImVec2(20, y), SampleData(c), ws, true);
          y += 66;
        }
        {  // Everything on the bar
          Config c;
          c.logoText = "NL";
          c.segments = {Seg::Fps, Seg::FrameTime, Seg::Ping, Seg::Game, Seg::Session, Seg::Date, Seg::Time,
                        Seg::Cpu, Seg::Gpu, Seg::Ram, Seg::User};
          c.showLow = false;
          const FontFace& face = fonts.Find(c.font);
          WatermarkStyle ws{&FindTheme("Neverlose"), face.medium, face.bold, 14.0f, c.opacity};
          Watermark(ImGui::GetBackgroundDrawList(), ImVec2(20, y), SampleData(c), ws, true);
          y += 60;
        }
        // Players: playing with cover, and the idle card.
        Config c;
        const FontFace& face = fonts.Find(c.font);
        WatermarkStyle ws{&FindTheme("Catppuccin Mocha"), face.medium, face.bold, 15.0f, c.opacity};
        ImVec2 p1(20, y + 4), p2(20 + PlayerSize(15.0f).x + 20, y + 4);
        PlayerEvents ev;
        Player(&p1, SamplePlayer(cover), ws, false, ImVec2(0, 0), io.DisplaySize, &ev);
        Player(&p2, PlayerData{}, ws, false, ImVec2(0, 0), io.DisplaySize, &ev);
      });
    }
    cv.Background();
    cv.Draw(ImGui::GetDrawData());
    cv.Save(out + "/watermark.ppm");
  }

  // 2. Menu, one image per tab.
  const char* tabNames[] = {"overlay", "bar", "music", "game", "theme", "font", "keybinds", "about"};
  for (int tab = 0; tab < 8; ++tab) {
    Config c;
    c.theme = tab == 4 ? "Midnight" : "Catppuccin Mocha";
    const Theme& theme = FindTheme(c.theme);
    Menu menu;
    menu.SelectTab(tab);
    const PlayerData player = SamplePlayer(cover);
    Canvas cv(1100, 640);
    for (int f = 0; f < 90; ++f) {
      frame(cv.w, cv.h, [&] {
        const FontFace& face = fonts.Find(c.font);
        ApplyImGuiStyle(theme, 1.0f);
        ui::g = ui::Context{&theme, 1.0f, face.medium, face.bold, 15.0f};
        ImGui::PushFont(face.medium, 15.0f);
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, Col(theme.crust, 0.45f));
        WatermarkStyle ws{&theme, face.medium, face.bold, 15.0f, c.opacity};
        Watermark(ImGui::GetBackgroundDrawList(), ImVec2(cv.w - 20 - Watermark(nullptr, {}, SampleData(c), ws, false).x, 14),
                  SampleData(c), ws, true);
        MenuContext ctx{&c, &fonts, &games, SampleData(c).ping, SampleData(c).fps, EtwStatus::Running,
                        tab == 6 ? Capture::ToggleKey : Capture::None, "", &player,
                        UpdateStatus{UpdateState::UpToDate, "0.3.0", ""}};
        ImVec2 ppos(40, cv.h - 40 - PlayerSize(15.0f).y);
        PlayerEvents pev;
        Player(&ppos, player, ws, true, ImVec2(0, 0), io.DisplaySize, &pev);
        MenuEvents ev;
        menu.Draw(ctx, 1.0f, ev);
        ImGui::PopFont();
      });
    }
    cv.Background();
    cv.Draw(ImGui::GetDrawData());
    cv.Save(out + "/menu-" + tabNames[tab] + ".ppm");
  }
  ImGui::DestroyContext();
  return 0;
}
