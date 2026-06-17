#include "platform.h"
#include "bar.h"

#include "paths.h"

#include <shellapi.h>

#include <windows.h>

#include <algorithm>
#include <filesystem>

namespace wingnome {

namespace {

std::filesystem::path resolveFontFile(const std::wstring& family) {
    static const std::pair<const wchar_t*, const wchar_t*> kMap[] = {
        {L"Segoe UI", L"segoeui.ttf"},
        {L"Segoe UI Semibold", L"seguisb.ttf"},
        {L"Consolas", L"consola.ttf"},
        {L"Arial", L"arial.ttf"},
    };
    for (const auto& [name, file] : kMap) {
        if (family == name) return std::filesystem::path(L"C:\\Windows\\Fonts") / file;
    }
    return std::filesystem::path(L"C:\\Windows\\Fonts\\segoeui.ttf");
}

void sendWinShiftS() {
    INPUT inputs[6]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_SHIFT;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'S';
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = 'S';
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[4].type = INPUT_KEYBOARD;
    inputs[4].ki.wVk = VK_SHIFT;
    inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[5].type = INPUT_KEYBOARD;
    inputs[5].ki.wVk = VK_LWIN;
    inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(6, inputs, sizeof(INPUT));
}

}  // namespace

void Bar::markDirty() {
    dirty_ = true;
}

bool Bar::loadFont() {
    const auto path = resolveFontFile(config_.font);
    return font_.loadFromFile(path.string());
}

void Bar::setupPopupActions() {
    popup_.setOnScreenshot([] { sendWinShiftS(); });
    popup_.setOnSettings([] {
        ShellExecuteW(nullptr, L"open", L"ms-settings:", nullptr, nullptr, SW_SHOW);
    });
    popup_.setOnPower([] {
        ShellExecuteW(nullptr, L"open", L"shutdown.exe", L"/l", nullptr, SW_HIDE);
    });
}

bool Bar::create() {
    configPath_ = findConfigFile(L"bar.json");
    reloadConfig();
    if (!loadFont()) return false;

    const int screenW = static_cast<int>(sf::VideoMode::getDesktopMode().width);
    window_.create(sf::VideoMode(screenW, config_.height), "WinGnome Bar", sf::Style::None);
    if (!window_.isOpen()) return false;

    window_.setPosition(sf::Vector2i(0, 0));
    window_.setVerticalSyncEnabled(config_.performance.vsync);
    window_.setActive(false);

    applyTopmost();

    if (!popup_.create(config_.height, screenW, config_)) return false;
    setupPopupActions();

    ctx_.font = &font_;
    ctx_.fontSize = static_cast<unsigned>(config_.fontSize);
    ctx_.config = &config_;
    ctx_.barHeight = config_.height;
    ctx_.popup = &popup_;

    rebuildModules();
    tickModules();
    frameClock_.restart();
    animClock_.restart();
    dataClock_.restart();
    dirty_ = true;
    layoutDirty_ = true;
    open_ = true;
    return true;
}

void Bar::destroy() {
    popup_.destroy();
    if (window_.isOpen()) window_.close();
    open_ = false;
    captureModule_ = nullptr;
    ctx_.popup = nullptr;
}

HWND Bar::hwnd() const {
    return window_.isOpen() ? reinterpret_cast<HWND>(window_.getSystemHandle()) : nullptr;
}

void Bar::applyTopmost() {
    HWND native = reinterpret_cast<HWND>(window_.getSystemHandle());
    if (!native) return;
    const int screenW = GetSystemMetrics(SM_CXSCREEN);
    LONG_PTR ex = GetWindowLongPtr(native, GWL_EXSTYLE);
    SetWindowLongPtr(native, GWL_EXSTYLE, ex | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
    SetWindowPos(native, HWND_TOPMOST, 0, 0, screenW, config_.height, SWP_SHOWWINDOW);
}

void Bar::reloadConfig() {
    config_ = BarConfig::load(configPath_);
    ctx_.config = &config_;
    ctx_.barHeight = config_.height;
    ctx_.fontSize = static_cast<unsigned>(config_.fontSize);
    if (window_.isOpen()) window_.setVerticalSyncEnabled(config_.performance.vsync);
    layoutDirty_ = true;
    markDirty();
}

void Bar::rebuildModules() {
    left_.clear();
    center_.clear();
    right_.clear();
    layout_.clear();
    auto add = [&](const std::vector<std::wstring>& names, auto& store) {
        for (const auto& n : names) {
            store.push_back(createModule(n, &ctx_));
        }
    };
    add(config_.modulesLeft, left_);
    add(config_.modulesCenter, center_);
    add(config_.modulesRight, right_);
    layoutDirty_ = true;
    markDirty();
}

void Bar::tickModules() {
    for (const auto& m : left_) m->tick();
    for (const auto& m : center_) m->tick();
    for (const auto& m : right_) m->tick();
    layoutDirty_ = true;
    markDirty();
}

void Bar::buildLayout() {
    layout_.clear();
    const float totalW = static_cast<float>(window_.getSize().x);
    const float h = static_cast<float>(window_.getSize().y);

    auto sectionWidth = [&](const auto& mods) {
        float w = 0.f;
        for (const auto& m : mods) w += m->measure().x;
        return w;
    };

    float xLeft = 0.f;
    float xCenter = (totalW - sectionWidth(center_)) * 0.5f;
    float xRight = totalW - sectionWidth(right_);

    auto place = [&](float& x, const auto& mods) {
        for (const auto& m : mods) {
            const sf::Vector2f sz = m->measure();
            layout_.push_back({m.get(), {x, 0.f, sz.x, h}});
            x += sz.x;
        }
    };

    place(xLeft, left_);
    place(xCenter, center_);
    place(xRight, right_);
    layoutDirty_ = false;
}

Bar::PlacedModule* Bar::moduleAt(const sf::Vector2i& pos) {
    if (layout_.empty()) buildLayout();
    for (auto& placed : layout_) {
        if (placed.bounds.contains(static_cast<float>(pos.x), static_cast<float>(pos.y)))
            return &placed;
    }
    return nullptr;
}

bool Bar::anyAnimating() const {
    if (popup_.isAnimating()) return true;
    auto check = [](const auto& mods) {
        for (const auto& m : mods) {
            if (m->isAnimating()) return true;
        }
        return false;
    };
    return check(left_) || check(center_) || check(right_);
}

void Bar::updateModules() {
    if (layout_.empty() || layoutDirty_) buildLayout();

    bool measureChanged = false;
    for (auto& placed : layout_) {
        ModuleInput input;
        input.mousePos = mousePos_;
        input.hovered = placed.bounds.contains(static_cast<float>(mousePos_.x),
                                               static_cast<float>(mousePos_.y));
        input.pressed = mouseDown_ && input.hovered;
        input.dragging = mouseDown_;
        placed.module->update(input, placed.bounds);
        if (placed.module->layoutAffectsMeasure()) measureChanged = true;
    }

    if (measureChanged) layoutDirty_ = true;
}

bool Bar::render() {
    if (layoutDirty_) buildLayout();

    window_.clear(toSfColor(config_.background));
    const sf::Color fg = toSfColor(config_.foreground);

    for (const auto& placed : layout_) {
        ModulePaintInfo info{window_, ctx_, placed.bounds, fg};
        placed.module->paint(info);
    }
    window_.display();

    if (popup_.isVisible()) popup_.render();
    return true;
}

bool Bar::pollEvents() {
    bool activity = false;

    if (popup_.isVisible()) activity |= popup_.pollEvents();

    sf::Event event{};
    while (window_.pollEvent(event)) {
        activity = true;
        markDirty();

        if (event.type == sf::Event::Closed) {
            open_ = false;
            PostQuitMessage(0);
            continue;
        }
        if (event.type == sf::Event::Resized) layoutDirty_ = true;

        if (event.type == sf::Event::MouseMoved) {
            mousePos_ = {event.mouseMove.x, event.mouseMove.y};
        }
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
            mouseDown_ = true;
            mousePos_ = {event.mouseButton.x, event.mouseButton.y};
            if (auto* placed = moduleAt(mousePos_)) {
                if (placed->module->interactive()) {
                    captureModule_ = placed->module;
                    placed->module->onClick(mousePos_, placed->bounds);
                }
            }
        }
        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Left) {
            mouseDown_ = false;
            captureModule_ = nullptr;
        }
        if (event.type == sf::Event::MouseWheelScrolled) {
            mousePos_ = sf::Mouse::getPosition(window_);
            if (auto* placed = moduleAt(mousePos_)) {
                if (placed->module->interactive())
                    placed->module->onScroll(mousePos_, event.mouseWheelScroll.delta, placed->bounds);
            }
        }
    }

