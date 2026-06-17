#pragma once

#include "platform.h"
#include "bar_config.h"
#include "bar_modules.h"

#include <SFML/Graphics.hpp>

#include "bar_popup.h"

#include <filesystem>
#include <memory>
#include <vector>

struct HWND__;
using HWND = HWND__*;

namespace wingnome {

constexpr unsigned WM_BAR_RELOAD = 0x8001;

class Bar {
public:
    bool create();
    void destroy();
    bool isOpen() const { return open_; }
    bool processFrame();
    HWND hwnd() const;
    int height() const { return config_.height; }
    int idleSleepMs() const { return config_.performance.idleSleepMs; }

private:
    struct PlacedModule {
        IModule* module{nullptr};
        sf::FloatRect bounds;
    };

    bool open_{false};
    bool dirty_{true};
    bool layoutDirty_{true};
    sf::RenderWindow window_;
    sf::Font font_;
    sf::Clock frameClock_;
    sf::Clock animClock_;
    sf::Clock dataClock_;
    BarConfig config_;
    std::filesystem::path configPath_;
    ModuleContext ctx_;
    std::vector<std::unique_ptr<IModule>> left_;
    std::vector<std::unique_ptr<IModule>> center_;
    std::vector<std::unique_ptr<IModule>> right_;
    std::vector<PlacedModule> layout_;
    BarPopup popup_;
    IModule* captureModule_{nullptr};
    bool mouseDown_{false};
    sf::Vector2i mousePos_{};

    bool loadFont();
    void reloadConfig();
    void rebuildModules();
    void tickModules();
    void buildLayout();
    bool pollEvents();
    void updateModules();
    bool anyAnimating() const;
    bool render();
    PlacedModule* moduleAt(const sf::Vector2i& pos);
    void setupPopupActions();
    void applyTopmost();
    void markDirty();
};

}  // namespace wingnome
