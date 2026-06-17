#include "platform.h"
#include "window_tracker.h"

#include <windows.h>

#include <algorithm>
#include <unordered_map>

namespace wingnome {

bool WindowTracker::shouldSkip(HWND hwnd) const {
    if (!hwnd || !IsWindow(hwnd)) return true;
    if (!IsWindowVisible(hwnd)) return true;
    if (GetWindow(hwnd, GW_OWNER) != nullptr) return true;

    for (HWND ex : exclude_) {
        if (hwnd == ex) return true;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) return true;

    wchar_t className[256]{};
    GetClassNameW(hwnd, className, 256);
    if (wcscmp(className, L"Shell_TrayWnd") == 0) return true;
    if (wcscmp(className, L"Shell_SecondaryTrayWnd") == 0) return true;
    if (wcscmp(className, L"Progman") == 0) return true;
    if (wcscmp(className, L"WorkerW") == 0) return true;
    if (wcscmp(className, L"WinGnomeDesktop") == 0) return true;
    if (wcscmp(className, L"WinGnome Dock") == 0) return true;

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if ((style & WS_VISIBLE) == 0) return true;

    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return true;
    if (exStyle & WS_EX_NOACTIVATE) return true;

    if (GetWindowTextLengthW(hwnd) == 0) return true;

    return false;
}

std::wstring WindowTracker::exePathForWindow(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return {};

    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc) return {};

    wchar_t path[MAX_PATH]{};
    DWORD size = MAX_PATH;
    std::wstring result;
    if (QueryFullProcessImageNameW(proc, 0, path, &size)) {
        result = path;
    }
    CloseHandle(proc);
    return result;
}

void WindowTracker::refresh() {
    apps_.clear();
    std::unordered_map<std::wstring, size_t> index;
    const HWND focused = GetForegroundWindow();

    struct State {
        WindowTracker* tracker;
        std::unordered_map<std::wstring, size_t>* index;
        std::vector<RunningApp>* apps;
        HWND focused;
    } state{this, &index, &apps_, focused};

    EnumWindows(
        [](HWND hwnd, LPARAM param) -> BOOL {
            auto* s = reinterpret_cast<State*>(param);
            if (s->tracker->shouldSkip(hwnd)) return TRUE;

            const std::wstring path = WindowTracker::exePathForWindow(hwnd);
            if (path.empty()) return TRUE;

            auto it = s->index->find(path);
            if (it == s->index->end()) {
                RunningApp app;
                app.exePath = path;
                app.windows.push_back(hwnd);
                app.focused = hwnd == s->focused;
                (*s->index)[path] = s->apps->size();
                s->apps->push_back(std::move(app));
            } else {
                RunningApp& app = (*s->apps)[it->second];
                app.windows.push_back(hwnd);
                if (hwnd == s->focused) app.focused = true;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&state));

    std::sort(apps_.begin(), apps_.end(),
              [](const RunningApp& a, const RunningApp& b) {
                  return _wcsicmp(a.exePath.c_str(), b.exePath.c_str()) < 0;
              });
}

}  // namespace wingnome
