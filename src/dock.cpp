#include "platform.h"
#include "dock.h"

#include "app_icons.h"
#include "json.h"
#include "paths.h"

#include <shellapi.h>

#include <algorithm>
#include <cmath>

namespace wingnome {

namespace {

constexpr float kSeparatorWidth = 12.f;

void focusWindow(HWND hwnd) {
    if (!hwnd) return;
    HWND fg = GetForegroundWindow();
    DWORD fgThread = GetWindowThreadProcessId(fg, nullptr);
    DWORD hwndThread = GetWindowThreadProcessId(hwnd, nullptr);
    DWORD curThread = GetCurrentThreadId();
    if (fgThread != curThread) AttachThreadInput(curThread, fgThread, TRUE);
    if (hwndThread != curThread) AttachThreadInput(curThread, hwndThread, TRUE);
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    AllowSetForegroundWindow(ASFW_ANY);
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    if (hwndThread != curThread) AttachThreadInput(curThread, hwndThread, FALSE);
    if (fgThread != curThread) AttachThreadInput(curThread, fgThread, FALSE);
}

GfxColor dockBgColor(const DockConfig& cfg, float alphaMul) {
    GfxColor c = gfxFromColorref(cfg.background);
    c.a = static_cast<uint8_t>(std::clamp(cfg.backgroundOpacity * alphaMul, 0.f, 1.f) * 255.f);
    return c;
}

}  // namespace

void Dock::markDirty() { dirty_ = true; }

int Dock::screenWidth() const { return GetSystemMetrics(SM_CXSCREEN); }

int Dock::screenHeight() const { return GetSystemMetrics(SM_CYSCREEN); }

bool Dock::usesLayeredWindow() const {
    return config_.transparentWindow && !config_.backgroundBlur;
}

bool Dock::recreateWindow() {
    if (!open_) return false;

    const GfxRect rect = dockScreenRect();
    const int w = std::max(static_cast<int>(rect.width), 64);
    const int h = static_cast<int>(rect.height);
    const int x = static_cast<int>(rect.left);
    const int y = static_cast<int>(rect.top);

    window_.destroy();
    icons_.clear();

    windowLayered_ = usesLayeredWindow();
    windowBlur_ = config_.backgroundBlur;
    if (!window_.create(L"WinGnome Dock", x, y, w, h, windowBlur_, windowLayered_)) {
        open_ = false;
        return false;
    }

    applyVisualStyle();
    applyTopmost();
    return true;
}

void Dock::applyVisualStyle() {
    if (!window_.isOpen()) return;

    window_.enableAcrylic(windowBlur_ && !windowLayered_);

    const GfxSize sz = window_.clientSize();
    const int w = static_cast<int>(sz.width);
    const int h = static_cast<int>(sz.height);
    if (windowBlur_ && !windowLayered_) {
        window_.setRoundedRegion(w, h, static_cast<float>(config_.cornerRadius));
    } else if (!windowLayered_) {
        window_.clearWindowRegion();
    }
    markDirty();
}

bool Dock::openWindow() {
    if (open_ || !config_.enabled) return open_;

    tracker_.refresh();
    rebuildItems();

    const int width = static_cast<int>(contentWidth());
    const int height = config_.height;
    const int x = (screenWidth() - width) / 2;
    const int y = screenHeight() - height;

    windowLayered_ = usesLayeredWindow();
    windowBlur_ = config_.backgroundBlur;
    if (!window_.create(L"WinGnome Dock", x, y, std::max(width, 64), height, windowBlur_,
                        windowLayered_)) {
        return false;
    }

    applyVisualStyle();
    icons_.setThemeEnabled(config_.useAdwaitaIcons);
    applyTopmost();

    visibility_.snap(config_.autohide ? 0.f : 1.f);
    if (!config_.autohide) visibility_.setTarget(1.f);

    open_ = true;
    applyPosition();
    dirty_ = true;
    return true;
}

void Dock::closeWindow() {
    if (!open_) return;
    icons_.clear();
    window_.destroy();
    open_ = false;
}

bool Dock::create() {
    configPath_ = findConfigFile(L"dock.json");
    reloadConfig();
    if (!config_.enabled) return true;
    if (!openWindow()) return false;

    frameTimer_.restart();
    animTimer_.restart();
    dataTimer_.restart();
    autohideTimer_.restart();
    return true;
}

void Dock::destroy() {
    closeWindow();
}

HWND Dock::hwnd() const { return window_.hwnd(); }

void Dock::setExcludeHwnds(const std::vector<HWND>& hwnds) {
    std::vector<HWND> all = hwnds;
    if (HWND self = hwnd()) all.push_back(self);
    tracker_.setExcludeHwnds(all);
}

void Dock::reloadConfig() {
    const bool prevLayered = windowLayered_;
    const bool prevBlur = windowBlur_;
    const bool prevAutohide = config_.autohide;
    const bool prevTheme = config_.useAdwaitaIcons;

    configPath_ = findConfigFile(L"dock.json");
    const auto parsed = JsonParser::parseFile(configPath_);
    if (!parsed || parsed->type() != JsonValue::Type::Object) {
        configMarkReloaded(configWatch_, configPath_);
        return;
    }

    config_ = DockConfig::load(configPath_);

    if (!config_.enabled) {
        closeWindow();
        configMarkReloaded(configWatch_, configPath_);
        return;
    }

    icons_.setThemeEnabled(config_.useAdwaitaIcons);
    if (prevTheme != config_.useAdwaitaIcons) icons_.clear();

    if (!open_) {
        openWindow();
        configMarkReloaded(configWatch_, configPath_);
        return;
    }

    const bool layered = usesLayeredWindow();
    const bool blur = config_.backgroundBlur;

    if (layered != prevLayered || blur != prevBlur) {
        recreateWindow();
    } else {
        applyVisualStyle();
    }

    if (prevAutohide != config_.autohide && !config_.autohide) {
        visibility_.setTarget(1.f);
    }

    tracker_.refresh();
    rebuildItems();
    configMarkReloaded(configWatch_, configPath_);
    dirty_ = true;
}

void Dock::checkConfigChanged() {
    if (!configNeedsReload(configWatch_, L"dock.json")) return;
    reloadConfig();
}

float Dock::contentWidth() const {
    if (!items_.empty()) {
        const auto& last = items_.back();
        return last.bounds.left + last.bounds.width + static_cast<float>(config_.paddingH);
    }

    int count = static_cast<int>(config_.pinned.size());
    if (config_.showActivities) ++count;
    if (count == 0) count = 1;
    const float icons = static_cast<float>(count);
    return config_.paddingH * 2.f + icons * static_cast<float>(config_.iconSize) +
           std::max(0.f, icons - 1.f) * static_cast<float>(config_.iconSpacing);
}

void Dock::rebuildItems() {
    items_.clear();

    if (config_.showActivities) {
        DockItem activities;
        activities.kind = ItemKind::Activities;
        activities.path = L"activities";
        items_.push_back(std::move(activities));
    }

    struct PinnedEntry {
        std::wstring launchPath;
        std::wstring exePath;
    };
    std::vector<PinnedEntry> pinned;
    pinned.reserve(config_.pinned.size());
    for (const auto& p : config_.pinned) {
        const std::wstring exePath = resolveAppPath(p);
        if (!exePath.empty()) pinned.push_back({p, exePath});
    }

    for (const auto& entry : pinned) {
        DockItem item;
        item.kind = ItemKind::App;
        item.path = entry.exePath;
        item.launchPath = entry.launchPath;
        item.pinned = true;
        items_.push_back(std::move(item));
    }

    auto markPinnedRunning = [&](const RunningApp& app) {
        for (auto& item : items_) {
            if (item.kind != ItemKind::App || !item.pinned) continue;
            if (_wcsicmp(item.path.c_str(), app.path.c_str()) != 0) continue;
            item.running = true;
            item.focused = app.focused;
            item.windows = app.windows;
            return true;
        }
        return false;
    };

    std::vector<RunningApp> unpinnedRunning;
    unpinnedRunning.reserve(tracker_.apps().size());
    for (const auto& app : tracker_.apps()) {
        if (markPinnedRunning(app)) continue;
        unpinnedRunning.push_back(app);
    }

    if (!pinned.empty() && !unpinnedRunning.empty()) {
        DockItem separator;
        separator.kind = ItemKind::Separator;
        items_.push_back(std::move(separator));
    }

    for (const auto& app : unpinnedRunning) {
        DockItem item;
        item.kind = ItemKind::App;
        item.path = app.path;
        item.launchPath = app.path;
        item.running = true;
        item.focused = app.focused;
        item.windows = app.windows;
        items_.push_back(std::move(item));
    }

    layoutItemBounds();
    if (open_) applyPosition();
}

void Dock::layoutItemBounds() {
    const float iconSize = static_cast<float>(config_.iconSize);
    const float spacing = static_cast<float>(config_.iconSpacing);
    const float pad = static_cast<float>(config_.paddingH);
    const float h = static_cast<float>(config_.height);
    float x = pad;

    for (auto& item : items_) {
        const float slotW = item.kind == ItemKind::Separator ? kSeparatorWidth : iconSize;
        item.bounds = {x, 0.f, slotW, h};
        x += slotW + spacing;
    }
}

GfxRect Dock::dockScreenRect() const {
    const float w = contentWidth();
    const float h = static_cast<float>(config_.height);
    const float x = (static_cast<float>(screenWidth()) - w) * 0.5f;
    const float vis = visibility_.value();
    const float y =
        static_cast<float>(screenHeight()) - h + (1.f - vis) * (h + 12.f);
    return {x, y, w, h};
}

void Dock::applyPosition() {
    if (!window_.isOpen()) return;
    const GfxRect rect = dockScreenRect();
    const int w = static_cast<int>(rect.width);
    const int h = static_cast<int>(rect.height);
    window_.resize(w, h);
    window_.setTopmost(static_cast<int>(rect.left), static_cast<int>(rect.top), w, h);
    if (windowBlur_ && !windowLayered_) {
        window_.setRoundedRegion(w, h, static_cast<float>(config_.cornerRadius));
    }
}

void Dock::applyTopmost() {
    if (!window_.isOpen()) return;
    const GfxRect rect = dockScreenRect();
    window_.setTopmost(static_cast<int>(rect.left), static_cast<int>(rect.top),
                       static_cast<int>(rect.width), static_cast<int>(rect.height));
}

void Dock::updateAutohide() {
    if (!config_.autohide) {
        if (visibility_.value() < 1.f) {
            visibility_.setTarget(1.f);
            markDirty();
        }
        return;
    }

    POINT cursor{};
    GetCursorPos(&cursor);
    const GfxRect dock = dockScreenRect();
    const float margin = 8.f;
    const bool inDock =
        static_cast<float>(cursor.x) >= dock.left - margin &&
        static_cast<float>(cursor.x) <= dock.left + dock.width + margin &&
        static_cast<float>(cursor.y) >= dock.top - margin &&
        static_cast<float>(cursor.y) <= static_cast<float>(screenHeight()) + margin;

    pointerNearDock_ = inDock;

    if (inDock) {
        autohideTimer_.restart();
        if (visibility_.value() < 1.f) {
            visibility_.setTarget(1.f);
            markDirty();
        }
        return;
    }

    if (autohideTimer_.elapsedMilliseconds() >= config_.autohideDelayMs && visibility_.value() > 0.f) {
        visibility_.setTarget(0.f);
        markDirty();
    }
}

void Dock::updateHover() {
    POINT cursor{};
    GetCursorPos(&cursor);
    const GfxRect dock = dockScreenRect();
    const GfxVec2i pos{
        cursor.x - static_cast<int>(dock.left),
        cursor.y - static_cast<int>(dock.top),
    };

    const int prev = hoveredIndex_;
    hoveredIndex_ = itemAt(pos);
    if (prev != hoveredIndex_) markDirty();
}

int Dock::itemAt(const GfxVec2i& pos) const {
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        const DockItem& item = items_[static_cast<size_t>(i)];
        if (item.kind == ItemKind::Separator) continue;
        if (item.bounds.contains(static_cast<float>(pos.x), static_cast<float>(pos.y))) {
            return i;
        }
    }
    return -1;
}

