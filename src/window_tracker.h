#pragma once

#include "platform.h"

#include <string>
#include <vector>

namespace wingnome {

struct RunningApp {
    std::wstring path;
    std::vector<HWND> windows;
    bool focused{false};
};

class WindowTracker {
public:
    void setExcludeHwnds(const std::vector<HWND>& hwnds);
    void refresh();
    const std::vector<RunningApp>& apps() const { return apps_; }
    void addWindow(HWND hwnd, const std::wstring& path);

private:
    std::vector<HWND> exclude_;
    std::vector<RunningApp> apps_;
};

}  // namespace wingnome
