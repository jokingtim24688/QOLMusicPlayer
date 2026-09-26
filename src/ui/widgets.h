#pragma once
#include <imgui.h>

#include <string>

#include "ui/theme.h"

// Custom widgets for the menu. Everything is drawn with ImDrawList so the look is ours, not ImGui's defaults;
// ImGui only provides layout, ids and input.
namespace ui {

struct Context {
  const Theme* theme = nullptr;
  float scale = 1.0f;  // DPI scale
  ImFont* regular = nullptr;
  ImFont* bold = nullptr;
  float fontSize = 15.0f;  // unscaled menu text size
};
extern Context g;

inline float S(float v) { return v * g.scale; }
inline const Theme& T() { return *g.theme; }

void Heading(const char* text);
void Note(const char* text, const ImVec4& color);
void Note(const char* text);

bool Toggle(const char* label, bool* v);
bool Segmented(const char* id, int* current, const char* const* items, int count);
bool Slider(const char* label, float* v, float min, float max, const char* format);
bool TextField(const char* label, std::string* value, const char* hint);

// Full-width selectable row with animated hover/selection. Content is drawn by the caller inside [min, max].
bool SelectRow(const char* id, bool selected, float height, ImVec2* min, ImVec2* max);

// Keybind button: shows the key, or a pulsing "press a key" while capturing.
bool KeybindButton(const char* label, const std::string& keyName, bool capturing);

}  // namespace ui
