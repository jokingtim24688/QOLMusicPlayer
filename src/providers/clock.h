#pragma once
#include <string>

// Local time in the user's Windows time zone, e.g. "11:44 AM" or "23:44:05".
std::string ClockText(bool use24h, bool seconds);
