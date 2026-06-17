#pragma once

#include "bar_anim.h"
#include "bar_config.h"

#include <SFML/Graphics.hpp>

#include <functional>

namespace wingnome {

class BarPopup {
public:
    using Action = std::function<void()>;

    bool create(int barHeight, int screenWidth, const BarConfig& config);
    void destroy();
    void toggle();
    void close();
    bool isVisible() const;
    bool isAnimating() const;
    bool processFrame(float deltaSeconds);
    bool pollEvents();
    void render();

    void setOnScreenshot(Action fn) { onScreenshot_ = std::move(fn); }
    void setOnSettings(Action fn) { onSettings_ = std::move(fn); }
    void setOnPower(Action fn) { onPower_ = std::move(fn); }

private:
    enum class Hit { None, Volume, Screenshot, Settings, Power };

    const BarConfig* config_{nullptr};
    sf::RenderWindow window_;
    AnimChannel openAnim_;
    bool open_{false};
    int barHeight_{32};
    int screenWidth_{0};
    sf::Vector2i mousePos_{};
    Hit hover_{Hit::None};
    bool draggingVolume_{false};
    float volumeLevel_{1.f};
    bool volumeMuted_{false};

    sf::Sprite volumeSprite_;
    sf::Sprite screenshotSprite_;
    sf::Sprite settingsSprite_;
    sf::Sprite powerSprite_;
    const sf::Texture* volumeTexture_{nullptr};
    const sf::Texture* mutedTexture_{nullptr};
    const sf::Texture* screenshotTexture_{nullptr};
    const sf::Texture* settingsTexture_{nullptr};
    const sf::Texture* powerTexture_{nullptr};

    Action onScreenshot_;
    Action onSettings_;
    Action onPower_;

    float panelWidth() const;
    float panelHeight() const;
    float panelX() const;
    sf::FloatRect buttonRect(int index) const;
    sf::FloatRect volumeSliderRect() const;
    sf::FloatRect volumeHitRect() const;
    Hit hitTest(const sf::Vector2i& pos) const;
    void applyPosition();
    void loadIcons();
    void refreshVolume();
    void setVolumeFromX(float x);
    void applyVolume();
    sf::Color panelColor(float alpha) const;
    sf::Color hoverColor(float alpha) const;
    void drawPanel();
};

}  // namespace wingnome
