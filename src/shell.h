#pragma once

#include "platform.h"
#include "bar.h"
#include "dock.h"
#include "desktop.h"
#include <windows.h>

namespace wingnome {

class Shell {
public:
    bool run(HINSTANCE instance);

private:
    Bar bar_;
    Dock dock_;
    Desktop desktop_;
};

}  // namespace wingnome
