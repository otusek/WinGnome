#pragma once

#include <windows.h>

#include <string>

namespace wingnome {

class Wallpaper {
public:
    Wallpaper();
    ~Wallpaper();

    Wallpaper(const Wallpaper&) = delete;
    Wallpaper& operator=(const Wallpaper&) = delete;

    bool load(const std::wstring& path);
    void paint(HDC hdc, const RECT& dest) const;
    static std::wstring systemPath();

private:
    class GdiState;
    GdiState* gdi_{nullptr};
    void* image_{nullptr};
    int width_{0};
    int height_{0};
};

}  // namespace wingnome
