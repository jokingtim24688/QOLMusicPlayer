#pragma once
#include <imgui.h>

// Tiny animation layer for immediate-mode widgets. State is keyed by ImGuiID.
// The app renders continuously only while anim::Active() is true, then goes back to idle.
namespace anim {

void BeginFrame(float dt);
bool Active();
// Request another frame even though no spring is moving (e.g. a pulsing indicator).
void KeepAlive();

// Critically damped spring toward `target` (no overshoot). `omega` ~ responsiveness (higher = faster).
// The first call for an id snaps to `target` unless `from` is given.
float Spring(ImGuiID id, float target, float omega = 22.0f, float from = -1e9f);

// Jump a spring to `x` (with zero velocity), e.g. to replay an entrance animation.
void Set(ImGuiID id, float x);

inline float EaseOutCubic(float t) {
  t = 1.0f - t;
  return 1.0f - t * t * t;
}

}  // namespace anim
