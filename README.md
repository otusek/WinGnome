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
    ├── desktop.cpp         # desktop window
    ├── wallpaper.cpp       # wallpaper loading and rendering
    ├── bar.cpp             # top bar (formerly waybar)
    ├── bar_modules.cpp     # bar modules
    ├── bar_config.cpp      # configuration parser
    ├── json.cpp            # minimal JSON parser
    └── paths.cpp           # config and asset path resolution
```

## Getting started

### 1. Clone and build

```powershell
git clone <repo-url> WinGnome
cd WinGnome
cmake -B build
cmake --build build --config Release
```

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

Edit `config\bar.json` — use `modules-left`, `modules-center`, and `modules-right` sections (similar to Waybar on Linux).

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
3. Change `config\bar.json` (e.g. background color) and restart.
4. Set a custom wallpaper in `config\shell.json` and verify it displays.
5. Hover the bottom screen edge to show the dock (if autohide is enabled).

## Technology choices

The project uses **C++17 + Win32 API** — a solid fit for a Windows shell:

- direct access to Win32/COM (wallpaper, audio, network)
- a single native executable with no runtime dependency
- low latency and full control over windows

## License

See the project repository.