void Dock::launchApp(const std::wstring& launchPath) {
    if (launchPath.empty()) return;

    const HINSTANCE result =
        ShellExecuteW(nullptr, L"open", launchPath.c_str(), nullptr, nullptr, SW_SHOW);
    if (reinterpret_cast<intptr_t>(result) > 32) return;

    const std::wstring resolved = resolveAppPath(launchPath);
    if (resolved.empty() || _wcsicmp(resolved.c_str(), launchPath.c_str()) == 0) return;

    ShellExecuteW(nullptr, L"open", resolved.c_str(), nullptr, nullptr, SW_SHOW);
}

void Dock::focusApp(const DockItem& item) {
    HWND target = nullptr;
    for (HWND hwnd : item.windows) {
        if (!IsWindow(hwnd)) continue;
        if (hwnd == GetForegroundWindow()) return;
        if (!target) target = hwnd;
        if (IsWindowVisible(hwnd)) {
            target = hwnd;
            break;
        }
    }
    if (!target) return;
    focusWindow(target);
}

void Dock::openActivities() {
    if (!config_.activitiesCommand.empty()) {
        std::wstring cmd = config_.activitiesCommand;
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        std::vector<wchar_t> buf(cmd.begin(), cmd.end());
        buf.push_back(L'\0');
        CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
        if (pi.hProcess) CloseHandle(pi.hProcess);
        if (pi.hThread) CloseHandle(pi.hThread);
        return;
    }

    INPUT inputs[4]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_TAB;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_TAB;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}

