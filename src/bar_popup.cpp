#include "bar_popup.h"

#include "shell_actions.h"

#include <windows.h>

#include <algorithm>

namespace wingnome {

namespace {

constexpr float kItemHeight = 36.f;
constexpr float kPadH = 12.f;
constexpr float kIconSize = 16.f;
constexpr float kWidth = 220.f;

}  // namespace

void BarPopup::buildItems() {
    items_ = {
        {L"Screenshot", L"icons/screenshot.svg", takeScreenshot},
        {L"Settings", L"icons/settings.svg", openSettings},
        {L"Log out", L"icons/logout.svg", logOff},
        {L"Restart", L"icons/restart.svg", restartSystem},
        {L"Power off", L"icons/power-off.svg", shutdownSystem},
    };
}

bool BarPopup::loadFont(const std::wstring& family) {
    if (!window_.writeFactory()) return false;
    return font_.create(window_.writeFactory(), family);
}

bool BarPopup::create() {
    buildItems();
    if (!window_.create(L"WinGnome Menu", 0, 0, static_cast<int>(kWidth),
                        static_cast<int>(items_.size() * kItemHeight), false)) {
        return false;
    }
    window_.enableAcrylic(false);
    ShowWindow(window_.hwnd(), SW_HIDE);
    open_ = true;
    visible_ = false;
    dirty_ = true;
    frameTimer_.restart();
    return true;
}

void BarPopup::destroy() {
    icons_.clear();
    font_.destroy();
    window_.destroy();
    open_ = false;
    visible_ = false;
}

void BarPopup::layoutItems() {
    itemBounds_.clear();
    itemBounds_.reserve(items_.size());
    for (size_t i = 0; i < items_.size(); ++i) {
        itemBounds_.push_back({0.f, static_cast<float>(i) * kItemHeight, kWidth, kItemHeight});
    }
}

void BarPopup::show(const GfxRect& anchor, COLORREF background, COLORREF foreground,
                    const std::wstring& fontFamily, int fontSize) {
    if (!open_) return;

    bg_ = gfxFromColorref(background, 245);
    fg_ = gfxFromColorref(foreground);
    fontSize_ = fontSize;
    loadFont(fontFamily);
    layoutItems();

    const int height = static_cast<int>(items_.size() * kItemHeight);
    const int x = static_cast<int>(anchor.left + anchor.width - kWidth);
    const int y = static_cast<int>(anchor.top + anchor.height);
    window_.setTopmost(x, y, static_cast<int>(kWidth), height);
    visible_ = true;
    hovered_ = -1;
    dirty_ = true;
    frameTimer_.restart();
}

void BarPopup::hide() {
    visible_ = false;
    hovered_ = -1;
    if (window_.hwnd()) ShowWindow(window_.hwnd(), SW_HIDE);
}

int BarPopup::itemAt(const GfxVec2i& pos) const {
    for (size_t i = 0; i < itemBounds_.size(); ++i) {
        const GfxRect& r = itemBounds_[i];
        if (r.contains(static_cast<float>(pos.x), static_cast<float>(pos.y))) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool BarPopup::pollEvents() {
    if (!visible_) return false;

    const bool activity = window_.pollEvents();
    const GfxVec2i pos = window_.mousePos();
    const int hover = itemAt(pos);
    if (hover != hovered_) {
        hovered_ = hover;
        dirty_ = true;
    }

    if (window_.mouseClicked()) {
        if (hovered_ >= 0 && hovered_ < static_cast<int>(items_.size())) {
            items_[static_cast<size_t>(hovered_)].action();
        }
        hide();
        window_.clearMouseClicked();
        dirty_ = true;
    }

    return activity || dirty_;
}

bool BarPopup::render() {
    if (!visible_ || !window_.isOpen()) return false;

    ID2D1RenderTarget* rt = window_.renderTarget();
    if (!rt) return false;

    window_.beginDraw(bg_);

    for (size_t i = 0; i < items_.size(); ++i) {
        const GfxRect& bounds = itemBounds_[i];
        if (static_cast<int>(i) == hovered_) {
            GfxColor hover = fg_;
            hover.a = 28;
            window_.fillRoundedRect(bounds, 6.f, hover);
        }

        GfxColor iconTint = fg_;
        if (ID2D1Bitmap* icon =
                icons_.get(rt, items_[i].icon, static_cast<int>(kIconSize), iconTint)) {
            const float iy = bounds.top + (bounds.height - kIconSize) * 0.5f;
            window_.drawBitmap(icon, {kPadH, iy, kIconSize, kIconSize});
        }

        const float textX = kPadH + kIconSize + 10.f;
        const float textY = bounds.top + (bounds.height - fontSize_) * 0.5f;
        window_.drawText(font_, static_cast<float>(fontSize_), items_[i].label, textX, textY, fg_);
    }

    window_.endDraw();
    dirty_ = false;
    return true;
}

bool BarPopup::processFrame() {
    if (!open_ || !visible_) return false;

    const bool hadInput = pollEvents();
    if (!dirty_ && !hadInput) return true;

    render();
    return true;
}

}  // namespace wingnome
