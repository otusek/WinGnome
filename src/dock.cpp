#include "platform.h"
#include "dock.h"

#include "paths.h"

#include <shellapi.h>

#include <algorithm>
#include <cmath>

namespace wingnome {

namespace {

void focusWindow(HWND hwnd) {
    if (!hwnd) return;
    HWND fg = GetForegroundWindow();
    DWORD fgThread = GetWindowThreadProcessId(fg, nullptr);
    DWORD curThread = GetCurrentThreadId();
    if (fgThread != curThread) AttachThreadInput(curThread, fgThread, TRUE);
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    if (fgThread != curThread) AttachThreadInput(curThread, fgThread, FALSE);
}

}  // namespace

bool Dock::create() {
    configPath_ = findConfigFile(L"dock.json");
    reloadConfig();
    if (!config_.enabled) return true;

    tracker_.refresh();
    rebuildItems();
    const int width = static_cast<int>(contentWidth());

    window_.create(sf::VideoMode(std::max(width, 64), config_.height), "WinGnome Dock", sf::Style::None);
    if (!window_.isOpen()) return false;

    window_.setVerticalSyncEnabled(config_.performance.vsync);
    window_.setActive(false);
    applyTopmost();
    applyPosition();

    visibility_.snap(config_.autohide ? 0.f : 1.f);

    frameClock_.restart();
    animClock_.restart();
    dataClock_.restart();
    dirty_ = true;
    open_ = true;
    return true;
}

void Dock::destroy() {
    if (window_.isOpen()) window_.close();
    icons_.clear();
    open_ = false;
}

HWND Dock::hwnd() const {
    return window_.isOpen() ? reinterpret_cast<HWND>(window_.getSystemHandle()) : nullptr;
}

void Dock::setExcludeHwnds(const std::vector<HWND>& hwnds) {
    std::vector<HWND> all = hwnds;
    if (HWND self = hwnd()) all.push_back(self);
    tracker_.setExcludeHwnds(all);
}

void Dock::reloadConfig() {
    config_ = DockConfig::load(configPath_);
    dirty_ = true;
}

float Dock::contentWidth() const {
    int count = 0;
    if (config_.showActivities) ++count;
    int apps = static_cast<int>(items_.size());
    if (apps == 0) apps = static_cast<int>(config_.pinned.size());
    count += apps;
    if (count == 0) count = config_.showActivities ? 1 : 0;
    const float icons = static_cast<float>(count);
    return config_.paddingH * 2.f +
           icons * static_cast<float>(config_.iconSize) +
           std::max(0.f, icons - 1.f) * static_cast<float>(config_.iconSpacing);
}

void Dock::rebuildItems() {
    items_.clear();

    std::vector<std::wstring> pinnedResolved;
    pinnedResolved.reserve(config_.pinned.size());
    for (const auto& p : config_.pinned) {
        const std::wstring resolved = resolveAppPath(p);
        if (!resolved.empty()) pinnedResolved.push_back(resolved);
    }

    auto addItem = [&](const std::wstring& path, const RunningApp* running) {
        for (auto& item : items_) {
            if (_wcsicmp(item.path.c_str(), path.c_str()) == 0) {
                if (running) {
                    item.running = true;
                    item.focused = running->focused;
                    item.windows = running->windows;
                }
                return;
            }
        }
        DockItem item;
        item.kind = ItemKind::App;
        item.path = path;
        if (running) {
            item.running = true;
            item.focused = running->focused;
            item.windows = running->windows;
        }
        items_.push_back(std::move(item));
    };

    for (const auto& path : pinnedResolved) addItem(path, nullptr);

    if (config_.showRunningApps) {
        for (const auto& app : tracker_.apps()) addItem(app.exePath, &app);
    }

    const int newW = static_cast<int>(contentWidth());
    if (window_.isOpen() && window_.getSize().x != static_cast<unsigned>(newW)) {
        window_.setSize(sf::Vector2u(newW, static_cast<unsigned>(config_.height)));
        applyTopmost();
    }
    dirty_ = true;
}

void Dock::applyTopmost() {
    HWND native = hwnd();
    if (!native) return;
    LONG_PTR ex = GetWindowLongPtr(native, GWL_EXSTYLE);
    SetWindowLongPtr(native, GWL_EXSTYLE, ex | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED);
    SetLayeredWindowAttributes(native, RGB(1, 0, 1), 0, LWA_COLORKEY);
    SetWindowPos(native, HWND_TOPMOST, window_.getPosition().x, window_.getPosition().y,
                 static_cast<int>(window_.getSize().x), config_.height, SWP_SHOWWINDOW);
}

void Dock::applyPosition() {
    if (!window_.isOpen()) return;
    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    const int screenH = GetSystemMetrics(SM_CYSCREEN);
    const int width = static_cast<int>(window_.getSize().x);
    const int x = (screenW - width) / 2;
    const float vis = config_.autohide ? visibility_.value() : 1.f;
    const int hiddenOffset = config_.height + config_.marginBottom;
    const int y = screenH - config_.marginBottom - config_.height +
                  static_cast<int>((1.f - vis) * static_cast<float>(hiddenOffset));
    window_.setPosition(sf::Vector2i(x, y));
    HWND native = hwnd();
    if (native) {
        SetWindowPos(native, HWND_TOPMOST, x, y, width, config_.height, SWP_SHOWWINDOW);
    }
}

sf::FloatRect Dock::dockScreenRect() const {
    const sf::Vector2i pos = window_.getPosition();
    return {static_cast<float>(pos.x), static_cast<float>(pos.y),
            static_cast<float>(window_.getSize().x), static_cast<float>(config_.height)};
}

void Dock::updateAutohide() {
    if (!config_.autohide) {
        visibility_.setTarget(1.f);
        visibility_.update(deltaTime_, 180.f);
        return;
    }

    POINT cursor{};
    GetCursorPos(&cursor);
    const int screenH = GetSystemMetrics(SM_CYSCREEN);
    const sf::FloatRect dock = dockScreenRect();
    const bool overDock = dock.contains(static_cast<float>(cursor.x), static_cast<float>(cursor.y));
    const bool nearBottom = cursor.y >= screenH - config_.autohideTriggerPx;
    visibility_.setTarget(overDock || nearBottom ? 1.f : 0.f);
    visibility_.update(deltaTime_, 220.f);
    applyPosition();
}

void Dock::updateHover() {
    POINT cursor{};
    GetCursorPos(&cursor);
    const sf::Vector2i winPos = window_.getPosition();
    const sf::Vector2i local{cursor.x - winPos.x, cursor.y - winPos.y};
    hoveredIndex_ = itemAt(local);

    const float hoverSpeed = 12.f;
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        const float target = (i == hoveredIndex_) ? 1.f : 0.f;
        items_[static_cast<size_t>(i)].hover = approach(items_[static_cast<size_t>(i)].hover, target,
                                                         hoverSpeed * deltaTime_);
        if (std::abs(items_[static_cast<size_t>(i)].hover - target) > 0.001f) dirty_ = true;
    }
    if (config_.showActivities) {
        const int actIndex = 0;
        const bool hoverAct = hoveredIndex_ == actIndex;
        (void)hoverAct;
    }
}

