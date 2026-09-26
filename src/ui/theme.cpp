#include "ui/theme.h"

#include <cstdint>
#include <cstring>

static constexpr ImVec4 Hex(uint32_t rgb) {
  return ImVec4(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f, 1.0f);
}

// Palettes use each theme's official colors.
// Columns: crust, base, surface0, surface1, overlay, text, subtext, accent, good, warn, bad
const std::vector<Theme>& Themes() {
  static const std::vector<Theme> themes = {
      {"Catppuccin Mocha", "Catppuccin", Hex(0x11111b), Hex(0x1e1e2e), Hex(0x313244), Hex(0x45475a), Hex(0x6c7086),
       Hex(0xcdd6f4), Hex(0xa6adc8), Hex(0xcba6f7), Hex(0xa6e3a1), Hex(0xf9e2af), Hex(0xf38ba8)},
      {"Catppuccin Macchiato", "Catppuccin", Hex(0x181926), Hex(0x24273a), Hex(0x363a4f), Hex(0x494d64),
       Hex(0x6e738d), Hex(0xcad3f5), Hex(0xa5adcb), Hex(0xc6a0f6), Hex(0xa6da95), Hex(0xeed49f), Hex(0xed8796)},
      {"Catppuccin Frappé", "Catppuccin", Hex(0x232634), Hex(0x303446), Hex(0x414559), Hex(0x51576d), Hex(0x737994),
       Hex(0xc6d0f5), Hex(0xa5adce), Hex(0xca9ee6), Hex(0xa6d189), Hex(0xe5c890), Hex(0xe78284)},
      {"Catppuccin Latte", "Catppuccin", Hex(0xdce0e8), Hex(0xeff1f5), Hex(0xccd0da), Hex(0xbcc0cc), Hex(0x9ca0b0),
       Hex(0x4c4f69), Hex(0x6c6f85), Hex(0x8839ef), Hex(0x40a02b), Hex(0xdf8e1d), Hex(0xd20f39)},
      {"Midnight", "Midnight", Hex(0x06080c), Hex(0x0b0e14), Hex(0x151a23), Hex(0x1f2633), Hex(0x3a4556),
       Hex(0xe6edf3), Hex(0x8b98a9), Hex(0x4c8dff), Hex(0x3fb950), Hex(0xd29922), Hex(0xf85149)},
      {"Neverlose", "Neverlose", Hex(0x05070b), Hex(0x0a0d14), Hex(0x111621), Hex(0x1a2130), Hex(0x2c3649),
       Hex(0xe8ecf4), Hex(0x7f8aa3), Hex(0x2fa8ff), Hex(0x57d68d), Hex(0xf5c451), Hex(0xff5c7a)},
      {"Tokyo Night", "Tokyo Night", Hex(0x16161e), Hex(0x1a1b26), Hex(0x24283b), Hex(0x292e42), Hex(0x565f89),
       Hex(0xc0caf5), Hex(0xa9b1d6), Hex(0x7aa2f7), Hex(0x9ece6a), Hex(0xe0af68), Hex(0xf7768e)},
      {"Dracula", "Dracula", Hex(0x21222c), Hex(0x282a36), Hex(0x343746), Hex(0x44475a), Hex(0x6272a4),
       Hex(0xf8f8f2), Hex(0xbfbfbf), Hex(0xbd93f9), Hex(0x50fa7b), Hex(0xf1fa8c), Hex(0xff5555)},
      {"Nord", "Nord", Hex(0x242933), Hex(0x2e3440), Hex(0x3b4252), Hex(0x434c5e), Hex(0x4c566a), Hex(0xeceff4),
       Hex(0xd8dee9), Hex(0x88c0d0), Hex(0xa3be8c), Hex(0xebcb8b), Hex(0xbf616a)},
      {"Gruvbox Dark", "Gruvbox", Hex(0x1d2021), Hex(0x282828), Hex(0x3c3836), Hex(0x504945), Hex(0x928374),
       Hex(0xebdbb2), Hex(0xbdae93), Hex(0xfe8019), Hex(0xb8bb26), Hex(0xfabd2f), Hex(0xfb4934)},
      {"Rosé Pine", "Rosé Pine", Hex(0x191724), Hex(0x1f1d2e), Hex(0x26233a), Hex(0x403d52), Hex(0x6e6a86),
       Hex(0xe0def4), Hex(0x908caa), Hex(0xebbcba), Hex(0x9ccfd8), Hex(0xf6c177), Hex(0xeb6f92)},
      {"One Dark", "One Dark", Hex(0x21252b), Hex(0x282c34), Hex(0x2c313a), Hex(0x3e4451), Hex(0x5c6370),
       Hex(0xabb2bf), Hex(0x828997), Hex(0x61afef), Hex(0x98c379), Hex(0xe5c07b), Hex(0xe06c75)},
  };
  return themes;
}

