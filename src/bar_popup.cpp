#include "bar_popup.h"

#include "audio_endpoint.h"
#include "icon_sprite.h"
#include "TextureManager.hpp"
#include "platform.h"

#include <windows.h>

#include <algorithm>

namespace wingnome {

namespace {

constexpr float kPad = 12.f;
constexpr float kPanelH = 96.f;
constexpr float kBtnW = 52.f;
constexpr float kBtnH = 34.f;
constexpr float kBtnGap = 10.f;
constexpr float kVolRowY = 24.f;
constexpr float kBtnRowY = 66.f;
constexpr float kSliderW = 132.f;
constexpr float kSliderH = 5.f;

}  // namespace

bool BarPopup::create(int barHeight, int screenWidth, const BarConfig& config) {
    barHeight_ = barHeight;
    screenWidth_ = screenWidth;
    config_ = &config;
    openAnim_.snap(0.f);
    loadIcons();
    refreshVolume();

    const float w = panelWidth();
    window_.create(sf::VideoMode(static_cast<unsigned>(w), static_cast<unsigned>(kPanelH)),
                   "WinGnome Menu", sf::Style::None);
    if (!window_.isOpen()) return false;

    HWND native = reinterpret_cast<HWND>(window_.getSystemHandle());
    LONG_PTR ex = GetWindowLongPtr(native, GWL_EXSTYLE);
    SetWindowLongPtr(native, GWL_EXSTYLE, ex | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
    window_.setVerticalSyncEnabled(config.performance.vsync);
    applyPosition();
    window_.setVisible(false);
    return true;
}

void BarPopup::destroy() {
    if (window_.isOpen()) window_.close();
    open_ = false;
}

void BarPopup::loadIcons() {
    volumeTexture_ = TextureManager::instance().get("icons/volume.svg");
    mutedTexture_ = TextureManager::instance().get("icons/volume-muted.svg");
    screenshotTexture_ = TextureManager::instance().get("icons/screenshot.svg");
    settingsTexture_ = TextureManager::instance().get("icons/settings.svg");
    powerTexture_ = TextureManager::instance().get("icons/power.svg");

    if (volumeTexture_) setupIconSprite(volumeSprite_, *volumeTexture_, kPopupIconSize);
    if (screenshotTexture_) setupIconSprite(screenshotSprite_, *screenshotTexture_, kPopupIconSize);
    if (settingsTexture_) setupIconSprite(settingsSprite_, *settingsTexture_, kPopupIconSize);
    if (powerTexture_) setupIconSprite(powerSprite_, *powerTexture_, kPopupIconSize);
}

void BarPopup::refreshVolume() {
    float level = volumeLevel_;
    bool muted = volumeMuted_;
    if (AudioEndpoint::get().read(level, muted)) {
        volumeLevel_ = level;
        volumeMuted_ = muted;
    }
}

void BarPopup::applyVolume() {
    AudioEndpoint::get().write(volumeLevel_, volumeMuted_);
}

void BarPopup::setVolumeFromX(float x) {
    const sf::FloatRect slider = volumeSliderRect();
    if (slider.width <= 1.f) return;
    volumeLevel_ = std::clamp((x - slider.left) / slider.width, 0.f, 1.f);
    volumeMuted_ = volumeLevel_ <= 0.001f;
    applyVolume();
}

float BarPopup::panelWidth() const {
    const float buttonsW = kPad + kBtnW * 3.f + kBtnGap * 2.f + kPad;
    const float volumeW = kPad + kPopupIconSize + 8.f + kSliderW + kPad;
    return std::max(buttonsW, volumeW);
}

float BarPopup::panelHeight() const { return kPanelH; }

float BarPopup::panelX() const { return static_cast<float>(screenWidth_) - panelWidth() - 8.f; }

sf::Color BarPopup::panelColor(float alpha) const {
    return sf::Color(GetRValue(config_->background), GetGValue(config_->background),
                     GetBValue(config_->background), static_cast<sf::Uint8>(240.f * alpha));
}

sf::Color BarPopup::hoverColor(float alpha) const {
    const auto lift = [](sf::Uint8 c) {
        return static_cast<sf::Uint8>(std::min(255, static_cast<int>(c) + 24));
    };
    return sf::Color(lift(GetRValue(config_->background)), lift(GetGValue(config_->background)),
                    lift(GetBValue(config_->background)), static_cast<sf::Uint8>(255.f * alpha));
}

void BarPopup::applyPosition() {
    const int y = barHeight_;
    const int x = static_cast<int>(panelX());
    window_.setPosition(sf::Vector2i(x, y));
    HWND native = reinterpret_cast<HWND>(window_.getSystemHandle());
    SetWindowPos(native, HWND_TOPMOST, x, y, static_cast<int>(panelWidth()),
                 static_cast<int>(kPanelH), SWP_SHOWWINDOW);
}

void BarPopup::toggle() {
    open_ = !open_;
    openAnim_.setTarget(open_ ? 1.f : 0.f);
    if (open_) {
        refreshVolume();
        applyPosition();
        window_.setVisible(true);
    }
}

void BarPopup::close() {
    open_ = false;
    openAnim_.setTarget(0.f);
    draggingVolume_ = false;
}

bool BarPopup::isVisible() const { return open_ || openAnim_.value() > 0.01f; }

bool BarPopup::isAnimating() const { return !openAnim_.settled(); }

sf::FloatRect BarPopup::buttonRect(int index) const {
    const float rowW = kBtnW * 3.f + kBtnGap * 2.f;
    const float x0 = (panelWidth() - rowW) * 0.5f;
    const float x = x0 + index * (kBtnW + kBtnGap);
    return {x, kBtnRowY - kBtnH * 0.5f, kBtnW, kBtnH};
}

sf::FloatRect BarPopup::volumeSliderRect() const {
    const float x = kPad + kPopupIconSize + 8.f;
    return {x, kVolRowY - kSliderH * 0.5f, kSliderW, kSliderH};
}

sf::FloatRect BarPopup::volumeHitRect() const {
    const auto slider = volumeSliderRect();
    return {kPad, slider.top - 12.f, slider.left + slider.width - kPad, slider.height + 24.f};
}

BarPopup::Hit BarPopup::hitTest(const sf::Vector2i& pos) const {
    const sf::Vector2f p(static_cast<float>(pos.x), static_cast<float>(pos.y));
    if (volumeHitRect().contains(p)) return Hit::Volume;
    for (int i = 0; i < 3; ++i) {
        if (buttonRect(i).contains(p)) {
            if (i == 0) return Hit::Screenshot;
            if (i == 1) return Hit::Settings;
            return Hit::Power;
        }
    }
    return Hit::None;
}

bool BarPopup::pollEvents() {
    if (!window_.isOpen()) return false;
    bool activity = false;
    sf::Event event{};
    while (window_.pollEvent(event)) {
        activity = true;
        if (event.type == sf::Event::Closed) close();
        if (event.type == sf::Event::MouseMoved) {
            mousePos_ = {event.mouseMove.x, event.mouseMove.y};
            hover_ = hitTest(mousePos_);
            if (draggingVolume_) setVolumeFromX(static_cast<float>(mousePos_.x));
        }
        if (event.type == sf::Event::MouseWheelScrolled) {
            const float dir = event.mouseWheelScroll.delta > 0.f ? 1.f : -1.f;
            volumeLevel_ = std::clamp(volumeLevel_ + dir * config_->animations.volume.step, 0.f, 1.f);
            if (volumeLevel_ > 0.f) volumeMuted_ = false;
            applyVolume();
        }
        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Left) {
            draggingVolume_ = false;
        }
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
            mousePos_ = {event.mouseButton.x, event.mouseButton.y};
            switch (hitTest(mousePos_)) {
            case Hit::Volume:
                draggingVolume_ = true;
                setVolumeFromX(static_cast<float>(mousePos_.x));
                break;
            case Hit::Screenshot:
                if (onScreenshot_) onScreenshot_();
                close();
                break;
            case Hit::Settings:
                if (onSettings_) onSettings_();
                close();
                break;
            case Hit::Power:
                if (onPower_) onPower_();
                close();
                break;
            default:
                close();
                break;
            }
        }
    }
    return activity;
}

