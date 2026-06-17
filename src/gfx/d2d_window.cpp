#include "gfx/d2d_window.h"

#include "platform.h"

#include <windowsx.h>

#include <algorithm>
#include <cmath>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

namespace wingnome {

namespace {

constexpr wchar_t kWndClass[] = L"WinGnomeD2dWindow";

bool registerWindowClass() {
    static bool registered = false;
    if (registered) return true;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = D2dWindow::wndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWndClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    registered = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    return registered;
}

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMSBT_TABBEDWINDOW
#define DWMSBT_TABBEDWINDOW 4
#endif
#ifndef DWMSBT_NONE
#define DWMSBT_NONE 1
#endif

}  // namespace

D2dWindow::~D2dWindow() { destroy(); }

void D2dWindow::destroy() {
    discardDeviceResources();
    if (writeFactory_) {
        writeFactory_->Release();
        writeFactory_ = nullptr;
    }
    if (factory_) {
        factory_->Release();
        factory_ = nullptr;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool D2dWindow::create(const wchar_t* title, int x, int y, int width, int height, bool wantAcrylic,
                       bool layeredTransparent) {
    destroy();
    if (!registerWindowClass()) return false;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory_))) return false;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(&writeFactory_)))) {
        return false;
    }

    acrylic_ = wantAcrylic;
    layered_ = layeredTransparent;

    DWORD exStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    if (layered_) exStyle |= WS_EX_LAYERED;

    hwnd_ = CreateWindowExW(exStyle, kWndClass, title, WS_POPUP, x, y,
                            std::max(width, 1), std::max(height, 1), nullptr, nullptr,
                            GetModuleHandleW(nullptr), this);
    if (!hwnd_) return false;

    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    if (!layered_) enableAcrylic(acrylic_);
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return createDeviceResources();
}

void D2dWindow::setTopmost(int x, int y, int width, int height) {
    if (!hwnd_) return;
    LONG_PTR ex = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, ex | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);
}

void D2dWindow::enableAcrylic(bool enabled) {
    if (!hwnd_ || layered_) return;
    acrylic_ = enabled;

    const BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    if (enabled) {
        const int backdrop = DWMSBT_TABBEDWINDOW;
        DwmSetWindowAttribute(hwnd_, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));

        MARGINS margins{-1, -1, -1, -1};
        DwmExtendFrameIntoClientArea(hwnd_, &margins);
    } else {
        const int backdrop = DWMSBT_NONE;
        DwmSetWindowAttribute(hwnd_, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
    }
}

void D2dWindow::setRoundedRegion(int width, int height, float radius) {
    if (!hwnd_ || layered_) return;
    width = std::max(width, 1);
    height = std::max(height, 1);
    const float maxRadius = std::min(static_cast<float>(width), static_cast<float>(height)) * 0.5f;
    const int r = static_cast<int>(std::min(radius, maxRadius));
    if (r <= 0) {
        clearWindowRegion();
        return;
    }
    HRGN rgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, r * 2, r * 2);
    if (rgn) SetWindowRgn(hwnd_, rgn, TRUE);
}

void D2dWindow::clearWindowRegion() {
    if (!hwnd_) return;
    SetWindowRgn(hwnd_, nullptr, TRUE);
}

void D2dWindow::resize(int width, int height) {
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (layered_) {
        if (width == dibWidth_ && height == dibHeight_) return;
        discardDeviceResources();
        createLayeredResources(width, height);
        return;
    }
    if (!target_) return;
    ID2D1HwndRenderTarget* hwndTarget = nullptr;
    if (FAILED(target_->QueryInterface(IID_PPV_ARGS(&hwndTarget)))) return;
    D2D1_SIZE_U size{static_cast<UINT32>(width), static_cast<UINT32>(height)};
    hwndTarget->Resize(size);
    hwndTarget->Release();
}

GfxSize D2dWindow::clientSize() const {
    if (!hwnd_) return {};
    if (layered_) return {static_cast<float>(dibWidth_), static_cast<float>(dibHeight_)};
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    return {static_cast<float>(rc.right - rc.left), static_cast<float>(rc.bottom - rc.top)};
}

bool D2dWindow::createLayeredResources(int width, int height) {
    if (!factory_ || !hwnd_) return false;

    width = std::max(width, 1);
    height = std::max(height, 1);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(nullptr);
    dib_ = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &dibBits_, nullptr, 0);
    memDC_ = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    if (!dib_ || !memDC_ || !dibBits_) return false;

    SelectObject(memDC_, dib_);
    dibWidth_ = width;
    dibHeight_ = height;

    const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    if (FAILED(factory_->CreateDCRenderTarget(&props, &dcTarget_))) return false;

    RECT bind{0, 0, width, height};
    if (FAILED(dcTarget_->BindDC(memDC_, &bind))) return false;

    dcTarget_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    dcTarget_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    target_ = dcTarget_;
    return true;
}

bool D2dWindow::createDeviceResources() {
    if (!hwnd_ || target_) return target_ != nullptr;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const int w = std::max(1L, rc.right - rc.left);
    const int h = std::max(1L, rc.bottom - rc.top);

    if (layered_) return createLayeredResources(w, h);

    const D2D1_SIZE_U size{static_cast<UINT32>(w), static_cast<UINT32>(h)};
    ID2D1HwndRenderTarget* hwndTarget = nullptr;
    if (FAILED(factory_->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd_, size), &hwndTarget))) {
        return false;
    }

    hwndTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    hwndTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    target_ = hwndTarget;
    return true;
}

