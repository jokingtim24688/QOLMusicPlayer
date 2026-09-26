# QOL Overlay

A Neverlose-style watermark and settings menu for games that runs as its own program. It never touches the
game process, so it stays in the same safety class as Discord's overlay or Crosshair X.

```
QOL | ▮▮▮ 219 FPS 167 1% low | ⌔ 23 ms | ◷ 11:44 AM | ● jokingtim
```

## Features
- **FPS + 1% low** of the selected game, measured with Windows event tracing (the method Intel PresentMon uses).
- **Ping**: detects the server or relay the game is talking to and measures the round trip to it.
- **Clock and date** in your Windows time zone (12/24-hour, optional seconds, five date styles).
- **Pick what's on the bar** (Bar tab): FPS, frame time, ping, game name, time in game, date, clock, CPU %,
  GPU %, RAM %, your name. Add, remove and reorder them.
- **Spotify player**: cover art, title, artist, progress, previous / play-pause / next. Drag it anywhere while
  the menu is open. It reads Windows' media controls, so there's no Spotify login.
- **Menu** with animations: position and look, bar contents, music, game selection, theme, font, keybinds, updates.
- **Auto-update**: checks GitHub every few hours, verifies the download's SHA-256, and installs only while no
  game is running. The overlay restarts by itself.
- **Themes**: Catppuccin (Mocha, Macchiato, Frappé, Latte), Midnight, Neverlose, Tokyo Night, Dracula, Nord,
  Gruvbox Dark, Rosé Pine, One Dark.
- **Fonts**: Geist, Geist Mono, JetBrains Mono, IBM Plex Sans, Space Grotesk, Manrope.
- **Low overhead**: while the menu is closed it redraws about once a second and only covers the watermark's rectangle.

## Install
Paste this into **PowerShell**:

```powershell
$f="$env:TEMP\QOLOverlay-Setup.exe"; irm https://github.com/jokingtim24688/QOLMusicPlayer/releases/latest/download/QOLOverlay-Setup.exe -OutFile $f; Start-Process $f
```

The installer puts QOL Overlay in Program Files and adds a desktop shortcut and a Start menu entry. It launches
the overlay when you click Finish. Accept the admin prompt: event tracing for FPS and ping needs it.
Updates install automatically (About tab to turn that off or check now). Running the command again also
updates. Uninstall it from Windows Settings → Apps.

Play in **borderless** or **windowed** mode. Exclusive fullscreen hides every overlay.

| Key | Action |
|---|---|
| `Delete` | Open / close the menu |
| `End` | Show / hide the overlay |
| `Esc` | Close the menu (or cancel a keybind change) |

Both binds can be changed in the menu. The tray icon also has open, show/hide and exit.
Settings live in `%APPDATA%\QOLOverlay\config.ini`.

## Why it doesn't get you banned
- It never opens, reads or writes the game's memory, and never injects or hooks anything.
- It uses no kernel drivers and no keyboard hooks (binds use `RegisterHotKey`).
- It doesn't simulate input or capture the screen.
- It's a normal visible window with a tray icon, and it doesn't hide from screen capture.

No tool can promise an anti-cheat will never change its rules, but this is the model other overlays already
use without bans.

## Known limits
- Ping is the network round trip to the game's server or relay. If that host ignores ping requests, the
  overlay says so rather than showing a made-up number.
- FPS covers DirectX 9–12 games. Vulkan/OpenGL games that don't present through DXGI show no FPS yet.

## Build
- **Windows (MSVC):** `cmake -S . -B build -A x64 && cmake --build build --config Release`
- **Installer:** `iscc /DAppVersion=0.2.0 installer\QOLOverlay.iss` ([Inno Setup 6](https://jrsoftware.org/isinfo.php)).
  Pushing a `v*` tag builds it in CI and publishes it as a GitHub Release.
- **Linux cross-compile check:** `cmake -B build-mingw -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake && cmake --build build-mingw`
- **UI preview without Windows:** `cmake -S tools/preview -B build-preview && cmake --build build-preview && ./build-preview/preview <out-dir>`
  renders the real menu and watermark to images.

Fonts are under the SIL Open Font License (see `assets/fonts/licenses`).
