#pragma once

#include "platform.h"

#include <d2d1.h>

#include <string>
#include <unordered_map>

namespace wingnome {

std::wstring resolveAppPath(const std::wstring& path);

class AppIconCache {
public:
    ~AppIconCache();

    ID2D1Bitmap* get(ID2D1RenderTarget* rt, const std::wstring& path, int size);
    void clear();

private:
    struct Key {
        std::wstring path;
        int size{0};
        bool operator==(const Key& o) const { return size == o.size && path == o.path; }
    };

    struct KeyHash {
        size_t operator()(const Key& k) const;
    };

    struct Entry {
        std::vector<uint8_t> pixels;
        int size{0};
        ID2D1Bitmap* bitmap{nullptr};
        ID2D1RenderTarget* owner{nullptr};
    };

    bool loadPixels(const std::wstring& path, int size, std::vector<uint8_t>& out) const;
    ID2D1Bitmap* ensureBitmap(ID2D1RenderTarget* rt, Entry& entry);

    std::unordered_map<Key, Entry, KeyHash> cache_;
};

}  // namespace wingnome
