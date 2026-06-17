#include "platform.h"
#include "bar.h"

#include "battery_status.h"
#include "json.h"
#include "network_status.h"
#include "paths.h"
#include "shell_actions.h"

namespace wingnome {

namespace {

int primaryScreenWidth() { return GetSystemMetrics(SM_CXSCREEN); }

constexpr float kTextPadH = 12.f;
constexpr float kIconPadH = 8.f;
constexpr int kIconSize = 16;

GfxSize measureText(const ModuleContext& ctx, const std::wstring& text, float padH = kTextPadH) {
    if (!ctx.window || !ctx.font || text.empty()) return {padH * 2.f, static_cast<float>(ctx.fontSize)};
    const GfxSize sz = ctx.window->measureText(*ctx.font, static_cast<float>(ctx.fontSize), text);
    return {sz.width + padH * 2.f, static_cast<float>(ctx.barHeight)};
}

void drawText(const ModulePaintInfo& info, const std::wstring& text, float padH = kTextPadH) {
    if (!info.ctx.font || text.empty()) return;
    const float size = static_cast<float>(info.ctx.fontSize);
    const GfxSize sz = info.window.measureText(*info.ctx.font, size, text);
    const float x = info.bounds.left + padH;
    const float y = info.bounds.top + (info.bounds.height - sz.height) * 0.5f;
    GfxColor fg = info.fg;
    fg.a = static_cast<uint8_t>(fg.a * info.reveal);
    info.window.drawText(*info.ctx.font, size, text, x, y, fg);
}

GfxSize measureIcon(const ModuleContext& ctx, float extraTextW = 0.f) {
    return {kIconPadH * 2.f + kIconSize + extraTextW, static_cast<float>(ctx.barHeight)};
}

void drawIcon(const ModulePaintInfo& info, const std::wstring& asset, float extraTextW = 0.f,
              const std::wstring& text = {}) {
    ID2D1RenderTarget* rt = info.window.renderTarget();
    if (!rt || !info.ctx.icons) return;

    GfxColor tint = info.fg;
    tint.a = static_cast<uint8_t>(tint.a * info.reveal);
    if (ID2D1Bitmap* bmp = info.ctx.icons->get(rt, asset, kIconSize, tint)) {
        const float iy = info.bounds.top + (info.bounds.height - kIconSize) * 0.5f;
        info.window.drawBitmap(bmp, {info.bounds.left + kIconPadH, iy,
                                     static_cast<float>(kIconSize), static_cast<float>(kIconSize)},
                               info.reveal);
    }

    if (!text.empty() && info.ctx.font) {
        const float size = static_cast<float>(info.ctx.fontSize);
        const float x = info.bounds.left + kIconPadH + kIconSize + 6.f;
        const float y = info.bounds.top + (info.bounds.height - size) * 0.5f;
        info.window.drawText(*info.ctx.font, size, text, x, y, tint);
    }
    (void)extraTextW;
}

class SpacerModule : public IModule {
public:
    explicit SpacerModule(std::wstring id) : id_(std::move(id)) {}
    std::wstring id() const override { return id_; }
    GfxSize measure() const override { return {1.f, 1.f}; }
    void paint(const ModulePaintInfo&) const override {}

private:
    std::wstring id_;
};

class TextModule : public IModule {
public:
    TextModule(std::wstring id, std::wstring text) : id_(std::move(id)), text_(std::move(text)) {}
    std::wstring id() const override { return id_; }
    std::wstring label() const override { return text_; }
    GfxSize measure() const override { return measureText(*ctx_, text_); }
    void paint(const ModulePaintInfo& info) const override { drawText(info, text_); }
    void setContext(ModuleContext* ctx) { ctx_ = ctx; }

private:
    std::wstring id_, text_;
    ModuleContext* ctx_{nullptr};
};

class ClockModule : public IModule {
public:
    explicit ClockModule(ModuleContext* ctx) : ctx_(ctx) { update(); }
    std::wstring id() const override { return L"clock"; }
    void tick() override { update(); }
    GfxSize measure() const override { return measureText(*ctx_, text_); }
    void paint(const ModulePaintInfo& info) const override { drawText(info, text_); }
    bool layoutAffectsMeasure() const override { return true; }

private:
    ModuleContext* ctx_;
    mutable std::wstring text_{L"--:--"};

