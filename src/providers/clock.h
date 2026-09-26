#pragma once
#include <string>

// Local time in the user's Windows time zone, e.g. "11:44 AM" or "23:44:05".
std::string ClockText(bool use24h, bool seconds);

// Date formats offered in the menu. Day and month names follow the Windows display language.
inline constexpr const char* kDateFormats[] = {"Sat, Sep 26", "26 Sep", "09/26", "26/09", "2026-09-26"};
inline constexpr int kDateFormatCount = 5;
std::string DateText(int format);

// "1:02:03" / "12:34" for durations.
std::string DurationText(double seconds);
