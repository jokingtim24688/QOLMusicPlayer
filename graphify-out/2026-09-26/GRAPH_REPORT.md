# Graph Report - QOLMusicPlayer  (2026-09-26)

## Corpus Check
- 55 files · ~50,619 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 19 file(s) not represented in the graph (top: .ttf 12, (none) 2, .cmake 1)

## Summary
- 811 nodes · 1468 edges · 36 communities
- Extraction: 90% EXTRACTED · 10% INFERRED · 0% AMBIGUOUS · INFERRED: 152 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `3c83ed38`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- app.h
- MenuContext
- watermark.cpp
- Config
- Canvas
- App
- Theme
- Renderer
- FpsProvider
- PingProvider
- OverlayWindow
- GameSelector
- ping.cpp
- MediaProvider
- app.cpp
- anim.cpp
- Hotkeys
- player.cpp
- HandleMessage
- PingSample
- EtwSession
- Endpoint
- Layout
- HotkeyName
- Traffic
- Progress
- QOL Overlay
- OnNetworkSend
- Updater
- WatermarkData
- algorithm
- preview.cpp
- Tick
- Fonts
- FontFace
- LoadFile

## God Nodes (most connected - your core abstractions)
1. `App` - 81 edges
2. `Config` - 39 edges
3. `PingProvider` - 36 edges
4. `MenuContext` - 30 edges
5. `Renderer` - 27 edges
6. `Theme` - 26 edges
7. `WatermarkData` - 26 edges
8. `Updater` - 26 edges
9. `GameSelector` - 22 edges
10. `Col()` - 22 edges

## Surprising Connections (you probably didn't know these)
- `main()` --references--> `Fonts`  [INFERRED]
  tools/preview/preview.cpp → src/ui/fonts.h
- `main()` --calls--> `Load`  [INFERRED]
  tools/preview/preview.cpp → src/ui/fonts.h
