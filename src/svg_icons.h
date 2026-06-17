#pragma once

#include "gfx/types.h"

#include <d2d1.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace wingnome {

class SvgIconCache {
public:
    ~SvgIconCache();
    void clear();

    ID2D1Bitmap* get(ID2D1RenderTarget* rt, const std::wstring& assetName, int size,
                     GfxColor tint = {}, bool tinted = true);

private:
    struct Key {
        std::wstring name;
        int size{0};
        GfxColor tint{};
        bool tinted{true};

        bool operator==(const Key& o) const {
            return size == o.size && name == o.name && tinted == o.tinted && tint.r == o.tint.r &&
                   tint.g == o.tint.g && tint.b == o.tint.b && tint.a == o.tint.a;
        }
    };

    struct KeyHash {
        size_t operator()(const Key& k) const;
    };

    struct Entry {
        int size{0};
        std::vector<uint8_t> pixels;
        ID2D1Bitmap* bitmap{nullptr};
        ID2D1RenderTarget* owner{nullptr};
    };

    bool loadPixels(const std::wstring& assetName, int size, GfxColor tint, bool tinted,
                    std::vector<uint8_t>& out) const;
    ID2D1Bitmap* ensureBitmap(ID2D1RenderTarget* rt, Entry& entry);

    std::unordered_map<Key, Entry, KeyHash> cache_;
};

}  // namespace wingnome
