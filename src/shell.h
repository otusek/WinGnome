#pragma once

#include "platform.h"
#include "bar.h"
#include "bar_config.h"
#include "dock.h"
#include "desktop.h"
#include "paths.h"

#include <windows.h>

#include <filesystem>

namespace wingnome {

class Shell {
public:
    bool run(HINSTANCE instance);

private:
    Bar bar_;
    Dock dock_;
    Desktop desktop_;
    HINSTANCE instance_{nullptr};
    std::filesystem::path shellConfigPath_;
    ShellConfig shellCfg_;
    ConfigChangeDetect shellConfigWatch_{};
    bool desktopActive_{false};
    int lastBarHeight_{0};

    void checkShellConfigChanged();
    void reloadShellConfig();
    void syncDesktopLayout();
    void updateExcludeHwnds();
};

}  // namespace wingnome
