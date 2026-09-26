# External Overlay — Research & Plan

> **Status (2026-09-26): v1 implemented.** Stack: C++20, Win32, D3D11 + DirectComposition, Dear ImGui 1.92.9.
> v1 scope as built: watermark (FPS + 1% low, ping, clock, username), keybinds (menu/show-hide), animated
> menu with theme and font pickers. See README.md for usage and `src/` for code. Config is a small
> key=value file instead of JSON (no JSON dependency needed). Inter and Fira Code weren't added because only
> variable/unbuilt font files were available; they can be added later as static TTFs.

Goal: a **Neverlose / Orbit / Krypton-style HUD** (watermark bar, indicator panels) that runs as a
**separate program**, never touches the game process, and is therefore in the same safety class
as Discord's overlay, Crosshair X or NVIDIA's overlay.

Reference look (Neverlose watermark):

```
NL | 219 FPS 167 (1%L) | 59% GPU | 34% CPU | 4.77 GHz | jokingtim | 11:44 AM | [avatar]
```

---

## 1. Research findings

### 1.1 What the cheat UIs actually are
- Neverlose, Orbit, Krypton, etc. are **internal** cheats: a DLL injected into `cs2.exe`, drawing
  with the game's own DirectX device (usually a Dear ImGui fork). The watermark, keybind list,
  spectator list and indicators are drawn *inside* the game.
- That injection (plus reading/writing game memory) is what anti-cheat detects — **not** the look
  of the UI. A purely cosmetic HUD drawn by an outside program is a different category.
- Neverlose's UI style: dark translucent pills, thin accent line/glow, small caps font, icon + value
  segments separated by dividers, rounded corners, subtle blur. A Figma kit exists
  ("Neverlose CS2: Recurrence Design") that can be used as a visual reference.

### 1.2 Precedent for "external + not banned"
- **Crosshair X**: topmost window overlay, no DLL injection, no memory hooks, no input simulation;
  ~3M installs, no ban reports across VAC, FACEIT, EAC, BattlEye, Vanguard, Ricochet.
- **Intel PresentMon**: gets per-game FPS and frame times *without injection*, using Windows ETW
  (Event Tracing for Windows) present events. Open source (GameTechDev/PresentMon).
- **Counter-example: RTSS / MSI Afterburner** draw via hooking into the game's Present call.
  Some anti-cheats dislike this. We avoid that approach entirely.

### 1.3 Where each number comes from (no injection, no kernel driver)
| Segment | Source | Notes |
|---|---|---|
| FPS + 1% low | ETW present events (PresentMon approach) for the foreground game PID | Needs admin **or** membership in "Performance Log Users". 1% low = avg of slowest 1% of frame times over a rolling window. |
| GPU % | PDH counters `\GPU Engine(*engtype_3D)\Utilization Percentage` | Same data Task Manager shows. No driver. |
| CPU % | PDH `\Processor Information(_Total)\% Processor Utility` | Matches Task Manager. |
| CPU GHz | PDH `% Processor Performance` × base MHz (from `CallNtPowerInformation`) | This is how Task Manager gets live clock. **Do not** use LibreHardwareMonitor — its WinRing0 driver is on Microsoft's vulnerable-driver blocklist and is flagged by kernel anti-cheats. |
| Username | `GetUserNameW` or a custom name in settings | |
| Time | system clock | |
| Avatar | Steam avatar via Steam Web API, or local image | Optional. |
| Game info (optional) | **CS2 Game State Integration (GSI)** — Valve-official HTTP push used by tournament HUDs | Health, armor, money, round phase, bomb state, map, score. Fully sanctioned. |
| Now playing (optional) | Windows SMTC (`GlobalSystemMediaTransportControlsSessionManager`) | Spotify/YouTube/any media app — fits this repo's music-player roots. |

Not possible without cheating (so we skip them): spectator list, enemy info, in-game ping
from game memory, anything that reads `cs2.exe` memory.

### 1.4 How we avoid bans (the rules)
1. Never call `OpenProcess` on the game with read/write rights; never read or write game memory.
2. No DLL injection, no hooks, no kernel drivers (including third-party sensor drivers).
3. No input simulation, no screen capture or pixel reading of the game.
4. Be a normal, visible app: normal window class/title, a tray icon, no hiding from the task list.
   Do **not** hijack other overlays' windows (e.g. the NVIDIA overlay) — a known cheat trick.
5. Don't hide from capture (`WDA_EXCLUDEFROMCAPTURE`) by default. Cheats use it to hide from
   streams, so it looks suspicious.
6. Code-sign releases when possible.

Caveat: no one can *guarantee* an anti-cheat won't change its rules. Following the Crosshair X /
Discord model keeps us in the category anti-cheats already accept.

**Display mode limitation:** a separate overlay window only shows over **borderless/windowed**
fullscreen, not exclusive fullscreen. CS2 players commonly run borderless; we'll document this.

---

## 2. Proposed architecture

**Recommended stack:** C++20, Win32 + DirectComposition + Direct3D 11 + Dear ImGui (custom-styled).
- ImGui is what these cheat menus are built on, so matching the look is easiest.
- DirectComposition gives true per-pixel transparency with low overhead. The window is
  `WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE`, so it's click-through.
