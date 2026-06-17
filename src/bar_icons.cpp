#include "bar_icons.h"

#include <algorithm>
#include <cmath>

namespace wingnome {

namespace {

sf::Color withAlpha(sf::Color c, float a) {
    c.a = static_cast<sf::Uint8>(255.f * a);
    return c;
}

void drawSpeaker(sf::RenderTarget& target, sf::Vector2f c, float s, sf::Color fg) {
    sf::ConvexShape body(3);
    body.setPoint(0, {c.x - s * 0.35f, c.y - s * 0.25f});
    body.setPoint(1, {c.x - s * 0.35f, c.y + s * 0.25f});
    body.setPoint(2, {c.x + s * 0.05f, c.y + s * 0.25f});
    body.setFillColor(fg);
    target.draw(body);

    sf::CircleShape wave1(s * 0.18f);
    wave1.setOrigin(s * 0.18f, s * 0.18f);
    wave1.setPosition(c.x + s * 0.2f, c.y);
    wave1.setFillColor(sf::Color::Transparent);
    wave1.setOutlineThickness(1.5f);
    wave1.setOutlineColor(fg);
    target.draw(wave1);

    sf::CircleShape wave2(s * 0.32f);
    wave2.setOrigin(s * 0.32f, s * 0.32f);
    wave2.setPosition(c.x + s * 0.2f, c.y);
    wave2.setFillColor(sf::Color::Transparent);
    wave2.setOutlineThickness(1.5f);
    wave2.setOutlineColor(withAlpha(fg, 0.7f));
    target.draw(wave2);
}

void drawPower(sf::RenderTarget& target, sf::Vector2f c, float s, sf::Color fg) {
    sf::CircleShape ring(s * 0.32f);
    ring.setOrigin(s * 0.32f, s * 0.32f);
    ring.setPosition(c);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.f);
    ring.setOutlineColor(fg);
    target.draw(ring);

    sf::RectangleShape stem({2.f, s * 0.3f});
    stem.setFillColor(fg);
    stem.setOrigin(1.f, 0.f);
    stem.setPosition(c.x, c.y - s * 0.34f);
    target.draw(stem);
}

void drawScreenshot(sf::RenderTarget& target, sf::Vector2f c, float s, sf::Color fg) {
    sf::RectangleShape frame({s * 0.55f, s * 0.4f});
    frame.setOrigin(s * 0.275f, s * 0.2f);
    frame.setPosition(c);
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineThickness(2.f);
    frame.setOutlineColor(fg);
    target.draw(frame);

    sf::CircleShape lens(s * 0.07f);
    lens.setOrigin(s * 0.07f, s * 0.07f);
    lens.setPosition(c.x + s * 0.14f, c.y - s * 0.08f);
    lens.setFillColor(fg);
    target.draw(lens);
}

void drawSettings(sf::RenderTarget& target, sf::Vector2f c, float s, sf::Color fg) {
    sf::CircleShape core(s * 0.1f);
    core.setOrigin(s * 0.1f, s * 0.1f);
    core.setPosition(c);
    core.setFillColor(fg);
    target.draw(core);

    for (int i = 0; i < 6; ++i) {
        const float a = static_cast<float>(i) * 1.0472f;
        sf::RectangleShape tooth({s * 0.1f, s * 0.16f});
        tooth.setOrigin(s * 0.05f, s * 0.22f);
        tooth.setPosition(c);
        tooth.setRotation(a * 57.2958f);
        tooth.setFillColor(fg);
        target.draw(tooth);
    }

    sf::CircleShape hole(s * 0.22f);
    hole.setOrigin(s * 0.22f, s * 0.22f);
    hole.setPosition(c);
    hole.setFillColor(sf::Color::Transparent);
    hole.setOutlineThickness(2.f);
    hole.setOutlineColor(fg);
    target.draw(hole);
}

}  // namespace

void drawBarGlyph(sf::RenderTarget& target, const sf::Vector2f& center, float size,
                  const sf::Color& fg, IconKind icon) {
    switch (icon) {
    case IconKind::Speaker: drawSpeaker(target, center, size, fg); break;
    case IconKind::Power: drawPower(target, center, size, fg); break;
    case IconKind::Screenshot: drawScreenshot(target, center, size, fg); break;
    case IconKind::Settings: drawSettings(target, center, size, fg); break;
    }
}

void drawCircleButton(sf::RenderTarget& target, const sf::Vector2f& center, float radius,
                      const sf::Color& bg, const sf::Color& fg, IconKind icon,
                      float alpha, bool hovered) {
    sf::Color buttonBg = bg;
    if (hovered) {
        buttonBg.r = static_cast<sf::Uint8>(std::min(255, buttonBg.r + 30));
        buttonBg.g = static_cast<sf::Uint8>(std::min(255, buttonBg.g + 30));
        buttonBg.b = static_cast<sf::Uint8>(std::min(255, buttonBg.b + 30));
    }
    buttonBg.a = static_cast<sf::Uint8>(255.f * alpha);

    sf::CircleShape circle(radius);
    circle.setOrigin(radius, radius);
    circle.setPosition(center);
    circle.setFillColor(buttonBg);
    target.draw(circle);

    drawBarGlyph(target, center, radius * 1.6f, withAlpha(fg, alpha), icon);
}

}  // namespace wingnome
