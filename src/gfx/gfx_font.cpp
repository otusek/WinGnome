#include "gfx/gfx_font.h"

#include <cmath>

namespace wingnome {

GfxFont::~GfxFont() { destroy(); }

void GfxFont::destroy() {
    for (auto& [_, fmt] : formats_) {
        if (fmt) fmt->Release();
    }
    formats_.clear();
    factory_ = nullptr;
}

bool GfxFont::create(IDWriteFactory* factory, const std::wstring& familyName) {
    destroy();
    if (!factory || familyName.empty()) return false;
    factory_ = factory;
    familyName_ = familyName;
    return true;
}

IDWriteTextFormat* GfxFont::getOrCreateFormat(float size) const {
    if (!factory_) return nullptr;
    const int key = static_cast<int>(std::lround(size * 10.f));
    const auto it = formats_.find(key);
    if (it != formats_.end()) return it->second;

    IDWriteTextFormat* fmt = nullptr;
    if (FAILED(factory_->CreateTextFormat(
            familyName_.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"pl-pl", &fmt))) {
        return nullptr;
    }
    fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    formats_[key] = fmt;
    return fmt;
}

IDWriteTextFormat* GfxFont::format(float size) const { return getOrCreateFormat(size); }

GfxSize GfxFont::measure(float size, const std::wstring& text, float maxWidth) const {
    if (!factory_ || text.empty()) return {};
    IDWriteTextFormat* fmt = getOrCreateFormat(size);
    if (!fmt) return {};

    IDWriteTextLayout* layout = nullptr;
    if (FAILED(factory_->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.size()), fmt,
                                       maxWidth, 512.f, &layout))) {
        return {};
    }

    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    layout->Release();
    return {metrics.width, metrics.height};
}

}  // namespace wingnome