int Dock::itemAt(const sf::Vector2i& pos) const {
    float x = static_cast<float>(config_.paddingH);
    const float cy = static_cast<float>(config_.height) * 0.5f;
    const float hit = static_cast<float>(config_.iconSize) * 0.55f;

    if (config_.showActivities) {
        const float cx = x + static_cast<float>(config_.iconSize) * 0.5f;
        if (std::abs(pos.x - cx) <= hit && std::abs(pos.y - cy) <= hit) return 0;
        x += static_cast<float>(config_.iconSize + config_.iconSpacing);
    }

    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        const int visualIndex = i + (config_.showActivities ? 1 : 0);
        const float cx = x + static_cast<float>(config_.iconSize) * 0.5f;
        if (std::abs(pos.x - cx) <= hit && std::abs(pos.y - cy) <= hit) return visualIndex;
        x += static_cast<float>(config_.iconSize + config_.iconSpacing);
    }
    return -1;
}

void Dock::launchApp(const std::wstring& path) {
    const std::wstring resolved = resolveAppPath(path);
    if (resolved.empty()) return;
    ShellExecuteW(nullptr, L"open", resolved.c_str(), nullptr, nullptr, SW_SHOW);
}

void Dock::focusApp(const DockItem& item) {
    HWND target = nullptr;
    for (HWND hwnd : item.windows) {
        if (IsWindow(hwnd) && IsWindowVisible(hwnd)) {
            target = hwnd;
            if (hwnd == GetForegroundWindow()) return;
        }
    }
    if (target) {
        focusWindow(target);
        return;
    }
    launchApp(item.path);
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
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(VK_TAB, 0, 0, 0);
    keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
}

void Dock::drawRoundedBackground(sf::RenderTarget& target, const sf::FloatRect& rect) const {
    const float r = static_cast<float>(config_.cornerRadius);
    const sf::Color bg = toSfColor(config_.background,
                                   static_cast<sf::Uint8>(config_.backgroundAlpha));

    auto drawBar = [&](float x, float y, float w, float h) {
        sf::RectangleShape bar({w, h});
        bar.setPosition(x, y);
        bar.setFillColor(bg);
        target.draw(bar);
    };

    drawBar(rect.left + r, rect.top, rect.width - 2.f * r, rect.height);
    drawBar(rect.left, rect.top + r, r, rect.height - 2.f * r);
    drawBar(rect.left + rect.width - r, rect.top + r, r, rect.height - 2.f * r);

    const sf::CircleShape corner(r);
    const_cast<sf::CircleShape&>(corner).setFillColor(bg);
    auto drawCorner = [&](float x, float y) {
        sf::CircleShape c(r);
        c.setFillColor(bg);
        c.setPosition(x, y);
        target.draw(c);
    };
    drawCorner(rect.left, rect.top);
    drawCorner(rect.left + rect.width - 2.f * r, rect.top);
    drawCorner(rect.left, rect.top + rect.height - 2.f * r);
    drawCorner(rect.left + rect.width - 2.f * r, rect.top + rect.height - 2.f * r);
}