    void update() const {
        std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        wchar_t buf[128]{};
        std::wcsftime(buf, 128, ctx_->config->clockFormat.c_str(), &local);
        text_ = buf;
    }
};

class ActivitiesModule : public IModule {
public:
    explicit ActivitiesModule(ModuleContext* ctx) : ctx_(ctx) {}
    std::wstring id() const override { return L"activities"; }
    bool interactive() const override { return true; }
    GfxSize measure() const override { return measureIcon(*ctx_); }
    void paint(const ModulePaintInfo& info) const override {
        drawIcon(info, L"icons/activities.svg");
    }
    void onClick(const GfxVec2i&, const GfxRect&) override {
        if (ctx_->host) ctx_->host->openActivities();
    }

private:
    ModuleContext* ctx_;
};

class BatteryModule : public IModule {
public:
    explicit BatteryModule(ModuleContext* ctx) : ctx_(ctx) { tick(); }
    std::wstring id() const override { return L"battery"; }
    void tick() override {
        BatteryStatus status{};
        if (queryBatteryStatus(status)) status_ = status;
    }
    GfxSize measure() const override {
        if (!status_.present || status_.percent < 0) return measureIcon(*ctx_);
        text_ = std::to_wstring(status_.percent) + L"%";
        const GfxSize textSz =
            ctx_->window->measureText(*ctx_->font, static_cast<float>(ctx_->fontSize), text_);
        return measureIcon(*ctx_, textSz.width + 6.f);
    }
    void paint(const ModulePaintInfo& info) const override {
        const std::wstring icon =
            status_.charging ? L"icons/battery-charging.svg" : L"icons/battery.svg";
        drawIcon(info, icon, 0.f, status_.present && status_.percent >= 0 ? text_ : L"");
    }
    bool layoutAffectsMeasure() const override { return true; }

private:
    ModuleContext* ctx_;
    BatteryStatus status_{};
    mutable std::wstring text_;
};

class NetworkModule : public IModule {
public:
    explicit NetworkModule(ModuleContext* ctx) : ctx_(ctx) { tick(); }
    std::wstring id() const override { return L"network"; }
    void tick() override {
        NetworkStatus status{};
        if (queryNetworkStatus(status)) status_ = status;
    }
    GfxSize measure() const override { return measureIcon(*ctx_); }
    void paint(const ModulePaintInfo& info) const override {
        drawIcon(info, status_.connected ? L"icons/network.svg" : L"icons/network-offline.svg");
    }

private:
    ModuleContext* ctx_;
    NetworkStatus status_{};
};

class VolumeModule : public IModule {
public:
    explicit VolumeModule(ModuleContext* ctx) : ctx_(ctx) {}
    std::wstring id() const override { return L"volume"; }
    bool interactive() const override { return true; }
    void tick() override {
        if (!ctx_->audio) return;
        muted_ = ctx_->audio->muted();
        level_ = ctx_->audio->level();
    }
    GfxSize measure() const override { return measureIcon(*ctx_); }
    void paint(const ModulePaintInfo& info) const override {
        drawIcon(info, muted_ || level_ <= 0.01f ? L"icons/volume-muted.svg" : L"icons/volume.svg");
    }
    void onClick(const GfxVec2i&, const GfxRect&) override {
        if (ctx_->audio) ctx_->audio->toggleMute();
    }
    void onScroll(const GfxVec2i&, float delta, const GfxRect&) override {
        if (!ctx_->audio) return;
        ctx_->audio->adjustLevel(delta > 0.f ? 0.05f : -0.05f);
    }

private:
    ModuleContext* ctx_;
    bool muted_{false};
    float level_{0.f};
};

class SystemModule : public IModule {
public:
    explicit SystemModule(ModuleContext* ctx) : ctx_(ctx) {}
    std::wstring id() const override { return L"system"; }
    bool interactive() const override { return true; }
    GfxSize measure() const override { return measureIcon(*ctx_); }
    void paint(const ModulePaintInfo& info) const override { drawIcon(info, L"icons/power.svg"); }
    void onClick(const GfxVec2i&, const GfxRect& bounds) override {
        if (!ctx_->host) return;
        if (ctx_->host->systemMenuVisible()) {
            ctx_->host->hideSystemMenu();
        } else {
            ctx_->host->openSystemMenu(bounds);
        }
    }

private:
    ModuleContext* ctx_;
};

class StubModule : public IModule {
public:
    explicit StubModule(std::wstring id) : id_(std::move(id)) {}
    std::wstring id() const override { return id_; }
    GfxSize measure() const override { return {0.f, 0.f}; }
    void paint(const ModulePaintInfo&) const override {}

private:
    std::wstring id_;
};

}  // namespace

GfxColor toGfxColor(COLORREF color, uint8_t alpha) {
    return gfxFromColorref(color, alpha);
}

std::unique_ptr<IModule> createModule(const std::wstring& name, ModuleContext* ctx) {
    if (name == L"clock") return std::make_unique<ClockModule>(ctx);
    if (name == L"spacer") return std::make_unique<SpacerModule>(name);
    if (name == L"activities") return std::make_unique<ActivitiesModule>(ctx);
    if (name == L"battery") return std::make_unique<BatteryModule>(ctx);
    if (name == L"network") return std::make_unique<NetworkModule>(ctx);
    if (name == L"volume") return std::make_unique<VolumeModule>(ctx);
    if (name == L"system") return std::make_unique<SystemModule>(ctx);
    if (name.rfind(L"text/", 0) == 0) {
        auto m = std::make_unique<TextModule>(name, name.substr(5));
        m->setContext(ctx);
        return m;
    }

    if (name == L"tray" || name == L"workspaces") {
        return std::make_unique<StubModule>(name);
    }

    auto m = std::make_unique<TextModule>(name, name);
    m->setContext(ctx);
    return m;
}

void Bar::markDirty() { dirty_ = true; }

bool Bar::loadFont() {
    if (!window_.writeFactory()) return false;
    return font_.create(window_.writeFactory(), config_.font);
}

int Bar::screenWidth() const { return primaryScreenWidth(); }

bool Bar::create() {
    configPath_ = findConfigFile(L"bar.json");
    reloadConfig();

    const int screenW = screenWidth();
    if (!window_.create(L"WinGnome Bar", 0, 0, screenW, config_.height, true)) return false;
    if (!loadFont()) return false;
    if (!popup_.create()) return false;

    applyTopmost();

    ctx_.font = &font_;
    ctx_.window = &window_;
    ctx_.icons = &icons_;
    ctx_.audio = &audio_;
    ctx_.host = this;
    ctx_.fontSize = static_cast<unsigned>(config_.fontSize);
    ctx_.config = &config_;
    ctx_.barHeight = config_.height;

    audio_.init();
    rebuildModules();
    tickModules();

    revealAnim_.snap(0.f);
    revealAnim_.setTarget(1.f);

    frameTimer_.restart();
    animTimer_.restart();
    dataTimer_.restart();
    dirty_ = true;
    layoutDirty_ = true;
    open_ = true;
    return true;
}

void Bar::destroy() {
    hideSystemMenu();
    popup_.destroy();
    audio_.shutdown();
    icons_.clear();
    window_.destroy();
    font_.destroy();
    open_ = false;
}

HWND Bar::hwnd() const { return window_.hwnd(); }

void Bar::applyTopmost() {
    window_.setTopmost(0, 0, screenWidth(), config_.height);
}

void Bar::reloadConfig() {
    configPath_ = findConfigFile(L"bar.json");
    const auto parsed = JsonParser::parseFile(configPath_);
    if (!parsed || parsed->type() != JsonValue::Type::Object) {
        configMarkReloaded(configWatch_, configPath_);
        return;
    }

    config_ = BarConfig::load(configPath_);
    ctx_.config = &config_;
    ctx_.barHeight = config_.height;
    ctx_.fontSize = static_cast<unsigned>(config_.fontSize);

    if (open_) {
        loadFont();
        rebuildModules();
        window_.resize(screenWidth(), config_.height);
        applyTopmost();
    }

    configMarkReloaded(configWatch_, configPath_);
    layoutDirty_ = true;
    markDirty();
}

void Bar::checkConfigChanged() {
    if (!configNeedsReload(configWatch_, L"bar.json")) return;
    reloadConfig();
}

void Bar::rebuildModules() {
    left_.clear();
    center_.clear();
    right_.clear();
    layout_.clear();

    auto add = [&](const std::vector<std::wstring>& names, auto& store) {
        for (const auto& n : names) store.push_back(createModule(n, &ctx_));
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
    markDirty();
}

void Bar::buildLayout() {
    layout_.clear();
    const float totalW = static_cast<float>(screenWidth());
    const float h = static_cast<float>(config_.height);

    auto sectionWidth = [&](const auto& mods) {
        float w = 0.f;
        for (const auto& m : mods) w += m->measure().width;
        return w;
    };

    float xLeft = 0.f;
    float xCenter = (totalW - sectionWidth(center_)) * 0.5f;
    float xRight = totalW - sectionWidth(right_);

    auto place = [&](float& x, const auto& mods) {
        for (const auto& m : mods) {
            const GfxSize sz = m->measure();
            layout_.push_back({m.get(), {x, 0.f, sz.width, h}});
            x += sz.width;
        }
    };

    place(xLeft, left_);
    place(xCenter, center_);
    place(xRight, right_);
    layoutDirty_ = false;
}

GfxVec2i Bar::adjustedMousePos() const {
    const float reveal = revealAnim_.value();
    const float slideY = (1.f - reveal) * -static_cast<float>(config_.height);
    GfxVec2i pos = window_.mousePos();
    pos.y -= static_cast<int>(slideY);
    return pos;
}

int Bar::moduleAt(const GfxVec2i& pos, float slideY) const {
    for (size_t i = 0; i < layout_.size(); ++i) {
        GfxRect bounds = layout_[i].bounds;
        bounds.top += slideY;
        if (bounds.contains(static_cast<float>(pos.x), static_cast<float>(pos.y))) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Bar::anyAnimating() const {
    if (!revealAnim_.settled()) return true;
    auto check = [](const auto& mods) {
        for (const auto& m : mods) {
            if (m->isAnimating()) return true;
        }
        return false;
    };
    return check(left_) || check(center_) || check(right_);
}

void Bar::openSystemMenu(const GfxRect& anchor) {
    popup_.show(anchor, config_.background, config_.foreground, config_.font, config_.fontSize);
    markDirty();
}

void Bar::openActivities() { wingnome::openActivitiesOverview(); }

bool Bar::systemMenuVisible() const { return popup_.isVisible(); }

void Bar::hideSystemMenu() { popup_.hide(); }

void Bar::updateModules() {
    if (layout_.empty() || layoutDirty_) buildLayout();

    const float reveal = revealAnim_.value();
    const float slideY = (1.f - reveal) * -static_cast<float>(config_.height);
    const GfxVec2i mouse = adjustedMousePos();
    const int hover = moduleAt(mouse, slideY);
    if (hover != hoveredIndex_) {
        hoveredIndex_ = hover;
        markDirty();
    }

    bool measureChanged = false;
    for (size_t i = 0; i < layout_.size(); ++i) {
        ModuleInput input{};
        input.mousePos = mouse;
        input.hovered = static_cast<int>(i) == hoveredIndex_;
        input.pressed = input.hovered && window_.mouseDown();
        input.wheelDelta = window_.mouseWheelDelta();
        layout_[i].module->update(input, layout_[i].bounds);
        if (layout_[i].module->layoutAffectsMeasure()) measureChanged = true;
    }

    if (measureChanged) layoutDirty_ = true;
}

bool Bar::render() {
    if (layoutDirty_) buildLayout();

    const float reveal = revealAnim_.value();
    const float slideY = (1.f - reveal) * -static_cast<float>(config_.height);

    GfxColor bg = toGfxColor(config_.background);
    if (!window_.beginDraw(bg)) return false;

    const GfxColor fg = toGfxColor(config_.foreground);

    for (size_t i = 0; i < layout_.size(); ++i) {
        const auto& placed = layout_[i];
        GfxRect bounds = placed.bounds;
        bounds.top += slideY;
        ModulePaintInfo info{window_, ctx_, bounds, fg, reveal, static_cast<int>(i) == hoveredIndex_};
        placed.module->paint(info);
    }

    window_.endDraw();
    return true;
}

bool Bar::pollEvents() {
    const bool activity = window_.pollEvents();
    const float reveal = revealAnim_.value();
    const float slideY = (1.f - reveal) * -static_cast<float>(config_.height);
    const GfxVec2i mouse = adjustedMousePos();

    if (popup_.isVisible() && window_.mouseClicked()) {
        const int idx = moduleAt(mouse, slideY);
        if (idx < 0 || !layout_[static_cast<size_t>(idx)].module->interactive() ||
            layout_[static_cast<size_t>(idx)].module->id() != L"system") {
            hideSystemMenu();
        }
    }

    if (window_.mouseClicked()) {
        const int idx = moduleAt(mouse, slideY);
        if (idx >= 0) {
            auto& placed = layout_[static_cast<size_t>(idx)];
            GfxRect bounds = placed.bounds;
            bounds.top += slideY;
            placed.module->onClick(mouse, bounds);
        }
        window_.clearMouseClicked();
        markDirty();
    }

    const short wheel = window_.mouseWheelDelta();
    if (wheel != 0) {
        const int idx = moduleAt(mouse, slideY);
        if (idx >= 0) {
            auto& placed = layout_[static_cast<size_t>(idx)];
            if (placed.module->interactive()) {
                GfxRect bounds = placed.bounds;
                bounds.top += slideY;
                placed.module->onScroll(mouse, static_cast<float>(wheel), bounds);
                markDirty();
            }
        }
        window_.clearMouseWheelDelta();
    }

    return activity;
}

bool Bar::processFrame() {
    if (!open_ || !window_.isOpen()) {
        open_ = false;
        return false;
    }

    checkConfigChanged();

    const bool popupActive = popup_.processFrame();
    const bool hadInput = pollEvents();

    if (dataTimer_.elapsedMilliseconds() >= config_.performance.dataTickMs) {
        tickModules();
        dataTimer_.restart();
    }

    const bool animating = anyAnimating();
    const float delta = static_cast<float>(animTimer_.elapsedSeconds());
    animTimer_.restart();
    ctx_.deltaTime = delta;

    if (!revealAnim_.settled()) {
        revealAnim_.update(delta, 320.f);
        markDirty();
    }

    if (animating || hadInput || popupActive) updateModules();

    if (!dirty_ && !animating && !hadInput && !popupActive) return false;

    const int animFps = std::max(60, std::max(1, config_.performance.animFps));
    const float minFrame = 1.f / static_cast<float>(animFps);
    if (animating && frameTimer_.elapsedSeconds() < minFrame) return true;

    frameTimer_.restart();
    render();
    dirty_ = animating || popupActive;
    return dirty_ || hadInput || popupActive;
}

HWND Bar::popupHwnd() const { return popup_.hwnd(); }

void Bar::handleMouseWheel(short delta, const POINT& screenPos) {
    POINT client = screenPos;
    ScreenToClient(window_.hwnd(), &client);
    window_.injectMouseWheel(delta, {client.x, client.y});
    markDirty();
}

}  // namespace wingnome
