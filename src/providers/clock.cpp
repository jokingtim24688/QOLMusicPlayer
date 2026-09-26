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