void Dock::drawBackground(float alpha) {
    const GfxSize sz = window_.clientSize();
    const GfxRect bg{0.f, 0.f, sz.width, sz.height};
    window_.fillRoundedRect(bg, static_cast<float>(config_.cornerRadius), dockBgColor(config_, alpha));
}

void Dock::drawActivitiesIcon(float cx, float cy, float scale, GfxColor color) {
    const float dot = 3.f * scale;
    const float gap = 5.f * scale;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            const float ox = (static_cast<float>(col) - 1.f) * (dot * 2.f + gap);
            const float oy = (static_cast<float>(row) - 1.f) * (dot * 2.f + gap);
            const GfxRect r{cx + ox - dot, cy + oy - dot, dot * 2.f, dot * 2.f};
            window_.fillEllipse(r, color);
        }
    }
}

void Dock::drawIconShadow(float cx, float iconBottom, float iconSize, float alpha) {
    const float shadowW = iconSize * 0.72f;
    const float shadowH = iconSize * 0.16f;
    GfxColor shadow{0, 0, 0, static_cast<uint8_t>(std::clamp(110.f * alpha, 0.f, 255.f))};
    const GfxRect r{cx - shadowW * 0.5f, iconBottom - shadowH * 0.55f, shadowW, shadowH};
    window_.fillEllipse(r, shadow);
}

