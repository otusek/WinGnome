#include "platform.h"
#include "shell.h"

#include "bar_config.h"
#include "paths.h"

#include <windows.h>
#include <objbase.h>

#include <algorithm>

namespace wingnome {

bool Shell::run(HINSTANCE instance) {
    (void)instance;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    const ShellConfig shellCfg = ShellConfig::load(findConfigFile(L"shell.json"));

    if (!bar_.create()) {
        CoUninitialize();
        return false;
    }

    if (!dock_.create()) {
        bar_.destroy();
        CoUninitialize();
        return false;
    }

    std::vector<HWND> exclude{bar_.hwnd(), dock_.hwnd()};
    dock_.setExcludeHwnds(exclude);

    if (shellCfg.desktopEnabled) {
        if (!desktop_.create(instance, bar_.height())) {
            dock_.destroy();
            bar_.destroy();
            CoUninitialize();
            return false;
        }
        exclude.push_back(desktop_.hwnd());
        dock_.setExcludeHwnds(exclude);
        desktop_.setWallpaperPath(shellCfg.wallpaper);
        ShowWindow(desktop_.hwnd(), SW_SHOW);
        UpdateWindow(desktop_.hwnd());
    }

    bool running = true;
    while (running && bar_.isOpen()) {
        const bool barActive = bar_.processFrame();
        const bool dockActive = dock_.processFrame();
        const bool active = barActive || dockActive;
        const DWORD idleMs = static_cast<DWORD>(
            std::min(bar_.idleSleepMs(), dock_.idleSleepMs()));
        const DWORD timeout = active ? 1u : idleMs;
        MsgWaitForMultipleObjects(0, nullptr, FALSE, timeout, QS_ALLINPUT);

        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    desktop_.destroy();
    dock_.destroy();
    bar_.destroy();
    CoUninitialize();
    return true;
}

}  // namespace wingnome