void Dock::drawActivitiesIcon(sf::RenderTarget& target, float cx, float cy, float scale,
                              const sf::Color& color) const {
    const float dot = 3.f * scale;
    const float gap = 5.f * scale;
    const sf::Color fg = color;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            sf::CircleShape dotShape(dot);
            dotShape.setFillColor(fg);
            dotShape.setOrigin(dot, dot);
            const float ox = (static_cast<float>(col) - 1.f) * (dot * 2.f + gap);
            const float oy = (static_cast<float>(row) - 1.f) * (dot * 2.f + gap);
            dotShape.setPosition(cx + ox, cy + oy);
            target.draw(dotShape);
        }
    }
}

void Dock::drawIndicator(sf::RenderTarget& target, float cx, float bottom, bool running,
                         bool focused) const {
    if (!running) return;
    const float w = focused ? 18.f : 5.f;
    const float h = focused ? 3.f : 5.f;
    sf::RectangleShape pill({w, h});
    pill.setFillColor(focused ? sf::Color::White : sf::Color(255, 255, 255, 170));
    pill.setOrigin(w * 0.5f, h);
    pill.setPosition(cx, bottom - 4.f);
    target.draw(pill);
}

bool Dock::pollEvents() {
    bool activity = false;
    sf::Event event{};
    while (window_.pollEvent(event)) {
        activity = true;
        dirty_ = true;
        if (event.type == sf::Event::Closed) {
            open_ = false;
            continue;
        }
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
            const int index = itemAt({event.mouseButton.x, event.mouseButton.y});
            if (index < 0) continue;
            if (config_.showActivities && index == 0) {
                openActivities();
                continue;
            }
            const int itemIndex = index - (config_.showActivities ? 1 : 0);
            if (itemIndex >= 0 && itemIndex < static_cast<int>(items_.size())) {
                focusApp(items_[static_cast<size_t>(itemIndex)]);
            }
        }
    }
    return activity;
}

bool Dock::render() {
    window_.clear(sf::Color(1, 0, 1));
    const sf::FloatRect bgRect{0.f, 0.f, static_cast<float>(window_.getSize().x),
                               static_cast<float>(config_.height)};
    drawRoundedBackground(window_, bgRect);

    float x = static_cast<float>(config_.paddingH);
    const float cy = static_cast<float>(config_.height) * 0.5f;
    const float indicatorY = static_cast<float>(config_.height) - static_cast<float>(config_.paddingV);

    if (config_.showActivities) {
        const float cx = x + static_cast<float>(config_.iconSize) * 0.5f;
        const float scale = hoveredIndex_ == 0 ? 1.12f : 1.f;
        drawActivitiesIcon(window_, cx, cy, scale, sf::Color::White);
        x += static_cast<float>(config_.iconSize + config_.iconSpacing);
    }

    for (const auto& item : items_) {
        const float hoverScale = 1.f + item.hover * 0.15f;
        const float drawSize = static_cast<float>(config_.iconSize) * hoverScale;
        const float cx = x + static_cast<float>(config_.iconSize) * 0.5f;

        if (sf::Texture* tex = icons_.get(item.path, config_.iconSize)) {
            sf::Sprite sprite(*tex);
            const sf::FloatRect bounds = sprite.getLocalBounds();
            const float scale = drawSize / std::max(bounds.width, bounds.height);
            sprite.setScale(scale, scale);
            sprite.setOrigin(bounds.width * 0.5f, bounds.height * 0.5f);
            sprite.setPosition(cx, cy);
            window_.draw(sprite);
        } else {
            sf::CircleShape fallback(drawSize * 0.35f);
            fallback.setFillColor(sf::Color(90, 85, 105));
            fallback.setOrigin(fallback.getRadius(), fallback.getRadius());
            fallback.setPosition(cx, cy);
            window_.draw(fallback);
        }

        drawIndicator(window_, cx, indicatorY, item.running, item.focused);
        x += static_cast<float>(config_.iconSize + config_.iconSpacing);
    }

    window_.display();
    return true;
}

bool Dock::processFrame() {
    if (!config_.enabled) return false;
    if (!open_ || !window_.isOpen()) {
        open_ = false;
        return false;
    }

    deltaTime_ = animClock_.restart().asSeconds();
    const bool hadInput = pollEvents();

    if (dataClock_.getElapsedTime().asMilliseconds() >= config_.performance.dataTickMs) {
        tracker_.refresh();
        rebuildItems();
        dataClock_.restart();
    }

    updateAutohide();
    updateHover();

    const bool animating = config_.autohide && !visibility_.settled();
    if (!dirty_ && !animating && !hadInput) return false;

    const int animFps = std::max(1, config_.performance.animFps);
    const float minFrame = 1.f / static_cast<float>(animFps);
    if (animating && frameClock_.getElapsedTime().asSeconds() < minFrame) return true;

    frameClock_.restart();
    render();
    dirty_ = animating || hadInput;
    return dirty_ || hadInput || animating;
}

}  // namespace wingnome
