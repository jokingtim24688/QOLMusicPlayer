#include "providers/clock.h"

#include <windows.h>

#include <cstdio>

std::string ClockText(bool use24h, bool seconds) {
  // GetLocalTime applies the user's Windows time zone and DST.
  SYSTEMTIME t;
  GetLocalTime(&t);
  char buf[32];
  int hour = t.wHour;
  const char* suffix = "";
  if (!use24h) {
    suffix = hour < 12 ? " AM" : " PM";
    hour = hour % 12 == 0 ? 12 : hour % 12;
  }
  if (seconds)
    snprintf(buf, sizeof(buf), use24h ? "%02d:%02d:%02d%s" : "%d:%02d:%02d%s", hour, t.wMinute, t.wSecond, suffix);
  else
    snprintf(buf, sizeof(buf), use24h ? "%02d:%02d%s" : "%d:%02d%s", hour, t.wMinute, suffix);
  return buf;
}

std::string DateText(int format) {
  static const wchar_t* kPictures[] = {L"ddd, MMM d", L"d MMM", L"MM/dd", L"dd/MM", L"yyyy-MM-dd"};
  if (format < 0 || format >= kDateFormatCount) format = 0;
  wchar_t buf[64];
  if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, nullptr, kPictures[format], buf, 64, nullptr)) return {};
  char out[128];
  WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, sizeof(out), nullptr, nullptr);
  return out;
}

std::string DurationText(double seconds) {
  const long long s = seconds > 0 ? static_cast<long long>(seconds) : 0;
  char buf[32];
  if (s >= 3600) snprintf(buf, sizeof(buf), "%lld:%02lld:%02lld", s / 3600, (s / 60) % 60, s % 60);
  else snprintf(buf, sizeof(buf), "%lld:%02lld", s / 60, s % 60);
  return buf;
}
