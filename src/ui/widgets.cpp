#include "ui/widgets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "ui/anim.h"

namespace ui {

Context g;

static const char* LabelEnd(const char* label) {
  const char* hash = std::strstr(label, "##");
  return hash ? hash : label + std::strlen(label);
}

static void CenteredText(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, const char* text) {
  ImVec2 size = ImGui::CalcTextSize(text, LabelEnd(text));
  dl->AddText(ImVec2(min.x + (max.x - min.x - size.x) * 0.5f, min.y + (max.y - min.y - size.y) * 0.5f), col, text,
              LabelEnd(text));
}

void Heading(const char* text) {
  ImGui::PushFont(g.bold, g.fontSize * 1.35f * g.scale);
  ImGui::TextUnformatted(text);
  ImGui::PopFont();
  ImGui::Dummy(ImVec2(0, S(2)));
}

void Note(const char* text, const ImVec4& color) {
  ImGui::PushStyleColor(ImGuiCol_Text, color);
  ImGui::PushTextWrapPos(0.0f);
  ImGui::TextUnformatted(text);
  ImGui::PopTextWrapPos();
  ImGui::PopStyleColor();
}

void Note(const char* text) { Note(text, T().subtext); }

bool Toggle(const char* label, bool* v) {
  const ImGuiID id = ImGui::GetID(label);
  const float w = ImGui::GetContentRegionAvail().x, h = S(36);
  const ImVec2 p = ImGui::GetCursorScreenPos();
  const bool clicked = ImGui::InvisibleButton(label, ImVec2(w, h));
  if (clicked) *v = !*v;
  const float on = anim::Spring(id, *v ? 1.0f : 0.0f, 20.0f);
  const float hover = anim::Spring(id + 1, ImGui::IsItemHovered() ? 1.0f : 0.0f, 30.0f);
  const float press = anim::Spring(id + 2, ImGui::IsItemActive() ? 1.0f : 0.0f, 40.0f);

  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(p, p + ImVec2(w, h), Col(T().surface0, 0.55f * hover), S(8));
  const ImVec2 ts = ImGui::CalcTextSize(label, LabelEnd(label));
  dl->AddText(ImVec2(p.x + S(10), p.y + (h - ts.y) * 0.5f), Col(T().text), label, LabelEnd(label));

  const float sw = S(36), sh = S(20);
  const ImVec2 sp(p.x + w - sw - S(10), p.y + (h - sh) * 0.5f);
  dl->AddRectFilled(sp, sp + ImVec2(sw, sh), Col(Mix(T().surface1, T().accent, on)), sh * 0.5f);
  const float r = (sh * 0.5f - S(3)) * (1.0f + 0.12f * press);
  const ImVec2 knob(sp.x + sh * 0.5f + on * (sw - sh), sp.y + sh * 0.5f);
  dl->AddCircleFilled(knob, r, Col(Mix(T().text, T().crust, on)));
  return clicked;
}

bool Segmented(const char* id, int* current, const char* const* items, int count) {
  ImGui::PushID(id);
  const ImGuiID gid = ImGui::GetID("##seg");
  const float w = ImGui::GetContentRegionAvail().x, h = S(32);
  const ImVec2 p = ImGui::GetCursorScreenPos();
  const float segW = w / static_cast<float>(count);
  const bool clicked = ImGui::InvisibleButton("##seg", ImVec2(w, h));
  bool changed = false;
  if (clicked) {
    int idx = std::clamp(static_cast<int>((ImGui::GetIO().MousePos.x - p.x) / segW), 0, count - 1);
    changed = idx != *current;
    *current = idx;
  }
  const float x = anim::Spring(gid, static_cast<float>(*current), 24.0f);

  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(p, p + ImVec2(w, h), Col(T().surface0), S(8));
  const ImVec2 hp(p.x + x * segW + S(3), p.y + S(3));
  dl->AddRectFilled(hp, hp + ImVec2(segW - S(6), h - S(6)), Col(T().surface1), S(6));
  for (int i = 0; i < count; ++i) {
    const float sel = std::max(0.0f, 1.0f - std::fabs(x - static_cast<float>(i)));
    const ImVec2 a(p.x + segW * static_cast<float>(i), p.y);
    CenteredText(dl, a, a + ImVec2(segW, h), Col(Mix(T().subtext, T().text, sel)), items[i]);
  }
  ImGui::PopID();
  return changed;
}

bool Slider(const char* label, float* v, float min, float max, const char* format) {
  ImGui::PushID(label);
  const ImGuiID id = ImGui::GetID("##slider");
  const float w = ImGui::GetContentRegionAvail().x;
  ImVec2 p = ImGui::GetCursorScreenPos();

  char value[32];
  snprintf(value, sizeof(value), format, *v);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const float lineH = ImGui::GetTextLineHeight();
  dl->AddText(ImVec2(p.x + S(10), p.y), Col(T().text), label, LabelEnd(label));
  const ImVec2 vs = ImGui::CalcTextSize(value);
  dl->AddText(ImVec2(p.x + w - S(10) - vs.x, p.y), Col(T().subtext), value);

  const float trackY = p.y + lineH + S(12), pad = S(10);
  ImGui::SetCursorScreenPos(ImVec2(p.x, trackY - S(10)));
  ImGui::InvisibleButton("##slider", ImVec2(w, S(20)));
  bool changed = false;
  if (ImGui::IsItemActive()) {
    float t = std::clamp((ImGui::GetIO().MousePos.x - p.x - pad) / (w - pad * 2), 0.0f, 1.0f);
    float nv = min + t * (max - min);
    changed = nv != *v;
    *v = nv;
  }
  const float hover = anim::Spring(id, ImGui::IsItemHovered() || ImGui::IsItemActive() ? 1.0f : 0.0f, 30.0f);
  const float t = (*v - min) / (max - min);
  const float x0 = p.x + pad, x1 = p.x + w - pad, kx = x0 + (x1 - x0) * t;
  dl->AddRectFilled(ImVec2(x0, trackY - S(2)), ImVec2(x1, trackY + S(2)), Col(T().surface1), S(2));
  dl->AddRectFilled(ImVec2(x0, trackY - S(2)), ImVec2(kx, trackY + S(2)), Col(T().accent), S(2));
  dl->AddCircleFilled(ImVec2(kx, trackY), S(6) + S(2) * hover, Col(T().accent));
  dl->AddCircleFilled(ImVec2(kx, trackY), S(2.5f), Col(T().crust));
  ImGui::SetCursorScreenPos(ImVec2(p.x, trackY + S(14)));
  ImGui::Dummy(ImVec2(w, 0));
  ImGui::PopID();
  return changed;
}

bool TextField(const char* label, std::string* value, const char* hint) {
  ImGui::PushID(label);
  const ImVec2 p = ImGui::GetCursorScreenPos();
  ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + S(10), p.y), Col(T().text), label, LabelEnd(label));
  ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
  char buf[64];
  snprintf(buf, sizeof(buf), "%s", value->c_str());
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  const bool changed = ImGui::InputTextWithHint("##field", hint, buf, sizeof(buf));
  if (changed) *value = buf;
  ImGui::PopID();
  return changed;
}

