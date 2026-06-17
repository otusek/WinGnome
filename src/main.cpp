#include "platform.h"
#include "shell.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    wingnome::Shell shell;
    return shell.run(instance) ? 0 : 1;
}
