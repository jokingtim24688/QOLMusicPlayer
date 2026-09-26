#include "ui/anim.h"

#include <cmath>
#include <unordered_map>

namespace anim {
namespace {
struct State {
  float x, v;
};
std::unordered_map<ImGuiID, State> g_states;
float g_dt = 0.0f;
bool g_active = false;
}  // namespace

void BeginFrame(float dt) {
  // After an idle stretch ImGui reports a huge delta; clamp so springs continue smoothly instead of jumping.
  g_dt = dt > 1.0f / 30.0f ? 1.0f / 30.0f : dt;
  g_active = false;
}

bool Active() { return g_active; }

void KeepAlive() { g_active = true; }

float Spring(ImGuiID id, float target, float omega, float from) {
  auto [it, inserted] = g_states.try_emplace(id, State{from > -1e8f ? from : target, 0.0f});
  State& s = it->second;
  if (inserted && from <= -1e8f) return target;

  // Implicit-Euler critically damped spring: stable for any dt.
  const float h = g_dt, oo = omega * omega;
  const float f = 1.0f + 2.0f * h * omega;
  const float detInv = 1.0f / (f + h * h * oo);
  const float x = (f * s.x + h * s.v + h * h * oo * target) * detInv;
  const float v = (s.v + h * oo * (target - s.x)) * detInv;
  s.x = x;
  s.v = v;
  if (std::fabs(s.x - target) < 0.0005f && std::fabs(s.v) < 0.005f) {
    s.x = target;
    s.v = 0.0f;
  } else {
    g_active = true;
  }
  return s.x;
}

void Set(ImGuiID id, float x) { g_states[id] = State{x, 0.0f}; }

}  // namespace anim
