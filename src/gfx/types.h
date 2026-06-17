#pragma once

#include <cstdint>

namespace wingnome {

struct GfxColor {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};
};

struct GfxVec2 {
    float x{0.f};
    float y{0.f};
};

struct GfxVec2i {
    int x{0};
    int y{0};
};

struct GfxSize {
    float width{0.f};
    float height{0.f};
};

struct GfxRect {
    float left{0.f};
    float top{0.f};
    float width{0.f};
    float height{0.f};

    bool contains(float x, float y) const {
        return x >= left && x < left + width && y >= top && y < top + height;
    }
};

inline GfxColor gfxFromColorref(uint32_t rgb, uint8_t alpha = 255) {
    return GfxColor{
        static_cast<uint8_t>(rgb & 0xFF),
        static_cast<uint8_t>((rgb >> 8) & 0xFF),
        static_cast<uint8_t>((rgb >> 16) & 0xFF),
        alpha,
    };
}

}  // namespace wingnome
