#include "bar_modules.h"

#include "battery_status.h"
#include "network_status.h"
#include "shell_actions.h"

#include <ctime>
#include <memory>
#include <string>

namespace wingnome {

GfxColor toGfxColor(COLORREF color, uint8_t alpha) {
    return gfxFromColorref(color, alpha);
}

namespace {

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
        const float cx = info.bounds.left + info.bounds.width * 0.5f;
        const float cy = info.bounds.top + info.bounds.height * 0.5f;
        const float dot = 2.5f;
        const float gap = 4.f;
        GfxColor color = info.fg;
        color.a = static_cast<uint8_t>(color.a * info.reveal);
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                const float ox = (static_cast<float>(col) - 1.f) * (dot * 2.f + gap);
                const float oy = (static_cast<float>(row) - 1.f) * (dot * 2.f + gap);
                const GfxRect r{cx + ox - dot, cy + oy - dot, dot * 2.f, dot * 2.f};
                info.window.fillEllipse(r, color);
            }
        }
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

}  // namespace wingnome
