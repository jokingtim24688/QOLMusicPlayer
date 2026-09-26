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
  d.user = "jokingtim";
  return d;
}

int main(int argc, char** argv) {
  const std::string out = argc > 1 ? argv[1] : ".";
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
  Fonts fonts;
  fonts.Load();

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

  // 1. Watermark in several themes over a bright/dark "game" background.
  {
    const char* themes[] = {"Neverlose", "Catppuccin Mocha", "Midnight", "Catppuccin Latte", "Nord", "Gruvbox Dark"};
    Canvas cv(900, 420);
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
      });
    }
    cv.Background();
    cv.Draw(ImGui::GetDrawData());
    cv.Save(out + "/watermark.ppm");
  }

  // 2. Menu, one image per tab.
  const char* tabNames[] = {"overlay", "game", "theme", "font", "keybinds"};
  for (int tab = 0; tab < 5; ++tab) {
    Config c;
    c.theme = tab == 2 ? "Midnight" : "Catppuccin Mocha";
    const Theme& theme = FindTheme(c.theme);
    Menu menu;
    menu.SelectTab(tab);
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
                        tab == 4 ? Capture::ToggleKey : Capture::None, ""};
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
