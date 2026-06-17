#include "platform.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <wininet.h>

#include "bar_modules.h"
#include "bar_anim.h"
#include "bar_icons.h"
#include "bar_popup.h"
#include "audio_endpoint.h"
#include "icon_sprite.h"
#include "TextureManager.hpp"

#include <algorithm>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ws2_32.lib")

namespace wingnome {

sf::String toSfString(const std::wstring& text) {
    if (text.empty()) return {};
    const auto* begin = reinterpret_cast<const sf::Uint16*>(text.data());
    return sf::String::fromUtf16(begin, begin + text.size());
}

sf::Color toSfColor(COLORREF color, sf::Uint8 alpha) {
    return sf::Color(GetRValue(color), GetGValue(color), GetBValue(color), alpha);
}

namespace {

sf::Vector2f measureText(const sf::Font& font, unsigned size, const std::wstring& text, float padH = 12.f) {
    const sf::String s = toSfString(text);
    sf::Text probe(s, font, size);
    const auto b = probe.getLocalBounds();
    return {b.width + padH * 2.f, static_cast<float>(size)};
}

void drawText(sf::RenderTarget& target, const sf::Font& font, unsigned size, const sf::Color& fg,
              const sf::FloatRect& bounds, const std::wstring& text, float padH = 12.f) {
    if (text.empty()) return;
    sf::Text label(toSfString(text), font, size);
    label.setFillColor(fg);
    label.setPosition(bounds.left + padH, bounds.top + (bounds.height - size) / 2.f - 2.f);
    target.draw(label);
}

constexpr float kIconPad = 8.f;
constexpr float kIconTextGap = 6.f;

void drawIcon(sf::RenderTarget& target, sf::Sprite& sprite, float left, float cy, sf::Color fg) {
    drawIconCentered(target, sprite, left + kBarIconSize * 0.5f, cy, fg);
}

float measureIconText(const sf::Font& font, unsigned size, const std::wstring& text) {
    const float textW = text.empty() ? 0.f : measureText(font, size, text, 0.f).x;
    return kIconPad + kBarIconSize + (text.empty() ? 0.f : kIconTextGap + textW) + kIconPad;
}

void drawIconText(sf::RenderTarget& target, const sf::Font& font, unsigned size, sf::Color fg,
                  const sf::FloatRect& bounds, sf::Sprite& icon, bool hasIcon, IconKind fallback,
                  const std::wstring& text) {
    const float cy = bounds.top + bounds.height * 0.5f;
    float textX = bounds.left + kIconPad;

    if (hasIcon) {
        drawIcon(target, icon, bounds.left + kIconPad, cy, fg);
        textX += kBarIconSize + kIconTextGap;
    } else {
        drawBarGlyph(target, {bounds.left + kIconPad + kBarIconSize * 0.5f, cy}, kBarIconSize, fg, fallback);
        textX += kBarIconSize + kIconTextGap;
    }

    if (!text.empty()) {
        sf::Text label(toSfString(text), font, size);
        label.setFillColor(fg);
        label.setPosition(textX, bounds.top + (bounds.height - size) / 2.f - 2.f);
        target.draw(label);
    }
}

class SpacerModule : public IModule {
public:
    explicit SpacerModule(std::wstring id) : id_(std::move(id)) {}
    std::wstring id() const override { return id_; }
    sf::Vector2f measure() const override { return {1.f, 1.f}; }
    void paint(const ModulePaintInfo&) const override {}
private:
    std::wstring id_;
};

class TextModule : public IModule {
public:
    TextModule(std::wstring id, std::wstring text) : id_(std::move(id)), text_(std::move(text)) {}
    std::wstring id() const override { return id_; }
    std::wstring label() const override { return text_; }
    sf::Vector2f measure() const override {
        return measureText(*ctx_->font, ctx_->fontSize, text_);
    }
    void paint(const ModulePaintInfo& info) const override {
        drawText(info.target, *info.ctx.font, info.ctx.fontSize, info.fg, info.bounds, text_);
    }
    void setContext(ModuleContext* ctx) { ctx_ = ctx; }
private:
    std::wstring id_, text_;
    ModuleContext* ctx_{nullptr};
};

class ActivitiesModule : public IModule {
public:
    explicit ActivitiesModule(ModuleContext* ctx) : ctx_(ctx) {}
    std::wstring id() const override { return L"activities"; }
    bool interactive() const override { return true; }
    void onClick(const sf::Vector2i&, const sf::FloatRect&) override {
        if (!ctx_->config->activitiesCommand.empty()) {
            std::wstring cmd = ctx_->config->activitiesCommand;
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
    sf::Vector2f measure() const override {
        return measureText(*ctx_->font, ctx_->fontSize, L"Activities", 16.f);
    }
    void paint(const ModulePaintInfo& info) const override {
        drawText(info.target, *info.ctx.font, info.ctx.fontSize, info.fg, info.bounds, L"Activities", 16.f);
    }
private:
    ModuleContext* ctx_;
};

class ClockModule : public IModule {
public:
    explicit ClockModule(ModuleContext* ctx) : ctx_(ctx) { update(); }
    std::wstring id() const override { return L"clock"; }
    void tick() override { update(); }
    sf::Vector2f measure() const override { return measureText(*ctx_->font, ctx_->fontSize, text_); }
    void paint(const ModulePaintInfo& info) const override {
        drawText(info.target, *info.ctx.font, info.ctx.fontSize, info.fg, info.bounds, text_);
    }
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

class BatteryModule : public IModule {
public:
    explicit BatteryModule(ModuleContext* ctx) : ctx_(ctx) {
        update();
        batteryTexture_ = TextureManager::instance().get("icons/battery.svg");
        chargingTexture_ = TextureManager::instance().get("icons/battery-charging.svg");
        if (batteryTexture_) {
            setupIconSprite(iconSprite_, *batteryTexture_, kBarIconSize);
            hasIcon_ = true;
        }
    }

    std::wstring id() const override { return L"battery"; }
    void tick() override { update(); }
    sf::Vector2f measure() const override {
        return {measureIconText(*ctx_->font, ctx_->fontSize, caption_), static_cast<float>(ctx_->barHeight)};
    }
    void paint(const ModulePaintInfo& info) const override {
        if (charging_ && chargingTexture_) iconSprite_.setTexture(*chargingTexture_, true);
        else if (batteryTexture_) iconSprite_.setTexture(*batteryTexture_, true);

        drawIconText(info.target, *info.ctx.font, info.ctx.fontSize, info.fg, info.bounds,
                     iconSprite_, hasIcon_, IconKind::Power, caption_);
    }

private:
    ModuleContext* ctx_;
    mutable sf::Sprite iconSprite_;
    const sf::Texture* batteryTexture_{nullptr};
    const sf::Texture* chargingTexture_{nullptr};
    bool hasIcon_{false};
    mutable std::wstring caption_{L"AC"};
    mutable bool charging_{false};

    void update() const {
        SYSTEM_POWER_STATUS ps{};
        if (!GetSystemPowerStatus(&ps)) {
            caption_ = L"--";
            charging_ = false;
            return;
        }
        charging_ = (ps.BatteryFlag & 8) != 0;
        if (ps.ACLineStatus == 1) {
            if (ps.BatteryFlag & 128) {
                caption_ = L"AC";
            } else if (charging_ && ps.BatteryLifePercent <= 100) {
                caption_ = std::to_wstring(ps.BatteryLifePercent) + L"%";
            } else {
                caption_ = L"AC";
            }
            return;
        }
        caption_ = (ps.BatteryLifePercent <= 100)
            ? std::to_wstring(ps.BatteryLifePercent) + L"%"
            : L"--";
    }
};

class NetworkModule : public IModule {
public:
    explicit NetworkModule(ModuleContext* ctx) : ctx_(ctx) {
        update();
        onlineTexture_ = TextureManager::instance().get("icons/network.svg");
        offlineTexture_ = TextureManager::instance().get("icons/network-offline.svg");
        if (onlineTexture_) {
            setupIconSprite(iconSprite_, *onlineTexture_, kBarIconSize);
            hasIcon_ = true;
        }
    }

    std::wstring id() const override { return L"network"; }
    void tick() override { update(); }
    sf::Vector2f measure() const override {
        return {measureIconText(*ctx_->font, ctx_->fontSize, caption_), static_cast<float>(ctx_->barHeight)};
    }
    void paint(const ModulePaintInfo& info) const override {
        if (!online_ && offlineTexture_) iconSprite_.setTexture(*offlineTexture_, true);
        else if (onlineTexture_) iconSprite_.setTexture(*onlineTexture_, true);

        drawIconText(info.target, *info.ctx.font, info.ctx.fontSize, info.fg, info.bounds,
                     iconSprite_, hasIcon_, IconKind::Settings, caption_);
    }

private:
    ModuleContext* ctx_;
    mutable sf::Sprite iconSprite_;
    const sf::Texture* onlineTexture_{nullptr};
    const sf::Texture* offlineTexture_{nullptr};
    bool hasIcon_{false};
    mutable std::wstring caption_{L"Offline"};
    mutable bool online_{false};

    void update() const {
        if (!InternetGetConnectedState(nullptr, 0)) {
            online_ = false;
            caption_ = L"Offline";
            return;
        }
        ULONG size = 0;
        GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                             nullptr, nullptr, &size);
        std::vector<BYTE> buf(size);
        auto* addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
        if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                 nullptr, addrs, &size) != NO_ERROR) {
            online_ = true;
            caption_ = L"Online";
            return;
        }
        online_ = false;
        for (auto* a = addrs; a; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp) continue;
            if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            online_ = true;
            caption_ = L"Online";
            return;
        }
        caption_ = L"Offline";
    }
};

class VolumeModule : public IModule {
public:
    explicit VolumeModule(ModuleContext* ctx) : ctx_(ctx) {
        refresh();
        volumeTexture_ = TextureManager::instance().get("icons/volume.svg");
        mutedTexture_ = TextureManager::instance().get("icons/volume-muted.svg");
        if (volumeTexture_) {
            setupIconSprite(iconSprite_, *volumeTexture_, kBarIconSize);
            hasIcon_ = true;
        }
    }

