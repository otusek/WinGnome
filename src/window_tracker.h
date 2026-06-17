#pragma once

#include "platform.h"

#include <string>
#include <vector>

struct HWND__;
using HWND = HWND__*;

namespace wingnome {

struct RunningApp {
    std::wstring exePath;
    std::vector<HWND> windows;
    bool focused{false};
};

class WindowTracker {
public:
    void setExcludeHwnds(const std::vector<HWND>& hwnds) { exclude_ = hwnds; }
    void refresh();
    const std::vector<RunningApp>& apps() const { return apps_; }

private:
    std::vector<HWND> exclude_;
    std::vector<RunningApp> apps_;

    bool shouldSkip(HWND hwnd) const;
    static std::wstring exePathForWindow(HWND hwnd);
};

}  // namespace wingnome
