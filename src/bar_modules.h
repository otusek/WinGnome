#pragma once

#include "platform.h"
#include "bar_config.h"

#include <SFML/Graphics.hpp>

#include <functional>
#include <memory>
#include <string>

namespace wingnome {

class BarPopup;

struct ModuleContext {
    const BarConfig* config{nullptr};
    sf::Font* font{nullptr};
    unsigned fontSize{11};
    int barHeight{32};
    float deltaTime{0.f};
    BarPopup* popup{nullptr};
};

struct ModulePaintInfo {
    sf::RenderTarget& target;
    const ModuleContext& ctx;
    sf::FloatRect bounds;
    sf::Color fg;
};

struct ModuleInput {
    sf::Vector2i mousePos;
    bool hovered{false};
    bool pressed{false};
    bool dragging{false};
};

class IModule {
public:
    virtual ~IModule() = default;
    virtual std::wstring id() const = 0;
    virtual std::wstring label() const { return id(); }
    virtual bool interactive() const { return false; }
    virtual void tick() {}
    virtual void update(const ModuleInput& input, const sf::FloatRect& bounds) { (void)input; (void)bounds; }
    virtual sf::Vector2f measure() const = 0;
    virtual void paint(const ModulePaintInfo& info) const = 0;
    virtual void onClick(const sf::Vector2i&, const sf::FloatRect&) {}
    virtual void onScroll(const sf::Vector2i&, float delta, const sf::FloatRect&) { (void)delta; }
    virtual bool isAnimating() const { return false; }
    virtual bool layoutAffectsMeasure() const { return false; }
};

std::unique_ptr<IModule> createModule(const std::wstring& name, ModuleContext* ctx);
sf::String toSfString(const std::wstring& text);
sf::Color toSfColor(COLORREF color, sf::Uint8 alpha = 255);

}  // namespace wingnome