    std::wstring id() const override { return L"volume"; }
    bool interactive() const override { return true; }

    void tick() override {
        if (!dragging_) refresh();
    }

    bool isAnimating() const override {
        if (!sliderAnim_.settled()) return true;
        return (hovered_ || dragging_) && muted_ && cfg().animations.pulseOnMute;
    }

    bool layoutAffectsMeasure() const override {
        return hovered_ || dragging_ || !sliderAnim_.settled();
    }

    void update(const ModuleInput& input, const sf::FloatRect& bounds) override {
        const bool wasHovered = hovered_;
        hovered_ = input.hovered;
        if (input.pressed && input.hovered && cfg().dragAdjust) dragging_ = true;
        if (!input.dragging) dragging_ = false;
        if (dragging_ && cfg().dragAdjust) setVolumeFromX(static_cast<float>(input.mousePos.x), bounds);

        sliderAnim_.setTarget((hovered_ || dragging_) ? 1.f : 0.f);
        if (hovered_ || dragging_ || !sliderAnim_.settled() || wasHovered != hovered_) {
            sliderAnim_.update(ctx_->deltaTime, cfg().animations.sliderExpandMs);
        }
        if (muted_ && cfg().animations.pulseOnMute) {
            pulse_.setEnabled(true);
            pulse_.update(ctx_->deltaTime, cfg().animations.pulseSpeedHz);
        } else {
            pulse_.setEnabled(false);
        }
    }

