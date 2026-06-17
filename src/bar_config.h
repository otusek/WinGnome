#pragma once

#include "platform.h"
#include "json.h"

#include <windows.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wingnome {

struct PerformanceConfig {
    int dataTickMs{1000};
    int idleSleepMs{100};
    int animFps{60};
};

struct BarConfig {
    int height{32};
    COLORREF background{RGB(36, 31, 49)};
    COLORREF foreground{RGB(255, 255, 255)};
    std::wstring font{L"Segoe UI"};
    int fontSize{11};
    std::wstring clockFormat{L"%a %d %b"};
    std::vector<std::wstring> modulesLeft{};
    std::vector<std::wstring> modulesCenter{L"clock"};
    std::vector<std::wstring> modulesRight{};
    PerformanceConfig performance;

    static BarConfig load(const std::filesystem::path& path);

private:
    static std::optional<COLORREF> parseColor(const std::wstring& s);
    static std::vector<std::wstring> moduleNames(const JsonArray& arr);
};

struct ShellConfig {
    std::wstring wallpaper;
    bool desktopEnabled{true};

    static ShellConfig load(const std::filesystem::path& path);
};

}  // namespace wingnome
