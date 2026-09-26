#pragma once
#include <imgui.h>

#include <string>
#include <vector>

// A theme is pure data; adding one = adding one entry to the table in theme.cpp.
struct Theme {
  const char* name = "";
  const char* family = "";
  ImVec4 crust, base, surface0, surface1, overlay, text, subtext, accent, good, warn, bad;
};

const std::vector<Theme>& Themes();
const Theme& FindTheme(const std::string& name);  // falls back to the first theme

// Current on-screen colors. Switching themes lerps every color over ~250 ms.
class ThemeAnimator {
 public:
  void Set(const Theme& target, bool instant);
  bool Update(float dt);  // true while still transitioning
  const Theme& Current() const { return current_; }

 private:
  Theme from_{}, to_{}, current_{};
  float t_ = 1.0f;
};

void ApplyImGuiStyle(const Theme& t, float scale);

inline ImU32 Col(const ImVec4& c, float alpha = 1.0f) {
  return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alpha));
}
inline ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t) {
  return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}
