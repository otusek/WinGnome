#pragma once

#include "gfx/types.h"

#include <dwrite.h>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace wingnome {

class GfxFont {
public:
    GfxFont() = default;
    ~GfxFont();

    GfxFont(const GfxFont&) = delete;
    GfxFont& operator=(const GfxFont&) = delete;

    bool create(IDWriteFactory* factory, const std::wstring& familyName);
    void destroy();

    IDWriteTextFormat* format(float size) const;
    GfxSize measure(float size, const std::wstring& text, float maxWidth = 4096.f) const;

private:
    IDWriteFactory* factory_{nullptr};
    std::wstring familyName_;
    mutable std::unordered_map<int, IDWriteTextFormat*> formats_;

    IDWriteTextFormat* getOrCreateFormat(float size) const;
};

}  // namespace wingnome
