#pragma once

#include "platform.h"

#include <windows.h>

namespace wingnome {

class FrameTimer {
public:
    FrameTimer() {
        QueryPerformanceFrequency(&freq_);
        restart();
    }

    void restart() { QueryPerformanceCounter(&last_); }

    double elapsedSeconds() const {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        return static_cast<double>(now.QuadPart - last_.QuadPart) /
               static_cast<double>(freq_.QuadPart);
    }

    long long elapsedMilliseconds() const {
        return static_cast<long long>(elapsedSeconds() * 1000.0);
    }

private:
    LARGE_INTEGER freq_{};
    LARGE_INTEGER last_{};
};

}  // namespace wingnome
