#pragma once

#include "platform.h"
#include "json.h"

#include <windows.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wingnome {

struct VolumeAnimConfig {
    float hoverFadeMs{150.f};
    float sliderExpandMs{220.f};
    bool pulseOnMute{true};
    float pulseSpeedHz{1.5f};
};

struct VolumeConfig {
    float step{0.05f};
    int sliderWidth{90};
    int sliderHeight{6};
    bool scrollAdjust{true};
    bool dragAdjust{true};
    bool clickTogglesMute{false};
    VolumeAnimConfig animations;
};

struct AnimationsConfig {
    float frameMs{16.f};
    VolumeConfig volume;
};

struct PerformanceConfig {
    bool vsync{true};
    int dataTickMs{1000};
    int idleSleepMs{100};
    int animFps{30};
};

struct BarConfig {
    int height{32};
    COLORREF background{RGB(36, 31, 49)};
    COLORREF foreground{RGB(255, 255, 255)};
    std::wstring font{L"Segoe UI"};
    int fontSize{11};
    std::wstring clockFormat{L"%a %d %b"};
    std::wstring activitiesCommand;
    std::vector<std::wstring> modulesLeft{};
    std::vector<std::wstring> modulesCenter{L"clock"};
    std::vector<std::wstring> modulesRight{L"system"};
    AnimationsConfig animations;
    PerformanceConfig performance;

    static BarConfig load(const std::filesystem::path& path);

private:
    static std::optional<COLORREF> parseColor(const std::wstring& s);
    static std::vector<std::wstring> moduleNames(const JsonArray& arr);
    static VolumeConfig loadVolumeConfig(const JsonObject& root);
    static VolumeAnimConfig loadVolumeAnim(const JsonObject& obj);
};

struct ShellConfig {
    std::wstring wallpaper;
    bool desktopEnabled{true};

    static ShellConfig load(const std::filesystem::path& path);
};

}  // namespace wingnome
