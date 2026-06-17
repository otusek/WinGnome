#pragma once

#include "wallpaper.h"

#include <windows.h>

#include <string>

namespace wingnome {

class Desktop {
public:
    bool create(HINSTANCE instance, int topInset);
    void destroy();
    HWND hwnd() const { return hwnd_; }

    void setWallpaperPath(const std::wstring& path);
    void setTopInset(int topInset);
    void relayout();

private:
    HINSTANCE instance_{};
    HWND hwnd_{};
    int topInset_{0};
    Wallpaper wallpaper_;

    static Desktop* from(HWND hwnd);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
};

}  // namespace wingnome