    void onClick(const sf::Vector2i& pos, const sf::FloatRect& bounds) override {
        if (cfg().clickTogglesMute) {
            muted_ = !muted_;
            apply();
            return;
        }
        if (cfg().dragAdjust) setVolumeFromX(static_cast<float>(pos.x), bounds);
    }

    void onScroll(const sf::Vector2i&, float delta, const sf::FloatRect&) override {
        if (!cfg().scrollAdjust) return;
        const float dir = delta > 0.f ? 1.f : -1.f;
        level_ = std::clamp(level_ + dir * cfg().step, 0.f, 1.f);
        if (level_ > 0.f) muted_ = false;
        apply();
    }

    sf::Vector2f measure() const override {
        const float extra = sliderAnim_.value() * static_cast<float>(cfg().sliderWidth) + 8.f;
        return {measureIconText(*ctx_->font, ctx_->fontSize, labelText()) + extra,
                static_cast<float>(ctx_->barHeight)};
    }

    void paint(const ModulePaintInfo& info) const override {
        sf::Color fg = info.fg;
        if (muted_ && cfg().animations.pulseOnMute) {
            const float pulse = pulse_.value();
            fg = sf::Color(
                static_cast<sf::Uint8>(fg.r * (0.6f + 0.4f * pulse)),
                static_cast<sf::Uint8>(fg.g * (0.6f + 0.4f * pulse)),
                static_cast<sf::Uint8>(fg.b * (0.6f + 0.4f * pulse)),
                fg.a);
        }

        if (muted_ && mutedTexture_) iconSprite_.setTexture(*mutedTexture_, true);
        else if (volumeTexture_) iconSprite_.setTexture(*volumeTexture_, true);

        drawIconText(info.target, *info.ctx.font, info.ctx.fontSize, fg, info.bounds,
                     iconSprite_, hasIcon_, IconKind::Speaker, labelText());

        const float expand = sliderAnim_.value();
        if (expand <= 0.01f) return;

        const float labelW = measureIconText(*info.ctx.font, info.ctx.fontSize, labelText());
        const float trackW = static_cast<float>(cfg().sliderWidth) * expand;
        const float trackH = static_cast<float>(cfg().sliderHeight);
        const float x = info.bounds.left + labelW + 4.f;
        const float y = info.bounds.top + (info.bounds.height - trackH) * 0.5f;

        sf::RectangleShape track({trackW, trackH});
        track.setPosition(x, y);
        track.setFillColor(sf::Color(
            static_cast<sf::Uint8>(fg.r / 4),
            static_cast<sf::Uint8>(fg.g / 4),
            static_cast<sf::Uint8>(fg.b / 4),
            static_cast<sf::Uint8>(200.f * expand)));
        info.target.draw(track);

        if (level_ > 0.001f) {
            sf::RectangleShape fill({trackW * level_, trackH});
            fill.setPosition(x, y);
            fill.setFillColor(sf::Color(fg.r, fg.g, fg.b, static_cast<sf::Uint8>(255.f * expand)));
            info.target.draw(fill);
        }
    }