void D2dWindow::discardDeviceResources() {
    if (brush_) {
        brush_->Release();
        brush_ = nullptr;
    }
    if (dcTarget_) {
        dcTarget_->Release();
        dcTarget_ = nullptr;
    }
    if (target_ && !layered_) {
        target_->Release();
    }
    target_ = nullptr;
    if (memDC_) {
        DeleteDC(memDC_);
        memDC_ = nullptr;
    }
    if (dib_) {
        DeleteObject(dib_);
        dib_ = nullptr;
    }
    dibBits_ = nullptr;
    dibWidth_ = 0;
    dibHeight_ = 0;
}

void D2dWindow::presentLayered() {
    if (!hwnd_ || !layered_ || !memDC_) return;

    RECT wr{};
    GetWindowRect(hwnd_, &wr);
    POINT ptDst{wr.left, wr.top};
    SIZE size{dibWidth_, dibHeight_};
    POINT ptSrc{0, 0};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    HDC screen = GetDC(nullptr);
    UpdateLayeredWindow(hwnd_, screen, &ptDst, &size, memDC_, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

D2D1_COLOR_F D2dWindow::toD2dColor(GfxColor c) const {
    return D2D1::ColorF(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
}

ID2D1SolidColorBrush* D2dWindow::brush(GfxColor color) {
    if (!target_) return nullptr;
    if (brush_ && brushColor_.r == color.r && brushColor_.g == color.g &&
        brushColor_.b == color.b && brushColor_.a == color.a) {
        return brush_;
    }
    if (brush_) brush_->Release();
    if (FAILED(target_->CreateSolidColorBrush(toD2dColor(color), &brush_))) return nullptr;
    brushColor_ = color;
    return brush_;
}

bool D2dWindow::beginDraw(GfxColor clear) {
    if (!target_ && !createDeviceResources()) return false;
    target_->BeginDraw();
    if (acrylic_ && !layered_) {
        clear.a = static_cast<uint8_t>(std::min<int>(clear.a, 210));
    }
    target_->Clear(toD2dColor(clear));
    return true;
}

void D2dWindow::endDraw() {
    if (!target_) return;
    const HRESULT status = target_->EndDraw();
    if (status == D2DERR_RECREATE_TARGET) {
        discardDeviceResources();
        createDeviceResources();
    }
    if (layered_) presentLayered();
}

void D2dWindow::fillRoundedRect(const GfxRect& rect, float radius, GfxColor color) {
    if (!target_) return;
    ID2D1SolidColorBrush* b = brush(color);
    if (!b) return;

    const D2D1_RECT_F r{rect.left, rect.top, rect.left + rect.width, rect.top + rect.height};
    const float rad = std::min(radius, std::min(rect.width, rect.height) * 0.5f);
    if (rad <= 0.f) {
        target_->FillRectangle(r, b);
        return;
    }
    D2D1_ROUNDED_RECT rr{r, rad, rad};
    target_->FillRoundedRectangle(rr, b);
}

void D2dWindow::fillEllipse(const GfxRect& rect, GfxColor color) {
    if (!target_) return;
    ID2D1SolidColorBrush* b = brush(color);
    if (!b) return;
    const D2D1_ELLIPSE ellipse{
        {rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f},
        rect.width * 0.5f,
        rect.height * 0.5f,
    };
    target_->FillEllipse(ellipse, b);
}

void D2dWindow::drawBitmap(ID2D1Bitmap* bitmap, const GfxRect& dest, float opacity) {
    if (!target_ || !bitmap) return;
    const D2D1_RECT_F dst{dest.left, dest.top, dest.left + dest.width, dest.top + dest.height};
    target_->DrawBitmap(bitmap, dst, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

void D2dWindow::drawText(const GfxFont& font, float size, const std::wstring& text, float x,
                         float y, GfxColor color, float maxWidth) {
    if (!target_ || text.empty()) return;
    IDWriteTextFormat* fmt = font.format(size);
    if (!fmt) return;

    IDWriteTextLayout* layout = nullptr;
    if (FAILED(writeFactory_->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.size()), fmt,
                                               maxWidth, 512.f, &layout))) {
        return;
    }

    ID2D1SolidColorBrush* b = brush(color);
    if (b) target_->DrawTextLayout(D2D1::Point2F(x, y), layout, b);
    layout->Release();
}

GfxSize D2dWindow::measureText(const GfxFont& font, float size, const std::wstring& text) const {
    return font.measure(size, text);
}

bool D2dWindow::pollEvents() {
    bool activity = false;
    MSG msg{};
    while (PeekMessageW(&msg, hwnd_, 0, 0, PM_REMOVE)) {
        activity = true;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return activity;
}

LRESULT CALLBACK D2dWindow::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    D2dWindow* self = reinterpret_cast<D2dWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<D2dWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
        case WM_NCHITTEST:
            if (self->layered_) return HTCLIENT;
            break;
        case WM_SIZE:
            if (!self->layered_) {
                self->resize(static_cast<int>(LOWORD(lp)), static_cast<int>(HIWORD(lp)));
            }
            break;
        case WM_MOUSEMOVE:
            self->mousePos_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            break;
        case WM_LBUTTONDOWN:
            self->mouseDown_ = true;
            self->mouseClicked_ = true;
            self->mousePos_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            break;
        case WM_LBUTTONUP:
            self->mouseDown_ = false;
            break;
        case WM_MOUSEWHEEL:
            self->mouseWheelDelta_ = GET_WHEEL_DELTA_WPARAM(wp);
            self->mousePos_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, reinterpret_cast<POINT*>(&self->mousePos_));
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace wingnome
