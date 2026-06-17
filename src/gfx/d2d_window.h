#pragma once

#include "gfx/gfx_font.h"
#include "gfx/types.h"

#include <d2d1.h>
#include <dwmapi.h>

namespace wingnome {

class D2dWindow {
public:
    D2dWindow() = default;
    ~D2dWindow();

    D2dWindow(const D2dWindow&) = delete;
    D2dWindow& operator=(const D2dWindow&) = delete;

    bool create(const wchar_t* title, int x, int y, int width, int height, bool wantAcrylic,
                bool layeredTransparent = false);
    void destroy();
    bool isOpen() const { return hwnd_ != nullptr; }
    HWND hwnd() const { return hwnd_; }

    void setTopmost(int x, int y, int width, int height);
    void enableAcrylic(bool enabled);
    void setRoundedRegion(int width, int height, float radius);
    void clearWindowRegion();
    void resize(int width, int height);
    GfxSize clientSize() const;

    bool beginDraw(GfxColor clear);
    void endDraw();

    void fillRoundedRect(const GfxRect& rect, float radius, GfxColor color);
    void fillEllipse(const GfxRect& rect, GfxColor color);
    void drawBitmap(ID2D1Bitmap* bitmap, const GfxRect& dest, float opacity = 1.f);
    void drawText(const GfxFont& font, float size, const std::wstring& text, float x, float y,
                  GfxColor color, float maxWidth = 4096.f);
    GfxSize measureText(const GfxFont& font, float size, const std::wstring& text) const;

    ID2D1Factory* factory() const { return factory_; }
    IDWriteFactory* writeFactory() const { return writeFactory_; }
    ID2D1RenderTarget* renderTarget() const { return target_; }

    bool pollEvents();
    GfxVec2i mousePos() const { return mousePos_; }
    bool mouseDown() const { return mouseDown_; }
    bool mouseClicked() const { return mouseClicked_; }
    void clearMouseClicked() { mouseClicked_ = false; }
    short mouseWheelDelta() const { return mouseWheelDelta_; }
    void clearMouseWheelDelta() { mouseWheelDelta_ = 0; }
    void injectMouseWheel(short delta, GfxVec2i clientPos) {
        mouseWheelDelta_ = delta;
        mousePos_ = clientPos;
    }

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    bool createDeviceResources();
    bool createLayeredResources(int width, int height);
    void discardDeviceResources();
    void presentLayered();
    ID2D1SolidColorBrush* brush(GfxColor color);
    D2D1_COLOR_F toD2dColor(GfxColor c) const;

    HWND hwnd_{nullptr};
    ID2D1Factory* factory_{nullptr};
    IDWriteFactory* writeFactory_{nullptr};
    ID2D1RenderTarget* target_{nullptr};
    ID2D1DCRenderTarget* dcTarget_{nullptr};
    ID2D1SolidColorBrush* brush_{nullptr};
    HDC memDC_{nullptr};
    HBITMAP dib_{nullptr};
    void* dibBits_{nullptr};
    int dibWidth_{0};
    int dibHeight_{0};
    GfxColor brushColor_{};
    bool acrylic_{false};
    bool layered_{false};
    GfxVec2i mousePos_{};
    bool mouseDown_{false};
    bool mouseClicked_{false};
    short mouseWheelDelta_{0};
};

}  // namespace wingnome
