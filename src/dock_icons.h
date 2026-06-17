#pragma once

#include "app_icons.h"
#include "gfx/types.h"
#include "svg_icons.h"

#include <d2d1.h>

#include <string>

namespace wingnome {

class DockIconCache {
public:
    void setThemeEnabled(bool enabled) { themeEnabled_ = enabled; }
    bool themeEnabled() const { return themeEnabled_; }

    ID2D1Bitmap* get(ID2D1RenderTarget* rt, const std::wstring& appPath, int size);
    void clear();

private:
    bool themeEnabled_{true};
    AppIconCache systemIcons_;
    SvgIconCache themeIcons_;
};

std::wstring adwaitaIconAssetForApp(const std::wstring& appPath);

}  // namespace wingnome
