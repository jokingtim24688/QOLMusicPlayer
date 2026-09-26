# Graph Report - QOLMusicPlayer  (2026-09-26)

## Corpus Check
- 50 files · ~39,115 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 19 file(s) not represented in the graph (top: .ttf 12, (none) 2, .cmake 1)

## Summary
- 700 nodes · 1216 edges · 29 communities (27 shown, 2 thin omitted)
- Extraction: 90% EXTRACTED · 10% INFERRED · 0% AMBIGUOUS · INFERRED: 120 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `707ba8cc`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- app.h
- MenuContext
- WatermarkData
- Config
- preview.cpp
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
- External Overlay — Research & Plan
- QOL Overlay
- SetTarget
- OnNetworkSend

## God Nodes (most connected - your core abstractions)
1. `App` - 73 edges
2. `Config` - 37 edges
3. `PingProvider` - 36 edges
4. `Renderer` - 27 edges
5. `MenuContext` - 26 edges
6. `Theme` - 26 edges
7. `MediaProvider` - 21 edges
8. `WatermarkData` - 21 edges
9. `Col()` - 19 edges
10. `Draw()` - 18 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `PlayerSize()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/player.cpp
- `main()` --calls--> `Player()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/player.cpp
- `main()` --calls--> `ApplyImGuiStyle()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/theme.cpp
- `main()` --calls--> `Col()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/theme.h
- `main()` --calls--> `Watermark()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/watermark.cpp

## Import Cycles
- None detected.

## Communities (29 total, 2 thin omitted)

### Community 0 - "app.h"
Cohesion: 0.07
Nodes (25): deque, etw_session, fps, imgui, map, media, memory, ping (+17 more)

### Community 1 - "MenuContext"
Cohesion: 0.09
Nodes (51): ImDrawList, ImVec2, Gap(), Capture, EtwStatus, string, Menu, Draw (+43 more)

### Community 2 - "WatermarkData"
Cohesion: 0.07
Nodes (39): ImDrawList, ImFont, ImU32, ImVec2, string, ImFont, string, IconBars() (+31 more)

### Community 3 - "Config"
Cohesion: 0.05
Nodes (43): algorithm, clock, Corner, cstdio, cstdlib, filesystem, fstream, shlobj (+35 more)

### Community 4 - "preview.cpp"
Cohesion: 0.07
Nodes (33): ImDrawData, ImDrawVert, ImTextureData, ImFont, FontFace, bold, boldRes, medium (+25 more)

### Community 5 - "App"
Cohesion: 0.04
Nodes (45): App, animating_, appliedTheme_, artSrv_, artVersion_, capture_, cfg_, cfgDirty_ (+37 more)

### Community 6 - "Theme"
Cohesion: 0.07
Nodes (34): cstdint, cstring, ApplyImGuiStyle(), ImVec4, Hex(), Lerp(), Theme, accent (+26 more)

### Community 7 - "Renderer"
Cohesion: 0.09
Nodes (30): d3d11, dcomp, dxgi1_2, ID3D11Device, ID3D11DeviceContext, ID3D11RenderTargetView, IDCompositionDevice, IDCompositionTarget (+22 more)

### Community 8 - "FpsProvider"
Cohesion: 0.11
Nodes (30): DWORD, LONGLONG, vector, FpsProvider, AverageFps, Fresh, IsPresenting, kRing (+22 more)

### Community 9 - "PingProvider"
Cohesion: 0.10
Nodes (21): atomic, condition_variable, DWORD, mutex, thread, PingProvider, current_, cv_ (+13 more)

### Community 10 - "OverlayWindow"
Cohesion: 0.14
Nodes (14): HINSTANCE, RECT, HWND, RECT, OverlayWindow, Create, Destroy, ReassertTopmost (+6 more)

### Community 11 - "GameSelector"
Cohesion: 0.07
Nodes (34): BOOL, game_select, LONG, DWORD, FpsProvider, HWND, LPARAM, string (+26 more)

### Community 12 - "ping.cpp"
Cohesion: 0.22
Nodes (12): icmpapi, iphlpapi, BYTE, string, EndpointText(), IsPrivate(), Echo, PickEndpoint (+4 more)

### Community 13 - "MediaProvider"
Cohesion: 0.05
Nodes (47): chrono, cwctype, GlobalSystemMediaTransportControlsSession, IRandomAccessStreamReference, shcore, MediaCommand, shared_ptr, DecodeArt() (+39 more)

### Community 14 - "app.cpp"
Cohesion: 0.18
Nodes (18): imgui_impl_dx11, imgui_impl_win32, shellapi, CloseMenu, MsUntilNextTick, OpenMenu, Render, Run (+10 more)

### Community 15 - "anim.cpp"
Cohesion: 0.18
Nodes (8): anim, cmath, ImGuiID, Set(), Spring(), State, v, x

### Community 16 - "Hotkeys"
Cohesion: 0.26
Nodes (10): HotkeyId, Hotkey, mods, vk, HWND, Hotkeys, hwnd_, Register (+2 more)

### Community 17 - "player.cpp"
Cohesion: 0.08
Nodes (45): ButtonCenters(), ImDrawList, ImFont, ImU32, ImVec2, string, Draw(), Fit() (+37 more)

### Community 18 - "HandleMessage"
Cohesion: 0.48
Nodes (7): LRESULT, HandleMessage, CALLBACK App::WndProc(), HWND, LPARAM, UINT, WPARAM

### Community 19 - "PingSample"
Cohesion: 0.29
Nodes (7): PingState, string, Sample, PingSample, endpoint, ms, state

### Community 20 - "EtwSession"
Cohesion: 0.09
Nodes (28): BOOLEAN, GUID, BYTE, FpsProvider, PEVENT_RECORD, PingProvider, ULONGLONG, vector (+20 more)

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

### Community 25 - "External Overlay — Research & Plan"
Cohesion: 0.11
Nodes (16): 1.1 What the cheat UIs actually are, 1.2 Precedent for "external + not banned", 1.3 Where each number comes from (no injection, no kernel driver), 1.4 How we avoid bans (the rules), 1. Research findings, 2.5 Visual design rules (anti-vibe pass), 2. Proposed architecture, 3. Milestones (+8 more)

### Community 26 - "QOL Overlay"
Cohesion: 0.29
Nodes (6): Build, Features, Install, Known limits, QOL Overlay, Why it doesn't get you banned

## Knowledge Gaps
- **253 isolated node(s):** `WndProc`, `cfg_`, `window_`, `renderer_`, `hotkeys_` (+248 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 384 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `App` connect `App` to `app.h`, `MenuContext`, `WatermarkData`, `Config`, `preview.cpp`, `Theme`, `Renderer`, `OverlayWindow`, `GameSelector`, `MediaProvider`, `app.cpp`, `Hotkeys`, `player.cpp`, `HandleMessage`, `EtwSession`, `HotkeyName`?**
  _High betweenness centrality (0.324) - this node is a cross-community bridge._
- **Why does `PingProvider` connect `PingProvider` to `app.h`, `ping.cpp`, `PingSample`, `Endpoint`, `Layout`, `Traffic`, `SetTarget`, `OnNetworkSend`?**
  _High betweenness centrality (0.099) - this node is a cross-community bridge._
- **Why does `Config` connect `Config` to `app.h`, `MenuContext`, `preview.cpp`, `App`, `Hotkeys`?**
  _High betweenness centrality (0.093) - this node is a cross-community bridge._
- **What connects `WndProc`, `cfg_`, `window_` to the rest of the system?**
  _253 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `app.h` be split into smaller, more focused modules?**
  _Cohesion score 0.07482993197278912 - nodes in this community are weakly interconnected._
- **Should `MenuContext` be split into smaller, more focused modules?**
  _Cohesion score 0.09143686502177069 - nodes in this community are weakly interconnected._
- **Should `WatermarkData` be split into smaller, more focused modules?**
  _Cohesion score 0.07087486157253599 - nodes in this community are weakly interconnected._