bool BarPopup::processFrame(float deltaSeconds) {
    if (!window_.isOpen()) return false;
    const bool wasAnimating = isAnimating();
    openAnim_.update(deltaSeconds, config_->animations.volume.animations.sliderExpandMs + 80.f);
    if (!isVisible()) {
        window_.setVisible(false);
        return wasAnimating;
    }
    if (open_ && !draggingVolume_) refreshVolume();
    window_.setVisible(true);
    return isAnimating() || open_;
}

void BarPopup::drawPanel() {
    const float t = openAnim_.value();
    const float alpha = t;
    const float slide = (1.f - t) * -kPanelH;

    sf::RectangleShape bg({panelWidth(), kPanelH});
    bg.setFillColor(panelColor(alpha));
    bg.setPosition(0.f, slide);
    window_.draw(bg);

    const sf::Color fg(255, 255, 255, static_cast<sf::Uint8>(255.f * alpha));

    // Volume row
    const float volIconX = kPad + kPopupIconSize * 0.5f;
    const float volIconY = kVolRowY + slide;
    if (volumeTexture_) {
        if (volumeMuted_ && mutedTexture_) volumeSprite_.setTexture(*mutedTexture_, true);
        else volumeSprite_.setTexture(*volumeTexture_, true);
        drawIconCentered(window_, volumeSprite_, volIconX, volIconY, fg);
    }

    const sf::FloatRect slider = volumeSliderRect();
    const sf::FloatRect track = {slider.left, slider.top + slide, slider.width, slider.height};
    sf::RectangleShape trackShape({track.width, track.height});
    trackShape.setPosition(track.left, track.top);
    trackShape.setFillColor(sf::Color(fg.r / 5, fg.g / 5, fg.b / 5, static_cast<sf::Uint8>(200.f * alpha)));
    window_.draw(trackShape);

    if (volumeLevel_ > 0.001f) {
        sf::RectangleShape fill({track.width * volumeLevel_, track.height});
        fill.setPosition(track.left, track.top);
        fill.setFillColor(sf::Color(fg.r, fg.g, fg.b, static_cast<sf::Uint8>(255.f * alpha)));
        window_.draw(fill);
    }

    // Action buttons with capsule hover
    struct Item {
        sf::Sprite* sprite;
        const sf::Texture* texture;
        Hit hit;
    };
    const Item items[] = {
        {&screenshotSprite_, screenshotTexture_, Hit::Screenshot},
        {&settingsSprite_, settingsTexture_, Hit::Settings},
        {&powerSprite_, powerTexture_, Hit::Power},
    };

    for (int i = 0; i < 3; ++i) {
        auto rect = buttonRect(i);
        rect.top += slide;
        const bool hot = hover_ == items[i].hit;
        if (hot) drawCapsule(window_, rect, hoverColor(alpha));

        if (items[i].texture) {
            const sf::Vector2f center(rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f);
            drawIconCentered(window_, *items[i].sprite, center.x, center.y, fg);
        }
    }
}

void BarPopup::render() {
    if (!window_.isOpen() || !isVisible()) return;
    window_.clear(sf::Color::Transparent);
    drawPanel();
    window_.display();
}

}  // namespace wingnome