- Alternative if you prefer: C# (.NET 8) + WPF. Easier, heavier, harder to match the look exactly.

```
overlay.exe
├── core/
│   ├── window        transparent click-through topmost window, DPI aware, multi-monitor
│   ├── renderer      D3D11 + DirectComposition + ImGui, capped redraw (~30–60 Hz)
│   └── config        JSON settings (layout, colors, enabled modules), hot-reload
├── providers/        each one produces values; no provider touches the game
│   ├── fps_etw       ETW present events → FPS, frame time, 1% low
│   ├── pdh_sensors   CPU %, CPU GHz, GPU %, VRAM, RAM
│   ├── game_detect   foreground window → which exe is the "game"
│   ├── cs2_gsi       local HTTP listener for CS2 GSI (optional)
│   └── media_smtc    now-playing title/artist/art (optional)
├── modules/          what gets drawn (Neverlose-style widgets)
│   ├── watermark     the bar from the screenshot
│   ├── indicators    small stat pills (e.g. frametime graph)
│   ├── now_playing   music widget
│   └── gsi_panel     CS2 info panel (health/money/bomb timer)
└── ui/
    ├── theme         Neverlose / Orbit / Krypton presets (colors, glow, rounding, font)
    └── menu          settings menu, toggled by hotkey (window becomes clickable while open)
```

---

## 2.5 Visual design rules (anti-vibe pass)

Applied before any UI code exists, so the overlay doesn't end up looking like a generic ImGui demo
or a generic AI-generated overlay.

**Tells we will not ship**
- Purple/violet or indigo-to-pink accents. Each theme preset gets its accent from the real cheat it
  imitates: Neverlose's cool blue/cyan, and the Orbit and Krypton colors taken from their menus.
- ImGui's default font (ProggyClean) or a default sans. Use one bundled typeface with **tabular
  figures**, so "219 FPS" doesn't jitter width every frame. A narrow grotesk for labels is fine;
  numbers are the priority.
- Every widget with the same rounding and shadow. The watermark is a single pill. Indicator panels
  are squarer and sit lower. The settings menu is the only surface with real elevation.
- Text separators (`|`, `·`) between segments. Use an icon plus value per segment with thin
  hairline dividers, as in the Neverlose screenshot.
- Decorative gradients. The only gradient allowed is the 1–2 px accent line/glow that Neverlose
  uses to mark the active widget.

**Glass, done honestly**
- ImGui can't blur what's behind a window. Real blur would need DWM acrylic
  (`DWMWA_SYSTEMBACKDROP_TYPE`, Windows 11 only), and it would force the game to be composited, which
  adds latency. So:
  - **Default:** "fake glass". A translucent dark fill (~70–80% opacity), a 1 px light top-edge
    highlight, a soft shadow, and a very faint noise texture. It's readable over any game scene.
  - **Optional (off by default, Win11 only):** real acrylic behind the watermark only, with a warning
    about latency.
- Contrast check against the worst case: bright CS2 skyboxes (Dust2 at midday) and flashbangs.
  Text must stay readable at full white behind it. Fill opacity has a floor the user can't go below.

**Depth and motion (one moment, not many)**
- Z-levels: indicator panels (flat) → watermark (lifted) → settings menu (highest, with dimmed backdrop).
- The one motion moment: when the settings menu opens, the watermark segments slide into their new
  positions with spring easing. Everything else changes instantly; no fading every number.
- Values never animate. An overlay that tweens its FPS number is lying about the FPS.

**Domain-specific behaviour instead of placeholders**
- FPS segment color follows thresholds tied to the monitor refresh rate (e.g. at 240 Hz, amber
  below 240 and red below 144), not fixed generic colors.
- 1% low sits next to the average FPS, because stutter matters more than the average.
- A frametime sparkline in the indicator module shows a stutter as a visible spike.
- Empty and error states say what to do. For example, "FPS needs admin or the Performance Log
  Users group — open settings to fix", not "N/A".
- Demo mode for screenshots uses realistic numbers (CS2 on a 4.8 GHz 8-core at 240 Hz), not zeros.

---

## 3. Milestones

1. **M0 – Skeleton**: CMake project, transparent click-through topmost window, ImGui renders a static bar.
2. **M1 – Watermark with system stats**: CPU %, CPU GHz, GPU %, clock, username (PDH only).
3. **M2 – FPS via ETW**: FPS + 1% low for the foreground game; graceful "N/A" without permissions.
4. **M3 – Theming**: Neverlose preset first, then Orbit/Krypton-like presets; draggable positions.
5. **M4 – Settings menu + config**: hotkey toggle, enable/disable segments, colors, save/load JSON.
6. **M5 – Extras**: now-playing (SMTC), CS2 GSI panel, frametime graph.
7. **M6 – Hardening & release**: performance budget (<1% CPU), signed build, installer, docs on
   borderless mode and on staying anti-cheat friendly.

## 4. Open questions for you
- Stack: C++/ImGui (recommended) or C#/WPF?
- Which games first: CS2 only, or anything (Valorant/FACEIT users care most about safety)?
- Want the optional modules (now playing, CS2 GSI) in v1, or watermark only?
- Keep building in this repo (QOLMusicPlayer) or a new repo?