- `main()` --calls--> `PlayerSize()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/player.cpp
- `main()` --calls--> `Player()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/player.cpp
- `main()` --calls--> `ApplyImGuiStyle()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/theme.cpp

## Import Cycles
- None detected.

## Communities (36 total, 0 thin omitted)

### Community 0 - "app.h"
Cohesion: 0.09
Nodes (20): cstdint, fps, imgui, memory, ping, PWSTR, socket, string (+12 more)

### Community 1 - "MenuContext"
Cohesion: 0.08
Nodes (58): Glyph, ImDrawList, ImVec2, Gap(), GlyphButton(), Capture, EtwStatus, string (+50 more)

### Community 2 - "watermark.cpp"
Cohesion: 0.21
Nodes (25): ImDrawList, ImFont, ImU32, ImVec2, Seg, IconBars(), IconCalendar(), IconChip() (+17 more)

### Community 3 - "Config"
Cohesion: 0.06
Nodes (39): Corner, cstdlib, filesystem, fstream, shlobj, Config, autoUpdate, clock24h (+31 more)

### Community 4 - "Canvas"
Cohesion: 0.20
Nodes (7): ImDrawData, ImDrawVert, Canvas, h, px, w, vector

### Community 5 - "App"
Cohesion: 0.04
Nodes (49): App, animating_, appliedTheme_, artSrv_, artVersion_, capture_, cfg_, cfgDirty_ (+41 more)

### Community 6 - "Theme"
Cohesion: 0.07
Nodes (33): cstring, ApplyImGuiStyle(), ImVec4, Hex(), Lerp(), Theme, accent, bad (+25 more)

### Community 7 - "Renderer"
Cohesion: 0.09
Nodes (30): d3d11, dcomp, dxgi1_2, ID3D11Device, ID3D11DeviceContext, ID3D11RenderTargetView, IDCompositionDevice, IDCompositionTarget (+22 more)

### Community 8 - "FpsProvider"
Cohesion: 0.06
Nodes (54): BOOL, game_select, LONG, DWORD, LONGLONG, vector, FpsProvider, AverageFps (+46 more)

### Community 9 - "PingProvider"
Cohesion: 0.09
Nodes (23): deque, map, atomic, condition_variable, DWORD, mutex, thread, PingProvider (+15 more)

### Community 10 - "OverlayWindow"
Cohesion: 0.15
Nodes (14): HINSTANCE, RECT, HWND, RECT, OverlayWindow, Create, Destroy, ReassertTopmost (+6 more)

### Community 11 - "GameSelector"
Cohesion: 0.13
Nodes (17): GameEntry, exe, fps, pid, GameSelector, candidates_, exe_, names_ (+9 more)

### Community 12 - "ping.cpp"
Cohesion: 0.22
Nodes (12): icmpapi, iphlpapi, BYTE, string, EndpointText(), IsPrivate(), Echo, PickEndpoint (+4 more)

### Community 13 - "MediaProvider"
Cohesion: 0.05
Nodes (48): chrono, cwctype, GlobalSystemMediaTransportControlsSession, IRandomAccessStreamReference, media, shcore, MediaCommand, shared_ptr (+40 more)

### Community 14 - "app.cpp"
Cohesion: 0.18
Nodes (16): imgui_impl_dx11, imgui_impl_win32, Balloon, CloseMenu, InstallUpdate, MsUntilNextTick, Render, Run (+8 more)

### Community 15 - "anim.cpp"
Cohesion: 0.18
Nodes (8): anim, cmath, ImGuiID, Set(), Spring(), State, v, x

### Community 16 - "Hotkeys"
Cohesion: 0.26
Nodes (10): HotkeyId, Hotkey, mods, vk, HWND, Hotkeys, hwnd_, Register (+2 more)

### Community 17 - "player.cpp"
Cohesion: 0.06
Nodes (52): ButtonCenters(), ImDrawList, ImFont, ImU32, ImVec2, string, Draw(), Fit() (+44 more)

### Community 18 - "HandleMessage"
Cohesion: 0.33
Nodes (10): LRESULT, HandleMessage, OpenMenu, ToggleOverlay, TrayMenu, CALLBACK App::WndProc(), HWND, LPARAM (+2 more)

### Community 19 - "PingSample"
Cohesion: 0.29
Nodes (7): PingState, string, Sample, PingSample, endpoint, ms, state

### Community 20 - "EtwSession"
Cohesion: 0.09
Nodes (29): BOOLEAN, etw_session, GUID, BYTE, FpsProvider, PEVENT_RECORD, PingProvider, ULONGLONG (+21 more)

### Community 21 - "Endpoint"
Cohesion: 0.33
Nodes (5): Endpoint, addr, family, array, BYTE

### Community 22 - "Layout"
Cohesion: 0.33
Nodes (6): Layout, addrLen, addrOffset, known, pidOffset, ULONG

### Community 23 - "HotkeyName"
Cohesion: 0.60
Nodes (5): FinishCapture, RegisterHotkeys, string, HotkeyName(), KeyName()

### Community 24 - "Traffic"
Cohesion: 0.50
Nodes (4): ULONGLONG, Traffic, tcpBytes, udpBytes

### Community 25 - "Progress"
Cohesion: 0.11
Nodes (17): 1.1 What the cheat UIs actually are, 1.2 Precedent for "external + not banned", 1.3 Where each number comes from (no injection, no kernel driver), 1.4 How we avoid bans (the rules), 1. Research findings, 2.5 Visual design rules (anti-vibe pass), 2. Proposed architecture, 3. Milestones (+9 more)

### Community 26 - "QOL Overlay"
Cohesion: 0.29
Nodes (6): Build, Features, Install, Known limits, QOL Overlay, Why it doesn't get you banned

### Community 27 - "OnNetworkSend"
Cohesion: 0.33
Nodes (4): DWORD, PEVENT_RECORD, OnNetworkSend, SetTarget

### Community 28 - "Updater"
Cohesion: 0.07
Nodes (38): bcrypt, shellapi, HWND, string, wstring, condition_variable, HWND, mutex (+30 more)

### Community 29 - "WatermarkData"
Cohesion: 0.07
Nodes (32): Seg, string, vector, HasSeg(), SegInfo, desc, id, key (+24 more)

### Community 30 - "algorithm"
Cohesion: 0.12
Nodes (16): algorithm, FILETIME, pdh, pdhmsg, ULONGLONG, ULONGLONG, SystemStats, CloseGpu (+8 more)

### Community 31 - "preview.cpp"
Cohesion: 0.23
Nodes (9): ImTextureData, player, ImTextureID, FakeCover(), main(), SampleData(), SamplePlayer(), ServiceTextures() (+1 more)

### Community 32 - "Tick"
Cohesion: 0.39
Nodes (7): clock, cstdio, Tick, ClockText(), string, DateText(), DurationText()

### Community 33 - "Fonts"
Cohesion: 0.32
Nodes (6): ImFont, Fonts, faces_, Load, vector, LoadEmbedded()

### Community 34 - "FontFace"
Cohesion: 0.29
Nodes (7): FontFace, bold, boldRes, medium, mediumRes, name, ImFont

### Community 35 - "LoadFile"
Cohesion: 0.33
Nodes (5): ImFont, string, Fonts::Load(), HotkeyName(), LoadFile()

## Knowledge Gaps
- **283 isolated node(s):** `WndProc`, `cfg_`, `window_`, `renderer_`, `hotkeys_` (+278 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 438 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `App` connect `App` to `app.h`, `MenuContext`, `Config`, `Theme`, `Renderer`, `OverlayWindow`, `GameSelector`, `MediaProvider`, `app.cpp`, `Hotkeys`, `player.cpp`, `HandleMessage`, `EtwSession`, `HotkeyName`, `Updater`, `WatermarkData`, `algorithm`, `Tick`, `Fonts`?**
  _High betweenness centrality (0.331) - this node is a cross-community bridge._
- **Why does `Config` connect `Config` to `app.h`, `MenuContext`, `App`, `Hotkeys`, `preview.cpp`?**
  _High betweenness centrality (0.088) - this node is a cross-community bridge._
- **Why does `PingProvider` connect `PingProvider` to `app.h`, `ping.cpp`, `PingSample`, `Endpoint`, `Layout`, `Traffic`, `OnNetworkSend`?**
  _High betweenness centrality (0.086) - this node is a cross-community bridge._
- **What connects `WndProc`, `cfg_`, `window_` to the rest of the system?**
  _283 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `app.h` be split into smaller, more focused modules?**
  _Cohesion score 0.09468599033816426 - nodes in this community are weakly interconnected._
- **Should `MenuContext` be split into smaller, more focused modules?**
  _Cohesion score 0.08415300546448087 - nodes in this community are weakly interconnected._
- **Should `Config` be split into smaller, more focused modules?**
  _Cohesion score 0.05512820512820513 - nodes in this community are weakly interconnected._