#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include "wallpaper.h"

#include <filesystem>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

namespace wingnome {

namespace {
ULONG_PTR g_gdiToken = 0;
ULONG g_gdiRefs = 0;

void acquireGdi() {
    if (g_gdiRefs++ == 0) {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&g_gdiToken, &input, nullptr);
    }
}

void releaseGdi() {
    if (g_gdiRefs > 0 && --g_gdiRefs == 0) {
        Gdiplus::GdiplusShutdown(g_gdiToken);
        g_gdiToken = 0;
    }
}
}  // namespace

class Wallpaper::GdiState {
public:
    GdiState() { acquireGdi(); }
    ~GdiState() { releaseGdi(); }
};

std::wstring Wallpaper::systemPath() {
    wchar_t path[MAX_PATH]{};
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0) && path[0] != L'\0')
        return path;
    return L"";
}

Wallpaper::Wallpaper() : gdi_(new GdiState()) {}

Wallpaper::~Wallpaper() {
    if (image_) {
        delete static_cast<Gdiplus::Image*>(image_);
        image_ = nullptr;
    }
    delete gdi_;
}

bool Wallpaper::load(const std::wstring& path) {
    if (image_) {
        delete static_cast<Gdiplus::Image*>(image_);
        image_ = nullptr;
        width_ = height_ = 0;
    }

    std::wstring resolved = path;
    if (resolved.empty()) resolved = systemPath();
    if (resolved.empty() || !std::filesystem::exists(resolved)) return false;

    auto* img = Gdiplus::Image::FromFile(resolved.c_str(), FALSE);
    if (!img || img->GetLastStatus() != Gdiplus::Ok) {
        delete img;
        return false;
    }

    image_ = img;
    width_ = static_cast<int>(img->GetWidth());
    height_ = static_cast<int>(img->GetHeight());
    return width_ > 0 && height_ > 0;
}

void Wallpaper::paint(HDC hdc, const RECT& dest) const {
    if (!image_) {
        HBRUSH brush = CreateSolidBrush(RGB(30, 30, 46));
        FillRect(hdc, &dest, brush);
        DeleteObject(brush);
        return;
    }

    const int destW = dest.right - dest.left;
    const int destH = dest.bottom - dest.top;
    if (destW <= 0 || destH <= 0) return;

    const double scaleW = static_cast<double>(destW) / width_;
    const double scaleH = static_cast<double>(destH) / height_;
    const double scale = scaleW > scaleH ? scaleW : scaleH;
    const int drawW = static_cast<int>(width_ * scale);
    const int drawH = static_cast<int>(height_ * scale);
    const int x = dest.left + (destW - drawW) / 2;
    const int y = dest.top + (destH - drawH) / 2;

    Gdiplus::Graphics gfx(hdc);
    gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    gfx.DrawImage(static_cast<Gdiplus::Image*>(image_), x, y, drawW, drawH);
}

}  // namespace wingnome
