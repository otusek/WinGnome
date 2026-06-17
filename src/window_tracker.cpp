#include "window_tracker.h"

#include <windows.h>

namespace wingnome {

namespace {

struct EnumData {
    WindowTracker* tracker;
    const std::vector<HWND>* exclude;
};

BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM param) {
    auto* data = reinterpret_cast<EnumData*>(param);
    WindowTracker* self = data->tracker;
    if (!IsWindowVisible(hwnd)) return TRUE;

    wchar_t title[256]{};
    if (GetWindowTextW(hwnd, title, 256) == 0) return TRUE;

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if ((style & WS_CHILD) != 0) return TRUE;

    for (HWND excluded : *data->exclude) {
        if (hwnd == excluded) return TRUE;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return TRUE;

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return TRUE;

    wchar_t path[MAX_PATH]{};
    DWORD pathLen = MAX_PATH;
    std::wstring exePath;
    if (QueryFullProcessImageNameW(process, 0, path, &pathLen)) exePath = path;
    CloseHandle(process);

    if (exePath.empty()) return TRUE;

    self->addWindow(hwnd, exePath);
    return TRUE;
}

}  // namespace

void WindowTracker::setExcludeHwnds(const std::vector<HWND>& hwnds) { exclude_ = hwnds; }

void WindowTracker::refresh() {
    apps_.clear();
    EnumData data{this, &exclude_};
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));

    HWND fg = GetForegroundWindow();
    for (auto& app : apps_) {
        for (HWND hwnd : app.windows) {
            if (hwnd == fg) {
                app.focused = true;
                break;
            }
        }
    }
}

void WindowTracker::addWindow(HWND hwnd, const std::wstring& path) {
    for (auto& app : apps_) {
        if (_wcsicmp(app.path.c_str(), path.c_str()) == 0) {
            app.windows.push_back(hwnd);
            return;
        }
    }
    RunningApp app;
    app.path = path;
    app.windows.push_back(hwnd);
    apps_.push_back(std::move(app));
}

}  // namespace wingnome
