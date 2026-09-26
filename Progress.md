# Progress

## 2026-09-26 — Research & planning (no code yet)
- Repo was empty. Branch: `claude/vibrant-heisenberg-5xg2ip`.
- Researched Neverlose/Orbit/Krypton-style HUDs and how to build one **externally** without anti-cheat risk.
- Full findings and plan: [docs/PLAN.md](docs/PLAN.md).
- Key decisions (proposed, awaiting user confirmation):
  - Separate click-through topmost overlay window (Crosshair X / Discord model), no injection.
  - FPS + 1% low via ETW (PresentMon approach); CPU/GPU via PDH counters. No kernel drivers.
  - Recommended stack: C++20 + D3D11 + DirectComposition + Dear ImGui.
- graphify wiki not built yet: the repo has no code, and indexing the docs needs an LLM API key that this container doesn't have. It will be built once M0 code exists (code indexing needs no key).

## 2026-09-26 — Anti-vibe design pass (still no code)
- Added docs/PLAN.md §2.5: design rules applied up front. Covers banned tells, how to fake glass
  honestly in ImGui (optional real DWM acrylic, off by default), one motion moment, tabular
  figures, FPS colors tied to refresh rate, and actionable error states.

## 2026-09-26 — v1 built
- Stack: C++20, Win32, D3D11 + DirectComposition, Dear ImGui 1.92.9 (FetchContent), CMake. Static exe (~4 MB, 2 MB is fonts).
- Watermark: FPS + 1% low (ETW DXGI/D3D9 present events), ping (Kernel-Network ETW picks the game's busiest
  remote address, then ICMP), local clock, username. Tabular digits so numbers don't jitter.
- Menu (Insert): Overlay / Game / Theme / Font / Keybinds tabs. Spring animations: open/close zoom and fade,
  sliding tab highlight, content slide-in, toggle knobs, theme color crossfade, scroll fade.
- 12 themes (Catppuccin x4, Midnight, Neverlose, Tokyo Night, Dracula, Nord, Gruvbox Dark, Rosé Pine, One Dark),
  6 fonts (Geist, Geist Mono, JetBrains Mono, IBM Plex Sans, Space Grotesk, Manrope).
- Low resources: redraws ~1/s when the menu is closed, the window shrinks to the watermark's rectangle, vsync only while animating.
- Anti-cheat safety: no OpenProcess on the game (Toolhelp for names), no hooks, RegisterHotKey, no drivers.
- Verified: mingw-w64 cross-compile is clean with -Wall -Wextra -Werror. The UI is rendered to PNG via
  tools/preview (docs/screenshots/). Not yet run on real Windows: the user needs to test it.
- CI: .github/workflows/build.yml builds with MSVC on windows-latest and uploads QOLOverlay.exe.
- Name: placeholder "QOL Overlay" (APP_NAME_W in src/app_info.h). The user was asked to pick a name.

## 2026-09-26 — v0.2.0: Spotify player + installer
- Music player card: cover art (WIC decode → D3D11 texture), title/artist with ellipsis, progress, prev/play/next.
  Reads System Media Transport Controls via C++/WinRT on a background MTA thread polling once a second. Prefers
  Spotify's session; the "Spotify only" toggle can widen it to any media app.
- Draggable while the menu is open (position saved as a fraction of free screen space). Buttons only work with
  the menu open; the overlay stays click-through otherwise. The passive window now covers the union of the
  watermark and player.
- New Music tab. App icon (res/app.ico) used by the exe, tray, installer and shortcuts.
- Inno Setup installer (installer/QOLOverlay.iss): Program Files, desktop + Start menu shortcuts, launches when
  done, closes a running copy first. CI builds it; `v*` tags publish QOLOverlay-Setup.exe as a GitHub Release.
- mingw builds stub the media provider (no C++/WinRT there); the real code is compiled by MSVC in CI.

## Next
- User tests on Windows (CS2 borderless): FPS vs cl_showfps, ping vs scoreboard, CPU use at idle.
- Pick the final name.
- Later: more themes/fonts, Vulkan/OpenGL FPS (DxgKrnl events), frametime graph, draggable watermark.