const Theme& FindTheme(const std::string& name) {
  for (const Theme& t : Themes())
    if (name == t.name) return t;
  return Themes().front();
}

static Theme Lerp(const Theme& a, const Theme& b, float t) {
  Theme r = b;
  r.crust = Mix(a.crust, b.crust, t);
  r.base = Mix(a.base, b.base, t);
  r.surface0 = Mix(a.surface0, b.surface0, t);
  r.surface1 = Mix(a.surface1, b.surface1, t);
  r.overlay = Mix(a.overlay, b.overlay, t);
  r.text = Mix(a.text, b.text, t);
  r.subtext = Mix(a.subtext, b.subtext, t);
  r.accent = Mix(a.accent, b.accent, t);
  r.good = Mix(a.good, b.good, t);
  r.warn = Mix(a.warn, b.warn, t);
  r.bad = Mix(a.bad, b.bad, t);
  return r;
}

void ThemeAnimator::Set(const Theme& target, bool instant) {
  if (!instant && std::strcmp(target.name, to_.name) == 0 && to_.name[0]) return;
  from_ = current_;
  to_ = target;
  t_ = instant ? 1.0f : 0.0f;
  if (instant) current_ = target;
}

bool ThemeAnimator::Update(float dt) {
  if (t_ >= 1.0f) return false;
  t_ = t_ + dt / 0.25f;
  if (t_ > 1.0f) t_ = 1.0f;
  float e = 1.0f - (1.0f - t_) * (1.0f - t_) * (1.0f - t_);  // ease-out cubic
  current_ = Lerp(from_, to_, e);
  return t_ < 1.0f;
}

void ApplyImGuiStyle(const Theme& t, float scale) {
  ImGuiStyle style;
  style.WindowPadding = ImVec2(0, 0);
  style.WindowRounding = 12.0f;
  style.WindowBorderSize = 1.0f;
  style.ChildRounding = 8.0f;
  style.FrameRounding = 6.0f;
  style.FramePadding = ImVec2(10, 7);
  style.ItemSpacing = ImVec2(8, 8);
  style.GrabRounding = 6.0f;
  style.ScrollbarRounding = 6.0f;
  style.ScrollbarSize = 8.0f;
  style.PopupRounding = 8.0f;
  style.ScaleAllSizes(scale);

  ImVec4* c = style.Colors;
  c[ImGuiCol_Text] = t.text;
  c[ImGuiCol_TextDisabled] = t.subtext;
  c[ImGuiCol_WindowBg] = t.base;
  c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_PopupBg] = t.base;
  c[ImGuiCol_Border] = ImVec4(t.overlay.x, t.overlay.y, t.overlay.z, 0.45f);
  c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_FrameBg] = t.surface0;
  c[ImGuiCol_FrameBgHovered] = t.surface1;
  c[ImGuiCol_FrameBgActive] = t.surface1;
  c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_ScrollbarGrab] = t.surface1;
  c[ImGuiCol_ScrollbarGrabHovered] = t.overlay;
  c[ImGuiCol_ScrollbarGrabActive] = t.accent;
  c[ImGuiCol_SliderGrab] = t.accent;
  c[ImGuiCol_SliderGrabActive] = t.accent;
  c[ImGuiCol_Button] = t.surface0;
  c[ImGuiCol_ButtonHovered] = t.surface1;
  c[ImGuiCol_ButtonActive] = t.surface1;
  c[ImGuiCol_Header] = t.surface0;
  c[ImGuiCol_HeaderHovered] = t.surface1;
  c[ImGuiCol_HeaderActive] = t.surface1;
  c[ImGuiCol_TextSelectedBg] = ImVec4(t.accent.x, t.accent.y, t.accent.z, 0.35f);
  c[ImGuiCol_NavCursor] = ImVec4(0, 0, 0, 0);
  ImGui::GetStyle() = style;
}
