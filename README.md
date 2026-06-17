# WinGnome

> Yes, the project is vibecoded. Yes, I know what I'm doing. Yes, the code is still a mess. Yes, I will optimize it.

A custom **Windows 11** shell inspired by GNOME. Replaces the default `explorer.exe` with a custom desktop, wallpaper, top bar, and dock.

## Project goals

WinGnome provides a minimal, extensible graphical shell:

- **Desktop** — fullscreen desktop window below the bar
- **Wallpaper** — system wallpaper or a custom image path
- **Bar** — configurable top panel (modules: clock, battery, network, volume, etc.)
- **Dock** — Dash to Dock-style bottom panel (pinned and running apps, autohide)

The project is in MVP stage — next steps are desktop icons, a context menu, and full Explorer replacement.

## Requirements

- Windows 11 (also works on Windows 10)
- [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload
- [CMake](https://cmake.org/download/) 3.16+
- Windows SDK 10.0.26100 or newer

## Directory structure

```
WinGnome/
├── CMakeLists.txt          # build configuration
├── README.md
├── config/
│   ├── bar.json            # bar configuration (formerly waybar.json)
│   ├── dock.json           # dock configuration
│   └── shell.json          # wallpaper and desktop settings
└── src/
    ├── main.cpp            # entry point (wWinMain)
    ├── shell.cpp           # component orchestration
    ├── gfx/                # Direct2D window, fonts, frame timer
    ├── bar.cpp             # top bar and modules
    ├── bar_modules.cpp     # bar module implementations
    ├── dock.cpp            # bottom dock
    ├── svg_icons.cpp       # SVG → ID2D1Bitmap (nanosvg)
    ├── desktop.cpp         # desktop window
    ├── wallpaper.cpp       # wallpaper loading and rendering
    └── paths.cpp           # config and asset path resolution
```

## Getting started

### 1. Clone and build

```powershell
git clone <repo-url> WinGnome
cd WinGnome
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

On VS 2022 use `-G "Visual Studio 17 2022"` instead.

If you previously built with SFML, delete the old `build\` folder first so CMake does not reuse stale artifacts.

### 2. Run (without replacing the shell)

```powershell
.\build\Release\wingnome.exe
```

`config\bar.json`, `config\dock.json`, and `config\shell.json` are copied automatically to `build\Release\config\`.

### 3. Wallpaper configuration

Edit `config\shell.json`:

```json
{
  "wallpaper": "C:\\Users\\You\\Pictures\\wallpaper.jpg",
  "desktop_enabled": 1
}
```

An empty `"wallpaper": ""` uses the current Windows system wallpaper.

### 4. Bar configuration

Edit `config\bar.json` — use `modules-left`, `modules-center`, and `modules-right` to choose bar modules (similar to Waybar on Linux). Supported module names: `activities`, `clock`, `battery`, `network`, `volume`, `system`, `spacer`, `text/<label>`. Changes are picked up automatically when the file is saved.

### 5. Dock configuration

Edit `config\dock.json` — pinned apps, icon size, autohide, and visual settings.

## Installing as the shell (Windows 11)

> **Note:** Replacing the shell requires a registry backup. Test by running the exe normally first.

1. Build the project in **Release** configuration.
2. Copy `wingnome.exe` and the `config\` folder to a permanent location, e.g. `C:\WinGnome\`.
3. Open **Registry Editor** (`regedit`).
4. Go to:
   ```
   HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion\Winlogon
   ```
   or (shell for the current session):
   ```
   HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer
   ```
5. Set the `Shell` value to the full path of `wingnome.exe`.
6. Sign out and sign back in.

To restore Explorer:
- Remove the `Shell` value or set it to `explorer.exe`, then restart the session.

## Testing

1. Run `wingnome.exe` — you should see the desktop wallpaper, top bar, and dock.
2. Check bar modules: clock, battery, network.
3. Change `config\bar.json` (e.g. background color or module list) and verify the bar updates without restarting.
4. Set a custom wallpaper in `config\shell.json` and verify it displays.
5. Hover the bottom screen edge to show the dock (if autohide is enabled).

## Troubleshooting

### `wingnome.exe` fails to start (`0xc0000006` or similar)

This usually means you are running an **old executable** from before the Direct2D migration, or a copy of `wingnome.exe` without its `config\` folder next to it.

1. Rebuild from a clean tree:
   ```powershell
   Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
   cmake -B build -G "Visual Studio 18 2026" -A x64
   cmake --build build --config Release
   ```
2. Run from the build output directory (config is copied automatically):
   ```powershell
   .\build\Release\wingnome.exe
   ```
3. The current build uses **Direct2D** and only Windows system DLLs. The MSVC runtime is linked **statically** — no `sfml-*.dll` or VC++ Redistributable install is required.

## Technology choices

The project uses **C++17 + Win32 API + Direct2D / DirectWrite** — hardware-accelerated rendering with native Windows integration (wallpaper, audio, network, battery).

## License

See the project repository.