bool SelectRow(const char* id, bool selected, float height, ImVec2* min, ImVec2* max) {
  const ImGuiID gid = ImGui::GetID(id);
  const float w = ImGui::GetContentRegionAvail().x;
  const ImVec2 p = ImGui::GetCursorScreenPos();
  const bool clicked = ImGui::InvisibleButton(id, ImVec2(w, height));
  const float hover = anim::Spring(gid, ImGui::IsItemHovered() ? 1.0f : 0.0f, 30.0f);
  const float sel = anim::Spring(gid + 1, selected ? 1.0f : 0.0f, 22.0f);

  ImDrawList* dl = ImGui::GetWindowDrawList();
  const float bg = std::max(sel, 0.55f * hover);
  dl->AddRectFilled(p, p + ImVec2(w, height), Col(T().surface0, bg), S(8));
  if (sel > 0.001f) {
    const float barH = (height - S(16)) * sel;
    const float cy = p.y + height * 0.5f;
    dl->AddRectFilled(ImVec2(p.x + S(4), cy - barH * 0.5f), ImVec2(p.x + S(7), cy + barH * 0.5f), Col(T().accent),
                      S(2));
  }
  *min = p;
  *max = p + ImVec2(w, height);
  return clicked;
}

bool KeybindButton(const char* label, const std::string& keyName, bool capturing) {
  ImGui::PushID(label);
  const ImGuiID id = ImGui::GetID("##bind");
  const float w = ImGui::GetContentRegionAvail().x, h = S(44);
  const ImVec2 p = ImGui::GetCursorScreenPos();
  const bool clicked = ImGui::InvisibleButton("##bind", ImVec2(w, h));
  const float hover = anim::Spring(id, ImGui::IsItemHovered() ? 1.0f : 0.0f, 30.0f);

  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(p, p + ImVec2(w, h), Col(T().surface0, 0.55f * hover), S(8));
  const ImVec2 ls = ImGui::CalcTextSize(label, LabelEnd(label));
  dl->AddText(ImVec2(p.x + S(10), p.y + (h - ls.y) * 0.5f), Col(T().text), label, LabelEnd(label));

  const char* text = capturing ? "Press a key..." : keyName.c_str();
  const ImVec2 ks = ImGui::CalcTextSize(text);
  const ImVec2 kmin(p.x + w - S(10) - ks.x - S(24), p.y + S(8));
  const ImVec2 kmax(p.x + w - S(10), p.y + h - S(8));
  ImVec4 border = T().overlay;
  if (capturing) {
    // Pulse while waiting for a key. This is chrome, so it's fine to keep animating.
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 6.0f);
    border = Mix(T().surface1, T().accent, pulse);
    anim::KeepAlive();
  }
  dl->AddRectFilled(kmin, kmax, Col(T().surface1), S(6));
  dl->AddRect(kmin, kmax, Col(border), S(6), S(1.5f));
  CenteredText(dl, kmin, kmax, Col(capturing ? T().accent : T().text), text);
  ImGui::PopID();
  return clicked;
}

}  // namespace ui
