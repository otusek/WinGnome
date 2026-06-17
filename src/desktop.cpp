#include "desktop.h"

namespace wingnome {

Desktop* Desktop::from(HWND hwnd) {
    return reinterpret_cast<Desktop*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

bool Desktop::create(HINSTANCE instance, int topInset) {
    instance_ = instance;
    topInset_ = topInset;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Desktop::wndProc;
    wc.hInstance = instance_;
    wc.lpszClassName = L"WinGnomeDesktop";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    const int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"WinGnome Desktop",
        WS_POPUP,
        0, topInset_, screenW, screenH - topInset_,
        nullptr, nullptr, instance_, this);
    if (!hwnd) return false;

    hwnd_ = hwnd;
    SetWindowPos(hwnd_, HWND_BOTTOM, 0, topInset_, screenW, screenH - topInset_, SWP_SHOWWINDOW);
    return true;
}

void Desktop::destroy() {
    if (!hwnd_) return;
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
}

void Desktop::setWallpaperPath(const std::wstring& path) {
    wallpaper_.load(path);
    if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

void Desktop::setTopInset(int topInset) {
    if (topInset_ == topInset) return;
    topInset_ = topInset;
    relayout();
}

void Desktop::relayout() {
    if (!hwnd_) return;
    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    const int screenH = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd_, HWND_BOTTOM, 0, topInset_, screenW, screenH - topInset_, SWP_SHOWWINDOW);
    InvalidateRect(hwnd_, nullptr, TRUE);
}

LRESULT CALLBACK Desktop::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    Desktop* self = from(hwnd);
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT client{};
        GetClientRect(hwnd, &client);
        self->wallpaper_.paint(hdc, client);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DISPLAYCHANGE:
        self->relayout();
        return 0;
    case WM_DESTROY:
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

}  // namespace wingnome
