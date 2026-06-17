#include "svg_icons.h"

#include "paths.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace wingnome {

namespace {

std::string readFileUtf8(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void tintPixels(std::vector<uint8_t>& pixels, GfxColor tint) {
    for (size_t i = 0; i + 3 < pixels.size(); i += 4) {
        const uint8_t a = pixels[i + 3];
        if (a == 0) continue;
        pixels[i + 0] = tint.b;
        pixels[i + 1] = tint.g;
        pixels[i + 2] = tint.r;
        pixels[i + 3] = static_cast<uint8_t>(a * tint.a / 255);
    }
}

void premultiplyPixels(std::vector<uint8_t>& pixels) {
    for (size_t i = 0; i + 3 < pixels.size(); i += 4) {
        const uint8_t a = pixels[i + 3];
        pixels[i + 0] = static_cast<uint8_t>(pixels[i + 0] * a / 255);
        pixels[i + 1] = static_cast<uint8_t>(pixels[i + 1] * a / 255);
        pixels[i + 2] = static_cast<uint8_t>(pixels[i + 2] * a / 255);
    }
}

}  // namespace

SvgIconCache::~SvgIconCache() { clear(); }

size_t SvgIconCache::KeyHash::operator()(const Key& k) const {
    return std::hash<std::wstring>{}(k.name) ^ static_cast<size_t>(k.size) * 1315423911u ^
           (static_cast<size_t>(k.tint.r) << 16) ^ (static_cast<size_t>(k.tint.g) << 8) ^
           k.tint.b ^ k.tint.a ^ (k.tinted ? 0x9e3779b9u : 0u);
}

void SvgIconCache::clear() {
    for (auto& [_, entry] : cache_) {
        if (entry.bitmap) entry.bitmap->Release();
    }
    cache_.clear();
}

bool SvgIconCache::loadPixels(const std::wstring& assetName, int size, GfxColor tint, bool tinted,
                              std::vector<uint8_t>& out) const {
    const std::filesystem::path path = findAssetFile(assetName.c_str());
    const std::string svg = readFileUtf8(path);
    if (svg.empty()) return false;

    NSVGimage* image = nsvgParse(const_cast<char*>(svg.c_str()), "px", 96.f);
    if (!image) return false;

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return false;
    }

    const float scale = static_cast<float>(size) / std::max(image->height, image->width);
    const int w = static_cast<int>(image->width * scale);
    const int h = static_cast<int>(image->height * scale);
    if (w <= 0 || h <= 0) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return false;
    }

    out.assign(static_cast<size_t>(w) * static_cast<size_t>(h) * 4, 0);
    nsvgRasterize(rast, image, 0.f, 0.f, scale, out.data(), w, h, w * 4);

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    if (w != size || h != size) {
        std::vector<uint8_t> square(static_cast<size_t>(size) * static_cast<size_t>(size) * 4, 0);
        const int ox = (size - w) / 2;
        const int oy = (size - h) / 2;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const size_t src = (static_cast<size_t>(y) * static_cast<size_t>(w) + x) * 4;
                const size_t dst =
                    (static_cast<size_t>(y + oy) * static_cast<size_t>(size) + x + ox) * 4;
                square[dst + 0] = out[src + 0];
                square[dst + 1] = out[src + 1];
                square[dst + 2] = out[src + 2];
                square[dst + 3] = out[src + 3];
            }
        }
        out = std::move(square);
    }

    if (tinted) {
        tintPixels(out, tint);
    } else {
        premultiplyPixels(out);
    }
    return true;
}

ID2D1Bitmap* SvgIconCache::ensureBitmap(ID2D1RenderTarget* rt, Entry& entry) {
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

ID2D1Bitmap* SvgIconCache::get(ID2D1RenderTarget* rt, const std::wstring& assetName, int size,
                               GfxColor tint, bool tinted) {
    if (!rt || assetName.empty() || size <= 0) return nullptr;

    const Key key{assetName, size, tint, tinted};
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        Entry entry;
        entry.size = size;
        if (!loadPixels(assetName, size, tint, tinted, entry.pixels)) return nullptr;
        auto [inserted, _] = cache_.emplace(key, std::move(entry));
        it = inserted;
    }

    return ensureBitmap(rt, it->second);
}

}  // namespace wingnome
