#include <windows.h>

#include "app.h"
#include "app_info.h"

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
  // One instance only: a second copy would fight over the hotkeys and the trace session.
  HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\QOLOverlay-SingleInstance");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    MessageBoxW(nullptr, APP_NAME_W L" is already running. Look for it in the system tray.", APP_NAME_W,
                MB_ICONINFORMATION);
    return 0;
  }
  App app;
  const int rc = app.Run(inst);
  if (mutex) CloseHandle(mutex);
  return rc;
}
