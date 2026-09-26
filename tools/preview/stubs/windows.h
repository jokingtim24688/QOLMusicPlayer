// Minimal Windows type stubs so the UI code (theme/anim/widgets/watermark/menu) builds on Linux for previews.
#pragma once
#include <cstdint>
#include <strings.h>
typedef uint32_t DWORD;
typedef uint8_t BYTE;
typedef unsigned char UCHAR, BOOLEAN;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef unsigned long long ULONGLONG;
typedef long long LONGLONG;
typedef void* HANDLE;
typedef struct HWND__* HWND;
typedef struct HMONITOR__* HMONITOR;
#define WINAPI
struct GUID { uint32_t a; uint16_t b, c; uint8_t d[8]; };
#define _stricmp strcasecmp
