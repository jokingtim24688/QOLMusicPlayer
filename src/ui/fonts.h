#pragma once
#include <imgui.h>

#include <string>
#include <vector>

struct FontFace {
  const char* name;
  int mediumRes, boldRes;
  ImFont* medium = nullptr;
  ImFont* bold = nullptr;
};

// Fonts are embedded in the exe (res/app.rc) and handed to ImGui without copying.
// ImGui 1.92 bakes glyphs lazily, so unused fonts cost almost nothing.
// Adding a font = TTF in assets/fonts/, a line in app.rc/resource.h, and a line in fonts.cpp.
class Fonts {
 public:
  void Load();
  std::vector<FontFace>& Faces() { return faces_; }
  const FontFace& Find(const std::string& name) const;  // falls back to the first font

 private:
  std::vector<FontFace> faces_;
};
