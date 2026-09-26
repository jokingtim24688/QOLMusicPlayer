#include "ui/fonts.h"

#include <windows.h>

#include "resource.h"

static ImFont* LoadEmbedded(int resId) {
  HMODULE mod = GetModuleHandleW(nullptr);
  HRSRC res = FindResourceW(mod, MAKEINTRESOURCEW(resId), MAKEINTRESOURCEW(10) /*RT_RCDATA*/);
  if (!res) return nullptr;
  HGLOBAL mem = LoadResource(mod, res);
  void* data = mem ? LockResource(mem) : nullptr;
  if (!data) return nullptr;
  ImFontConfig cfg;
  cfg.FontDataOwnedByAtlas = false;  // the data lives in the exe image for the whole run
  return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(data, static_cast<int>(SizeofResource(mod, res)), 16.0f, &cfg);
}

void Fonts::Load() {
  faces_ = {
      {"Geist", IDR_FONT_GEIST_MEDIUM, IDR_FONT_GEIST_BOLD},
      {"Geist Mono", IDR_FONT_GEISTMONO_MEDIUM, IDR_FONT_GEISTMONO_BOLD},
      {"JetBrains Mono", IDR_FONT_JETBRAINSMONO_MEDIUM, IDR_FONT_JETBRAINSMONO_BOLD},
      {"IBM Plex Sans", IDR_FONT_PLEXSANS_MEDIUM, IDR_FONT_PLEXSANS_BOLD},
      {"Space Grotesk", IDR_FONT_SPACEGROTESK_MEDIUM, IDR_FONT_SPACEGROTESK_BOLD},
      {"Manrope", IDR_FONT_MANROPE_MEDIUM, IDR_FONT_MANROPE_BOLD},
  };
  for (FontFace& f : faces_) {
    f.medium = LoadEmbedded(f.mediumRes);
    f.bold = LoadEmbedded(f.boldRes);
  }
  // Anything that failed to load falls back to ImGui's built-in font rather than crashing.
  ImFont* fallback = ImGui::GetIO().Fonts->AddFontDefault();
  for (FontFace& f : faces_) {
    if (!f.medium) f.medium = fallback;
    if (!f.bold) f.bold = f.medium;
  }
}

const FontFace& Fonts::Find(const std::string& name) const {
  for (const FontFace& f : faces_)
    if (name == f.name) return f;
  return faces_.front();
}
