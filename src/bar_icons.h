#pragma once

#include <SFML/Graphics.hpp>

namespace wingnome {

enum class IconKind { Speaker, Power, Screenshot, Settings };

void drawCircleButton(sf::RenderTarget& target, const sf::Vector2f& center, float radius,
                      const sf::Color& bg, const sf::Color& fg, IconKind icon,
                      float alpha = 1.f, bool hovered = false);

void drawBarGlyph(sf::RenderTarget& target, const sf::Vector2f& center, float size,
                  const sf::Color& fg, IconKind icon);

}  // namespace wingnome
