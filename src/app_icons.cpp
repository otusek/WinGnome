#include "platform.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>

#include "app_icons.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace wingnome {

namespace {

std::wstring toLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

bool endsWith(const std::wstring& s, const std::wstring& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool hiconToTexture(HICON icon, int size, sf::Texture& out) {
    if (!icon) return false;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP dib = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        ReleaseDC(nullptr, screen);
        return false;
    }

    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ old = SelectObject(mem, dib);
    ReleaseDC(nullptr, screen);

    RECT fill{0, 0, size, size};
    FillRect(mem, &fill, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    DrawIconEx(mem, 0, 0, icon, size, size, 0, nullptr, DI_NORMAL);

    sf::Image image;
    image.create(static_cast<unsigned>(size), static_cast<unsigned>(size));
    const auto* px = static_cast<const uint8_t*>(bits);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int i = (y * size + x) * 4;
            image.setPixel(static_cast<unsigned>(x), static_cast<unsigned>(y),
                           sf::Color(px[i + 2], px[i + 1], px[i + 0], px[i + 3]));
        }
    }

    SelectObject(mem, old);
    DeleteObject(dib);
    DeleteDC(mem);

    return out.loadFromImage(image);
}

}  // namespace

std::wstring resolveAppPath(const std::wstring& path) {
    if (path.empty()) return {};
    if (!endsWith(toLower(path), L".lnk")) return path;

    IShellLinkW* link = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                reinterpret_cast<void**>(&link)))) {
        return path;
    }

    IPersistFile* file = nullptr;
    if (FAILED(link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&file)))) {
        link->Release();
        return path;
    }

    std::wstring resolved = path;
    if (SUCCEEDED(file->Load(path.c_str(), STGM_READ))) {
        wchar_t target[MAX_PATH]{};
        if (SUCCEEDED(link->GetPath(target, MAX_PATH, nullptr, SLGP_UNCPRIORITY)) && target[0]) {
            resolved = target;
        }
    }

    file->Release();
    link->Release();
    return resolved;
}

bool loadIconTexture(const std::wstring& path, int size, sf::Texture& out) {
    const std::wstring resolved = resolveAppPath(path);
    if (resolved.empty()) return false;

    SHFILEINFOW sfi{};
    const UINT flags = SHGFI_ICON | SHGFI_LARGEICON;
    if (SHGetFileInfoW(resolved.c_str(), 0, &sfi, sizeof(sfi), flags) == 0) return false;

    const bool ok = hiconToTexture(sfi.hIcon, size, out);
    DestroyIcon(sfi.hIcon);
    return ok;
}

sf::Texture* AppIconCache::get(const std::wstring& path, int size) {
    const Key key{resolveAppPath(path), size};
    auto it = cache_.find(key);
    if (it != cache_.end()) return &it->second;

    sf::Texture tex;
    if (!loadIconTexture(key.path, size, tex)) return nullptr;
    tex.setSmooth(true);
    auto [inserted, _] = cache_.emplace(key, std::move(tex));
    return &inserted->second;
}

void AppIconCache::clear() { cache_.clear(); }

}  // namespace wingnome
