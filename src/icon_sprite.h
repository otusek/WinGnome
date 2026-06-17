#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>

namespace wingnome {

inline constexpr float kBarIconSize = 19.f;
inline constexpr float kPopupIconSize = 16.f;

inline void setupIconSprite(sf::Sprite& sprite, const sf::Texture& texture, float targetSize) {
    sprite.setTexture(texture, true);
    const sf::FloatRect bounds = sprite.getLocalBounds();
    const float scale = targetSize / std::max(bounds.width, bounds.height);
    sprite.setScale(scale, scale);
    sprite.setOrigin(bounds.width * 0.5f, bounds.height * 0.5f);
}

inline void drawIconCentered(sf::RenderTarget& target, sf::Sprite& sprite, float cx, float cy, sf::Color fg) {
    sprite.setColor(fg);
    sprite.setPosition(cx, cy);
    target.draw(sprite);
}

inline void drawCapsule(sf::RenderTarget& target, const sf::FloatRect& rect, sf::Color color) {
    const float h = rect.height;
    const float r = h * 0.5f;
    if (rect.width <= h) {
        sf::CircleShape dot(r);
        dot.setPosition(rect.left, rect.top);
        dot.setFillColor(color);
        target.draw(dot);
        return;
    }

    sf::CircleShape left(r);
    left.setPosition(rect.left, rect.top);
    left.setFillColor(color);
    target.draw(left);

    sf::RectangleShape mid({rect.width - h, h});
    mid.setPosition(rect.left + r, rect.top);
    mid.setFillColor(color);
    target.draw(mid);

    sf::CircleShape right(r);
    right.setPosition(rect.left + rect.width - h, rect.top);
    right.setFillColor(color);
    target.draw(right);
}

}  // namespace wingnome