void Dock::drawIndicator(float cx, float bottom, bool running, bool focused, float alpha) {
    if (!running) return;
    const float w = focused ? 18.f : 5.f;
    const float h = focused ? 3.f : 5.f;
    GfxColor color = focused ? GfxColor{255, 255, 255, 255} : GfxColor{255, 255, 255, 170};
    color.a = static_cast<uint8_t>(color.a * alpha);
    const GfxRect pill{cx - w * 0.5f, bottom - h - 3.f, w, h};
    window_.fillRoundedRect(pill, h * 0.5f, color);
}

void Dock::drawSeparator(const GfxRect& bounds, float alpha) {
    const float lineH = bounds.height * 0.52f;
    const float lineY = bounds.top + (bounds.height - lineH) * 0.5f;
    const float lineX = bounds.left + bounds.width * 0.5f - 0.5f;
    GfxColor color{255, 255, 255, static_cast<uint8_t>(std::clamp(72.f * alpha, 0.f, 255.f))};
    window_.fillRoundedRect({lineX, lineY, 1.f, lineH}, 0.5f, color);
}

bool Dock::anyAnimating() const {
    return !visibility_.settled() || hoveredIndex_ >= 0;
}

bool Dock::pollEvents() {
    const bool activity = window_.pollEvents();

    if (window_.mouseClicked()) {
        const GfxVec2i pos = window_.mousePos();
        const int idx = itemAt(pos);
        if (idx >= 0) {
            const DockItem& item = items_[static_cast<size_t>(idx)];
            if (item.kind == ItemKind::Activities) {
                openActivities();
            } else if (item.running) {
                focusApp(item);
            } else {
                const std::wstring& launch =
                    !item.launchPath.empty() ? item.launchPath : item.path;
                launchApp(launch);
            }
        }
        window_.clearMouseClicked();
        markDirty();
    }

    return activity;
}

