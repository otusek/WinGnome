#pragma once

#include "platform.h"
#include "bar_config.h"
#include "json.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wingnome {

struct DockConfig {
    bool enabled{true};
    int height{56};
    int iconSize{40};
    int iconSpacing{6};
    int paddingH{14};
    int paddingV{8};
    int marginBottom{10};
    int cornerRadius{18};
    COLORREF background{RGB(36, 31, 49)};
    int backgroundAlpha{215};
    bool autohide{true};
    int autohideTriggerPx{4};
    bool showActivities{true};
    bool showRunningApps{true};
    std::wstring activitiesCommand;
    std::vector<std::wstring> pinned;
    PerformanceConfig performance;

    static DockConfig load(const std::filesystem::path& path);

private:
    static std::optional<COLORREF> parseColor(const std::wstring& s);
};

}  // namespace wingnome
