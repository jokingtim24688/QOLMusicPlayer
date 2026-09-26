#pragma once
#include <imgui.h>

#include <string>

#include "providers/media.h"
#include "ui/watermark.h"

struct PlayerData {
  bool active = false;  // false = Spotify isn't open (shown as an idle card unless hidden)
  bool playing = false;
  std::string title, artist;
  float position = 0, duration = 0;  // seconds
  ImTextureID art = ImTextureID_Invalid;

  std::string Signature() const;
};

struct PlayerEvents {
  MediaCommand command = MediaCommand::None;
  bool moved = false;     // dragged this frame
  bool released = false;  // drag finished (save position)
};

ImVec2 PlayerSize(float fontSize);

// Draws the now-playing card at *pos (client coordinates).
// interactive (menu open): the card can be dragged anywhere inside [boundsMin, boundsMax] and its buttons work.
// Otherwise it's drawn into the background list and ignores the mouse, like the watermark.
void Player(ImVec2* pos, const PlayerData& d, const WatermarkStyle& st, bool interactive, ImVec2 boundsMin,
            ImVec2 boundsMax, PlayerEvents* ev);