bool Dock::render() {
    if (!window_.isOpen()) return false;

    ID2D1RenderTarget* rt = window_.renderTarget();
    if (!rt) return false;

    const float vis = visibility_.value();
    if (vis <= 0.01f) {
        window_.beginDraw({0, 0, 0, 0});
        window_.endDraw();
        return true;
    }

    window_.beginDraw({0, 0, 0, 0});
    drawBackground(vis);

    const float iconSize = static_cast<float>(config_.iconSize);
    const float h = static_cast<float>(config_.height);
    const float cy = h * 0.5f;

    for (size_t i = 0; i < items_.size(); ++i) {
        const DockItem& item = items_[i];

        if (item.kind == ItemKind::Separator) {
            drawSeparator(item.bounds, vis);
            continue;
        }

        const float iconX = item.bounds.left + item.bounds.width * 0.5f;
        const bool hovered = static_cast<int>(i) == hoveredIndex_;
        const float hoverScale = hovered ? 1.08f : 1.f;
        const float drawSize = iconSize * hoverScale;
        const float drawX = iconX - drawSize * 0.5f;
        const float drawY = cy - drawSize * 0.5f;
        const float iconBottom = drawY + drawSize;

        drawIconShadow(iconX, iconBottom + 2.f, drawSize, vis);

        if (item.kind == ItemKind::Activities) {
            drawActivitiesIcon(iconX, cy, hoverScale, GfxColor{255, 255, 255, 255});
        } else if (ID2D1Bitmap* bmp = icons_.get(rt, item.path, config_.iconSize)) {
            window_.drawBitmap(bmp, {drawX, drawY, drawSize, drawSize}, vis);
        }

        drawIndicator(iconX, h, item.running, item.focused, vis);
    }

    window_.endDraw();
    return true;
}

bool Dock::processFrame() {
    checkConfigChanged();

    if (!config_.enabled) return false;
    if (!open_ || !window_.isOpen()) {
        open_ = false;
        return false;
    }

    const bool hadInput = pollEvents();
    updateHover();
    updateAutohide();

    if (dataTimer_.elapsedMilliseconds() >= config_.performance.dataTickMs) {
        tracker_.refresh();
        rebuildItems();
        dataTimer_.restart();
        markDirty();
    }

    deltaTime_ = static_cast<float>(animTimer_.elapsedSeconds());
    animTimer_.restart();

    if (!visibility_.settled()) {
        visibility_.update(deltaTime_, 280.f);
        applyPosition();
        markDirty();
    }

    const bool animating = anyAnimating();
    if (!dirty_ && !animating && !hadInput) return false;

    const int animFps = std::max(60, std::max(1, config_.performance.animFps));
    if (animating && frameTimer_.elapsedSeconds() < 1.f / static_cast<float>(animFps)) return true;

    frameTimer_.restart();
    render();
    dirty_ = animating || !visibility_.settled();
    return dirty_ || hadInput;
}

}  // namespace wingnome
