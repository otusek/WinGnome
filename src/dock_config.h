#pragma once

#include "platform.h"
#include "json.h"

#include <windows.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wingnome {

struct DockPerformanceConfig {
    int dataTickMs{1000};
    int idleSleepMs{100};
    int animFps{60};
};

struct DockConfig {
    bool enabled{true};
    int height{56};
    int iconSize{40};
    int iconSpacing{6};
    int paddingH{14};
    int cornerRadius{18};
    COLORREF background{RGB(36, 31, 49)};
    float backgroundOpacity{0.88f};
    bool backgroundBlur{false};
    bool transparentWindow{true};
    bool useAdwaitaIcons{true};
    bool autohide{false};
    int autohideDelayMs{350};
    bool showActivities{true};
    std::wstring activitiesCommand;
    std::vector<std::wstring> pinned;
    DockPerformanceConfig performance;

    static DockConfig load(const std::filesystem::path& path);

private:
    static std::optional<COLORREF> parseColor(const std::wstring& s);
    static std::vector<std::wstring> stringList(const JsonArray& arr);
};

}  // namespace wingnome