    if (mouseDown_ && captureModule_) {
        activity = true;
        markDirty();
        for (auto& placed : layout_) {
            if (placed.module == captureModule_) {
                ModuleInput input{mousePos_, true, true, true};
                placed.module->update(input, placed.bounds);
                break;
            }
        }
    }

    return activity;
}

bool Bar::processFrame() {
    if (!open_ || !window_.isOpen()) {
        open_ = false;
        return false;
    }

    const bool hadInput = pollEvents();

    if (dataClock_.getElapsedTime().asMilliseconds() >= config_.performance.dataTickMs) {
        tickModules();
        dataClock_.restart();
    }

    const bool popupActive = popup_.isVisible() || popup_.isAnimating();
    if (popupActive) popup_.processFrame(ctx_.deltaTime);

    const bool animating = anyAnimating();
    if (animating || hadInput || mouseDown_ || popupActive) {
        ctx_.deltaTime = animClock_.restart().asSeconds();
        updateModules();
    }

    if (!dirty_ && !animating && !hadInput && !mouseDown_ && !popupActive) return false;

    const int animFps = std::max(1, config_.performance.animFps);
    const float minFrame = 1.f / static_cast<float>(animFps);
    if ((animating || popupActive) && frameClock_.getElapsedTime().asSeconds() < minFrame) return true;

    frameClock_.restart();
    render();
    dirty_ = animating || mouseDown_ || popupActive;
    return dirty_ || hadInput;
}

}  // namespace wingnome
