#include "app_icons.h"

#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <algorithm>
#include <cstdint>

#pragma comment(lib, "ole32.lib")

namespace wingnome {

namespace {

bool endsWith(const std::wstring& s, const std::wstring& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::wstring toLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

}  // namespace

AppIconCache::~AppIconCache() { clear(); }

size_t AppIconCache::KeyHash::operator()(const Key& k) const {
    return std::hash<std::wstring>{}(k.path) ^ static_cast<size_t>(k.size) * 1315423911u;
}

void AppIconCache::clear() {
    for (auto& [_, entry] : cache_) {
        if (entry.bitmap) entry.bitmap->Release();
    }
    cache_.clear();
}

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

bool AppIconCache::loadPixels(const std::wstring& path, int size,
                              std::vector<uint8_t>& out) const {
    const std::wstring resolved = resolveAppPath(path);
    if (resolved.empty()) return false;

    SHFILEINFOW sfi{};
    if (SHGetFileInfoW(resolved.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON) == 0) {
        return false;
    }

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
        DestroyIcon(sfi.hIcon);
        return false;
    }

    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ old = SelectObject(mem, dib);
    ReleaseDC(nullptr, screen);

    RECT fill{0, 0, size, size};
    FillRect(mem, &fill, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    DrawIconEx(mem, 0, 0, sfi.hIcon, size, size, 0, nullptr, DI_NORMAL);
    DestroyIcon(sfi.hIcon);

    out.resize(static_cast<size_t>(size) * static_cast<size_t>(size) * 4);
    const auto* px = static_cast<const uint8_t*>(bits);
    for (size_t i = 0; i < out.size(); i += 4) {
        out[i + 0] = px[i + 2];
        out[i + 1] = px[i + 1];
        out[i + 2] = px[i + 0];
        out[i + 3] = px[i + 3];
    }

    SelectObject(mem, old);
    DeleteObject(dib);
    DeleteDC(mem);
    return true;
}

ID2D1Bitmap* AppIconCache::ensureBitmap(ID2D1RenderTarget* rt, Entry& entry) {
    if (!rt || entry.pixels.empty()) return nullptr;
    if (entry.bitmap && entry.owner == rt) return entry.bitmap;

    if (entry.bitmap) {
        entry.bitmap->Release();
        entry.bitmap = nullptr;
    }

    const UINT32 size = static_cast<UINT32>(entry.size);
    const D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    if (FAILED(rt->CreateBitmap(D2D1::SizeU(size, size), entry.pixels.data(), size * 4, props,
                                &entry.bitmap))) {
        return nullptr;
    }
    entry.owner = rt;
    return entry.bitmap;
}

ID2D1Bitmap* AppIconCache::get(ID2D1RenderTarget* rt, const std::wstring& path, int size) {
    if (!rt || path.empty()) return nullptr;

    const Key key{resolveAppPath(path), size};
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        Entry entry;
        entry.size = size;
        if (!loadPixels(key.path, size, entry.pixels)) return nullptr;
        auto [inserted, _] = cache_.emplace(key, std::move(entry));
        it = inserted;
    }

    return ensureBitmap(rt, it->second);
}

}  // namespace wingnome
