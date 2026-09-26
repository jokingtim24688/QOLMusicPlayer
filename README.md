# QOL Overlay

A Neverlose-style watermark and settings menu for games that runs as its own program. It never touches the
game process, so it stays in the same safety class as Discord's overlay or Crosshair X.

```
QOL | ▮▮▮ 219 FPS 167 1% low | ⌔ 23 ms | ◷ 11:44 AM | ● jokingtim
```

## Features
- **FPS + 1% low** of the selected game, measured with Windows event tracing (the method Intel PresentMon uses).
- **Ping**: detects the server or relay the game is talking to and measures the round trip to it.
- **Clock** in your Windows time zone (12/24-hour, optional seconds).
- **Menu** with animations: theme, font, which segments show, position, opacity, game selection, keybinds.
- **Themes**: Catppuccin (Mocha, Macchiato, Frappé, Latte), Midnight, Neverlose, Tokyo Night, Dracula, Nord,
  Gruvbox Dark, Rosé Pine, One Dark.
- **Fonts**: Geist, Geist Mono, JetBrains Mono, IBM Plex Sans, Space Grotesk, Manrope.
- **Low overhead**: while the menu is closed it redraws about once a second and only covers the watermark's rectangle.

## Use
1. Download `QOLOverlay.exe` from the latest successful **build** run under the repo's Actions tab (artifact
   `QOLOverlay`).
2. Run it and accept the admin prompt (event tracing for FPS and ping needs admin).
3. Play in **borderless** or **windowed** mode. Exclusive fullscreen hides every overlay.

| Key | Action |
|---|---|
| `Insert` | Open / close the menu |
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
- **Linux cross-compile check:** `cmake -B build-mingw -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake && cmake --build build-mingw`
- **UI preview without Windows:** `cmake -S tools/preview -B build-preview && cmake --build build-preview && ./build-preview/preview <out-dir>`
  renders the real menu and watermark to images.

Fonts are under the SIL Open Font License (see `assets/fonts/licenses`).