    bool isDragging() const { return dragging_; }

private:
    ModuleContext* ctx_;
    mutable float level_{0.f};
    mutable bool muted_{false};
    bool hovered_{false};
    bool dragging_{false};
    mutable AnimChannel sliderAnim_;
    mutable PulseAnim pulse_;
    mutable sf::Sprite iconSprite_;
    const sf::Texture* volumeTexture_{nullptr};
    const sf::Texture* mutedTexture_{nullptr};
    bool hasIcon_{false};

    const VolumeConfig& cfg() const { return ctx_->config->animations.volume; }

    std::wstring labelText() const {
        if (muted_) return L"Muted";
        return std::to_wstring(static_cast<int>(level_ * 100.f + 0.5f)) + L"%";
    }

    sf::FloatRect sliderRect(const sf::FloatRect& bounds) const {
        const float labelW = measureIconText(*ctx_->font, ctx_->fontSize, labelText());
        const float trackW = static_cast<float>(cfg().sliderWidth) * sliderAnim_.value();
        const float trackH = static_cast<float>(cfg().sliderHeight);
        const float x = bounds.left + labelW + 4.f;
        const float y = bounds.top + (bounds.height - trackH) * 0.5f;
        return {x, y, trackW, std::max(trackH, bounds.height)};
    }

    void setVolumeFromX(float x, const sf::FloatRect& bounds) {
        const sf::FloatRect slider = sliderRect(bounds);
        if (slider.width <= 1.f) return;
        const float t = std::clamp((x - slider.left) / slider.width, 0.f, 1.f);
        level_ = t;
        muted_ = level_ <= 0.001f;
        apply();
    }

