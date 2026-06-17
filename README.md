# WinGnome

Niestandardowa powłoka (shell) dla **Windows 11**, inspirowana środowiskiem GNOME. Zastępuje domyślny `explorer.exe` własnym pulpitem, tapetą i górnym paskiem (`bar`).

## Cel projektu

WinGnome dostarcza minimalną, rozszerzalną powłokę graficzną:

- **Desktop** — pełnoekranowe okno pulpitu pod paskiem
- **Wallpaper** — wyświetlanie tapety systemowej lub własnej ścieżki
- **Bar** — konfigurowalny górny pasek (moduły: zegar, bateria, sieć, głośność itd.)
- **Dock** — dolny pasek w stylu Dash to Dock (przypięte i uruchomione aplikacje, autoukrywanie)

Projekt jest w fazie MVP — kolejne kroki to ikony pulpitu, menu kontekstowe i pełna zamiana Explorera.

## Wymagania

- Windows 11 (działa też na Windows 10)
- [Visual Studio 2022](https://visualstudio.microsoft.com/) z workloadem **Desktop development with C++**
- [CMake](https://cmake.org/download/) 3.16+
- Windows SDK 10.0.26100 lub nowszy

## Struktura katalogów

```
WinGnome/
├── CMakeLists.txt          # konfiguracja builda
├── README.md
├── config/
│   ├── bar.json            # konfiguracja paska (poprzednio waybar.json)
│   ├── dock.json           # konfiguracja docka
│   └── shell.json          # tapeta i ustawienia pulpitu
└── src/
    ├── main.cpp            # punkt wejścia (wWinMain)
    ├── shell.cpp           # orchestracja komponentów
    ├── desktop.cpp         # okno pulpitu
    ├── wallpaper.cpp       # ładowanie i rysowanie tapety
    ├── bar.cpp             # górny pasek (poprzednio waybar)
    ├── bar_modules.cpp     # moduły paska
    ├── bar_config.cpp      # parser konfiguracji
    ├── json.cpp            # minimalny parser JSON
    └── paths.cpp           # ścieżki do plików konfiguracyjnych
```

## Instrukcja uruchomienia

### 1. Klonowanie i kompilacja

```powershell
git clone <repo-url> WinGnome
cd WinGnome
cmake -B build
cmake --build build --config Release
```

### 2. Uruchomienie (test bez zamiany shella)

```powershell
.\build\Release\wingnome.exe
```

Pliki `config\bar.json`, `config\dock.json` i `config\shell.json` są kopiowane automatycznie do `build\Release\config\`.

### 3. Konfiguracja tapety

Edytuj `config\shell.json`:

```json
{
  "wallpaper": "C:\\Users\\Ty\\Pictures\\tapeta.jpg",
  "desktop_enabled": 1
}
```

Puste `"wallpaper": ""` — używa aktualnej tapety systemowej Windows.

### 4. Konfiguracja paska

Edytuj `config\bar.json` — sekcje `modules-left`, `modules-center`, `modules-right` (jak w Waybarze na Linuxie).

## Instalacja jako shell (Windows 11)

> **Uwaga:** Zamiana shella wymaga kopii zapasowej rejestru. Testuj najpierw przez zwykłe uruchomienie exe.

1. Skompiluj projekt w konfiguracji **Release**.
2. Skopiuj `wingnome.exe` i folder `config\` do stałej lokalizacji, np. `C:\WinGnome\`.
3. Otwórz **Registry Editor** (`regedit`).
4. Przejdź do:
   ```
   HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion\Winlogon
   ```
   lub (shell dla bieżącej sesji):
   ```
   HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer
   ```
5. Ustaw wartość `Shell` na pełną ścieżkę do `wingnome.exe`.
6. Wyloguj się i zaloguj ponownie.

Aby przywrócić Explorer:
- Usuń wartość `Shell` lub ustaw `explorer.exe`, potem restart sesji.

## Testowanie

1. Uruchom `wingnome.exe` — powinien pojawić się pulpit z tapetą i górny pasek.
2. Sprawdź moduły na pasku: zegar, bateria, sieć.
3. Zmień `config\bar.json` (np. kolor tła) i uruchom ponownie.
4. Ustaw własną tapetę w `config\shell.json` i zweryfikuj wyświetlanie.

## Wybór technologii

Projekt używa **C++17 + Win32 API** — najlepszy wybór dla shella Windows:

- bezpośredni dostęp do Win32/COM (tapeta, audio, sieć)
- jeden natywny plik wykonywalny bez runtime
- niska latencja i pełna kontrola nad oknami

## Licencja

Zgodnie z repozytorium projektu.
