# Graph Report - QOLMusicPlayer  (2026-09-26)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 554 nodes · 972 edges · 25 communities
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 106 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `e189c73d`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Community 0
- Community 1
- Community 2
- Community 3
- Community 4
- Community 5
- Community 6
- Community 7
- Community 8
- Community 9
- Community 10
- Community 11
- Community 12
- Community 13
- Community 14
- Community 15
- Community 16
- Community 17
- Community 18
- Community 19
- Community 20
- Community 21
- Community 22
- Community 23
- Community 24

## God Nodes (most connected - your core abstractions)
1. `App` - 63 edges
2. `PingProvider` - 36 edges
3. `Config` - 32 edges
4. `Theme` - 26 edges
5. `Renderer` - 26 edges
6. `MenuContext` - 23 edges
7. `WatermarkData` - 21 edges
8. `EtwSession` - 16 edges
9. `GameSelector` - 16 edges
10. `Col()` - 16 edges

## Surprising Connections (you probably didn't know these)
- `LoadFile()` --references--> `WatermarkData`  [INFERRED]
  tools/preview/preview.cpp → src/ui/watermark.h
- `main()` --calls--> `Col()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/theme.h
- `main()` --calls--> `Watermark()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/watermark.cpp
- `main()` --calls--> `ApplyImGuiStyle()`  [INFERRED]
  tools/preview/preview.cpp → src/ui/theme.cpp
- `HotkeyName()` --references--> `Hotkey`  [EXTRACTED]
  tools/preview/preview.cpp → src/config/config.h

## Import Cycles
- None detected.

## Communities (25 total, 0 thin omitted)

### Community 0 - "Community 0"
Cohesion: 0.06
Nodes (43): BOOLEAN, etw_session, fps, GUID, imgui, ping, socket, string (+35 more)

### Community 1 - "Community 1"
Cohesion: 0.09
Nodes (50): ImDrawList, ImVec2, Gap(), Capture, EtwStatus, string, Menu, Draw (+42 more)

### Community 2 - "Community 2"
Cohesion: 0.07
Nodes (38): ImDrawList, ImFont, ImU32, ImVec2, string, ImFont, string, IconBars() (+30 more)

### Community 3 - "Community 3"
Cohesion: 0.06
Nodes (37): clock, Corner, cstdio, cstdlib, filesystem, fstream, shlobj, Config (+29 more)

### Community 4 - "Community 4"
Cohesion: 0.07
Nodes (30): ImDrawData, ImDrawVert, ImTextureData, ImFont, FontFace, bold, boldRes, medium (+22 more)

### Community 5 - "Community 5"
Cohesion: 0.05
Nodes (37): App, animating_, appliedTheme_, capture_, cfg_, cfgDirty_, data_, etw_ (+29 more)

### Community 6 - "Community 6"
Cohesion: 0.07
Nodes (34): cstdint, cstring, ApplyImGuiStyle(), ImVec4, Hex(), Lerp(), Theme, accent (+26 more)

### Community 7 - "Community 7"
Cohesion: 0.09
Nodes (29): algorithm, d3d11, dcomp, dxgi1_2, ID3D11Device, ID3D11DeviceContext, ID3D11RenderTargetView, IDCompositionDevice (+21 more)

### Community 8 - "Community 8"
Cohesion: 0.11
Nodes (30): DWORD, LONGLONG, vector, FpsProvider, AverageFps, Fresh, IsPresenting, kRing (+22 more)

### Community 9 - "Community 9"
Cohesion: 0.09
Nodes (23): condition_variable, deque, map, atomic, DWORD, mutex, thread, PingProvider (+15 more)

### Community 10 - "Community 10"
Cohesion: 0.14
Nodes (14): HINSTANCE, RECT, HWND, RECT, OverlayWindow, Create, Destroy, ReassertTopmost (+6 more)

### Community 11 - "Community 11"
Cohesion: 0.15
Nodes (16): BOOL, game_select, LONG, DWORD, FpsProvider, HWND, LPARAM, string (+8 more)

### Community 12 - "Community 12"
Cohesion: 0.16
Nodes (16): icmpapi, iphlpapi, BYTE, DWORD, PEVENT_RECORD, string, EndpointText(), IsPrivate() (+8 more)

### Community 13 - "Community 13"
Cohesion: 0.19
Nodes (12): HMONITOR, GameEntry, exe, fps, pid, GameSelector, candidates_, exe_ (+4 more)

### Community 14 - "Community 14"
Cohesion: 0.17
Nodes (13): PWSTR, MsUntilNextTick, Render, Run, StartCapture, Tick, TrayAdd, TrayRemove (+5 more)

### Community 15 - "Community 15"
Cohesion: 0.18
Nodes (8): anim, cmath, ImGuiID, Set(), Spring(), State, v, x

### Community 16 - "Community 16"
Cohesion: 0.26
Nodes (10): HotkeyId, Hotkey, mods, vk, HWND, Hotkeys, hwnd_, Register (+2 more)

### Community 17 - "Community 17"
Cohesion: 0.23
Nodes (8): imgui_impl_dx11, imgui_impl_win32, shellapi, CloseMenu, OpenMenu, SaveIfDirty, ToggleOverlay, TrayMenu

### Community 18 - "Community 18"
Cohesion: 0.48
Nodes (7): LRESULT, HandleMessage, CALLBACK App::WndProc(), HWND, LPARAM, UINT, WPARAM

### Community 19 - "Community 19"
Cohesion: 0.29
Nodes (7): PingState, string, Sample, PingSample, endpoint, ms, state

### Community 20 - "Community 20"
Cohesion: 0.33
Nodes (6): RefreshNames, string, wstring, Utf8ToWide(), WideToUtf8(), wchar_t

### Community 21 - "Community 21"
Cohesion: 0.33
Nodes (5): Endpoint, addr, family, array, BYTE

### Community 22 - "Community 22"
Cohesion: 0.33
Nodes (6): Layout, addrLen, addrOffset, known, pidOffset, ULONG

### Community 23 - "Community 23"
Cohesion: 0.60
Nodes (5): FinishCapture, RegisterHotkeys, string, HotkeyName(), KeyName()

### Community 24 - "Community 24"
Cohesion: 0.50
Nodes (4): ULONGLONG, Traffic, tcpBytes, udpBytes

## Knowledge Gaps
- **182 isolated node(s):** `FpsProvider`, `PingProvider`, `networkEnabled_`, `session_`, `thread_` (+177 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 288 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `App` connect `Community 5` to `Community 0`, `Community 1`, `Community 2`, `Community 3`, `Community 4`, `Community 6`, `Community 7`, `Community 10`, `Community 13`, `Community 14`, `Community 16`, `Community 17`, `Community 18`, `Community 23`?**
  _High betweenness centrality (0.336) - this node is a cross-community bridge._
- **Why does `PingProvider` connect `Community 9` to `Community 0`, `Community 12`, `Community 19`, `Community 21`, `Community 22`, `Community 24`?**
  _High betweenness centrality (0.127) - this node is a cross-community bridge._
- **Why does `Config` connect `Community 3` to `Community 0`, `Community 1`, `Community 4`, `Community 5`, `Community 16`?**
  _High betweenness centrality (0.105) - this node is a cross-community bridge._
- **What connects `FpsProvider`, `PingProvider`, `networkEnabled_` to the rest of the system?**
  _182 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.05505952380952381 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.08974358974358974 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.06852497096399536 - nodes in this community are weakly interconnected._