    void refresh() {
        float level = 0.f;
        bool muted = false;
        if (AudioEndpoint::get().read(level, muted)) {
            level_ = level;
            muted_ = muted;
        }
    }

    void apply() {
        AudioEndpoint::get().write(level_, muted_);
    }
};

class TrayModule : public IModule {
public:
    explicit TrayModule(ModuleContext* ctx) : ctx_(ctx) {}
    std::wstring id() const override { return L"tray"; }
    sf::Vector2f measure() const override {
        return measureText(*ctx_->font, ctx_->fontSize, L"Tray", 10.f);
    }
    void paint(const ModulePaintInfo& info) const override {
        drawText(info.target, *info.ctx.font, info.ctx.fontSize, info.fg, info.bounds, L"Tray", 10.f);
    }
private:
    ModuleContext* ctx_;
};

class WorkspacesModule : public IModule {
public:
    explicit WorkspacesModule(ModuleContext* ctx) : ctx_(ctx) {}
    std::wstring id() const override { return L"workspaces"; }
    bool interactive() const override { return true; }
    void onClick(const sf::Vector2i&, const sf::FloatRect&) override {
        INPUT inputs[6]{};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_LWIN;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_CONTROL;
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = VK_RIGHT;
        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_RIGHT;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[4].type = INPUT_KEYBOARD;
        inputs[4].ki.wVk = VK_CONTROL;
        inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[5].type = INPUT_KEYBOARD;
        inputs[5].ki.wVk = VK_LWIN;
        inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(6, inputs, sizeof(INPUT));
    }
    sf::Vector2f measure() const override {
        return {static_cast<float>(dots_ * 22 + 8), static_cast<float>(ctx_->fontSize)};
    }
    void paint(const ModulePaintInfo& info) const override {
        const sf::Color active = info.fg;
        const sf::Color idle(info.fg.r / 3, info.fg.g / 3, info.fg.b / 3);
        float x = info.bounds.left + 8.f;
        const float cy = info.bounds.top + info.bounds.height * 0.5f;
        for (size_t i = 0; i < dots_; ++i) {
            sf::RectangleShape dot({10.f, 10.f});
            dot.setPosition(x, cy - 5.f);
            dot.setFillColor(i == active_ ? active : idle);
            info.target.draw(dot);
            x += 22.f;
        }
    }
private:
    ModuleContext* ctx_;
    size_t dots_{4};
    size_t active_{0};
};

class SystemClusterModule : public IModule {
public:
    explicit SystemClusterModule(ModuleContext* ctx) : ctx_(ctx) {
        refresh();
        volumeTexture_ = TextureManager::instance().get("icons/volume.svg");
        mutedTexture_ = TextureManager::instance().get("icons/volume-muted.svg");
        powerTexture_ = TextureManager::instance().get("icons/power.svg");

        const float iconSize = ctx_->barHeight * 0.46f;
        if (volumeTexture_) {
            setupIconSprite(volumeSprite_, *volumeTexture_, iconSize);
            hasVolumeIcon_ = true;
        }
        if (powerTexture_) {
            setupIconSprite(powerSprite_, *powerTexture_, iconSize);
            hasPowerIcon_ = true;
        }
    }

    std::wstring id() const override { return L"system"; }
    bool interactive() const override { return true; }

    void tick() override {
        if (!draggingVolume_) refresh();
    }

    bool isAnimating() const override {
        if (ctx_->popup && (ctx_->popup->isAnimating() || ctx_->popup->isVisible())) return true;
        return !sliderAnim_.settled();
    }

    bool layoutAffectsMeasure() const override { return hovered_ || !sliderAnim_.settled(); }

    void update(const ModuleInput& input, const sf::FloatRect& bounds) override {
        hovered_ = input.hovered;
        if (input.pressed && input.hovered) pressed_ = true;
        if (!input.dragging) {
            draggingVolume_ = false;
            pressed_ = false;
        }

        sliderAnim_.setTarget(hovered_ ? 1.f : 0.f);
        if (hovered_ || !sliderAnim_.settled())
            sliderAnim_.update(ctx_->deltaTime, 150.f);
    }

