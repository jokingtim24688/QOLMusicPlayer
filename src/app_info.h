#pragma once

// Product name in one place: window title, tray tooltip, single-instance mutex.
// (The config folder %APPDATA%\QOLOverlay and the ETW session name stay fixed so renames don't lose settings.)
#define APP_NAME_W L"QOL Overlay"

#ifndef QOL_VERSION
#define QOL_VERSION "0.0.0-dev"
#endif
#define QOL_WIDEN2(x) L##x
#define QOL_WIDEN(x) QOL_WIDEN2(x)
#define QOL_VERSION_W QOL_WIDEN(QOL_VERSION)
