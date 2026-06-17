#pragma once

#include "gfx/d2d_window.h"
#include "gfx/frame_timer.h"
#include "gfx/gfx_font.h"
#include "gfx/types.h"
#include "svg_icons.h"

#include <windows.h>

#include <vector>

namespace wingnome {

class BarPopup {
public:
    bool create();
    void destroy();
    bool isOpen() const { return open_; }
    bool isVisible() const { return visible_; }
    HWND hwnd() const { return window_.hwnd(); }

    void show(const GfxRect& anchor, COLORREF background, COLORREF foreground,
              const std::wstring& font, int fontSize);
    void hide();

    bool processFrame();

private:
    struct Item {
        std::wstring label;
        std::wstring icon;
        void (*action)();
    };

    bool open_{false};
    bool visible_{false};
    bool dirty_{true};
    int hovered_{-1};
  D2dWindow window_;
    GfxFont font_;
    SvgIconCache icons_;
    FrameTimer frameTimer_;
    GfxColor bg_{};
    GfxColor fg_{};
    int fontSize_{11};
    std::vector<Item> items_;
    std::vector<GfxRect> itemBounds_;

    bool loadFont(const std::wstring& family);
    void buildItems();
    void layoutItems();
    int itemAt(const GfxVec2i& pos) const;
    bool pollEvents();
    bool render();
};

}  // namespace wingnome