    void onClick(const sf::Vector2i&, const sf::FloatRect&) override {
        if (ctx_->popup) ctx_->popup->toggle();
    }

    void onScroll(const sf::Vector2i&, float delta, const sf::FloatRect&) override {
        const float dir = delta > 0.f ? 1.f : -1.f;
        const float step = ctx_->config->animations.volume.step;
        level_ = std::clamp(level_ + dir * step, 0.f, 1.f);
        if (level_ > 0.f) muted_ = false;
        apply();
    }

    sf::Vector2f measure() const override {
        return {72.f + sliderAnim_.value() * 8.f, static_cast<float>(ctx_->barHeight)};
    }

    void paint(const ModulePaintInfo& info) const override {
        const float h = info.bounds.height;
        const float cy = info.bounds.top + h * 0.5f;
        const float iconSize = h * 0.46f;
        const sf::Color fg = info.fg;

        const float xSpeaker = info.bounds.left + 22.f;
        const float xPower = info.bounds.left + info.bounds.width - 22.f;

        if (hasVolumeIcon_) {
            if (muted_ && mutedTexture_) volumeSprite_.setTexture(*mutedTexture_, true);
            else if (volumeTexture_) volumeSprite_.setTexture(*volumeTexture_, true);
            drawIconCentered(info.target, volumeSprite_, xSpeaker, cy, fg);
        } else {
            drawBarGlyph(info.target, {xSpeaker, cy}, iconSize, fg, IconKind::Speaker);
            if (muted_) {
                sf::Vertex line[] = {
                    sf::Vertex({xSpeaker - iconSize * 0.3f, cy - iconSize * 0.3f}, fg),
                    sf::Vertex({xSpeaker + iconSize * 0.3f, cy + iconSize * 0.3f}, fg),
                };
                info.target.draw(line, 2, sf::Lines);
            }
        }

        if (hasPowerIcon_) {
            drawIconCentered(info.target, powerSprite_, xPower, cy, fg);
        } else {
            drawBarGlyph(info.target, {xPower, cy}, iconSize, fg, IconKind::Power);
        }
    }

private:
    ModuleContext* ctx_;
    mutable float level_{0.f};
    mutable bool muted_{false};
    bool hovered_{false};
    bool pressed_{false};
    bool draggingVolume_{false};
    mutable AnimChannel sliderAnim_;
    mutable sf::Sprite volumeSprite_;
    mutable sf::Sprite powerSprite_;
    const sf::Texture* volumeTexture_{nullptr};
    const sf::Texture* mutedTexture_{nullptr};
    const sf::Texture* powerTexture_{nullptr};
    bool hasVolumeIcon_{false};
    bool hasPowerIcon_{false};

    void refresh() {
        float level = 0.f;
        bool muted = false;
        if (AudioEndpoint::get().read(level, muted)) {
            level_ = level;
            muted_ = muted;
        }
    }

    void apply() {
        AudioEndpoint::get().write(level_, muted_);
    }
};

}  // namespace

std::unique_ptr<IModule> createModule(const std::wstring& name, ModuleContext* ctx) {
    if (name == L"system") return std::make_unique<SystemClusterModule>(ctx);
    if (name == L"activities") return std::make_unique<ActivitiesModule>(ctx);
    if (name == L"clock") return std::make_unique<ClockModule>(ctx);
    if (name == L"battery") return std::make_unique<BatteryModule>(ctx);
    if (name == L"network") return std::make_unique<NetworkModule>(ctx);
    if (name == L"volume") return std::make_unique<VolumeModule>(ctx);
    if (name == L"tray") return std::make_unique<TrayModule>(ctx);
    if (name == L"workspaces") return std::make_unique<WorkspacesModule>(ctx);
    if (name == L"spacer") return std::make_unique<SpacerModule>(name);
    if (name.rfind(L"text/", 0) == 0) {
        auto m = std::make_unique<TextModule>(name, name.substr(5));
        m->setContext(ctx);
        return m;
    }
    auto m = std::make_unique<TextModule>(name, name);
    m->setContext(ctx);
    return m;
}

}  // namespace wingnome
