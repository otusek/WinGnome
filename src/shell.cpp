#include "platform.h"
#include "shell.h"

#include "json.h"
#include "paths.h"

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>

#include <algorithm>

namespace wingnome {

void Shell::updateExcludeHwnds() {
    std::vector<HWND> exclude{bar_.hwnd(), dock_.hwnd(), bar_.popupHwnd()};
    if (desktopActive_) exclude.push_back(desktop_.hwnd());
    dock_.setExcludeHwnds(exclude);
}

void Shell::syncDesktopLayout() {
    const int barHeight = bar_.height();
    if (barHeight == lastBarHeight_) return;
    lastBarHeight_ = barHeight;
    if (desktopActive_) desktop_.setTopInset(barHeight);
}

void Shell::reloadShellConfig() {
    shellConfigPath_ = findConfigFile(L"shell.json");
    const auto parsed = JsonParser::parseFile(shellConfigPath_);
    if (!parsed || parsed->type() != JsonValue::Type::Object) {
        configMarkReloaded(shellConfigWatch_, shellConfigPath_);
        return;
    }

    shellCfg_ = ShellConfig::load(shellConfigPath_);

    if (shellCfg_.desktopEnabled && !desktopActive_) {
        if (desktop_.create(instance_, bar_.height())) {
            desktopActive_ = true;
            desktop_.setWallpaperPath(shellCfg_.wallpaper);
            ShowWindow(desktop_.hwnd(), SW_SHOW);
            UpdateWindow(desktop_.hwnd());
        }
    } else if (!shellCfg_.desktopEnabled && desktopActive_) {
        desktop_.destroy();
        desktopActive_ = false;
    } else if (desktopActive_) {
        desktop_.setWallpaperPath(shellCfg_.wallpaper);
    }

    configMarkReloaded(shellConfigWatch_, shellConfigPath_);
    updateExcludeHwnds();
}

void Shell::checkShellConfigChanged() {
    if (!configNeedsReload(shellConfigWatch_, L"shell.json")) return;
    reloadShellConfig();
}

bool Shell::run(HINSTANCE instance) {
    instance_ = instance;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    shellConfigPath_ = findConfigFile(L"shell.json");
    shellCfg_ = ShellConfig::load(shellConfigPath_);
    configMarkReloaded(shellConfigWatch_, shellConfigPath_);

    if (!bar_.create()) {
        CoUninitialize();
        return false;
    }

    if (!dock_.create()) {
        bar_.destroy();
        CoUninitialize();
        return false;
    }

    lastBarHeight_ = bar_.height();
    updateExcludeHwnds();

    if (shellCfg_.desktopEnabled) {
        if (!desktop_.create(instance, bar_.height())) {
            dock_.destroy();
            bar_.destroy();
            CoUninitialize();
            return false;
        }
        desktopActive_ = true;
        desktop_.setWallpaperPath(shellCfg_.wallpaper);
        ShowWindow(desktop_.hwnd(), SW_SHOW);
        UpdateWindow(desktop_.hwnd());
        updateExcludeHwnds();
    }

    bool running = true;
    while (running && bar_.isOpen()) {
        const bool barActive = bar_.processFrame();
        const bool dockActive = dock_.processFrame();
        checkShellConfigChanged();
        syncDesktopLayout();
        updateExcludeHwnds();
        const bool active = barActive || dockActive;
        const DWORD idleMs =
            static_cast<DWORD>(std::min(bar_.idleSleepMs(), dock_.idleSleepMs()));
        const DWORD timeout = active ? 1u : idleMs;
        MsgWaitForMultipleObjects(0, nullptr, FALSE, timeout, QS_ALLINPUT);

        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            if (msg.message == WM_MOUSEWHEEL) {
                POINT pt{GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)};
                RECT rc{};
                GetWindowRect(bar_.hwnd(), &rc);
                if (PtInRect(&rc, pt)) {
                    bar_.handleMouseWheel(GET_WHEEL_DELTA_WPARAM(msg.wParam), pt);
                    continue;
                }
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (desktopActive_) desktop_.destroy();
    dock_.destroy();
    bar_.destroy();
    CoUninitialize();
    return true;
}

}  // namespace wingnome
