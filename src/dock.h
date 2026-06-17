#pragma once

#include "platform.h"
#include "dock_config.h"
#include "app_icons.h"
#include "window_tracker.h"
#include "bar_anim.h"
#include "bar_modules.h"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <vector>

struct HWND__;
using HWND = HWND__*;

namespace wingnome {

class Dock {
public:
    bool create();
    void destroy();
    bool isOpen() const { return open_; }
    bool processFrame();
    HWND hwnd() const;
    int idleSleepMs() const { return config_.performance.idleSleepMs; }
    void setExcludeHwnds(const std::vector<HWND>& hwnds);

private:
    enum class ItemKind { Activities, App };

    struct DockItem {
        ItemKind kind{ItemKind::App};
        std::wstring path;
        std::vector<HWND> windows;
        bool running{false};
        bool focused{false};
        float hover{0.f};
    };

    bool open_{false};
    bool dirty_{true};
    sf::RenderWindow window_;
    sf::Clock frameClock_;
    sf::Clock animClock_;
    sf::Clock dataClock_;
    DockConfig config_;
    std::filesystem::path configPath_;
    AppIconCache icons_;
    WindowTracker tracker_;
    std::vector<DockItem> items_;
    int hoveredIndex_{-1};
    AnimChannel visibility_;
    float deltaTime_{0.f};

    void reloadConfig();
    void rebuildItems();
    void applyPosition();
    void applyTopmost();
    void updateAutohide();
    void updateHover();
    bool pollEvents();
    bool render();
    int itemAt(const sf::Vector2i& pos) const;
    float contentWidth() const;
    sf::FloatRect dockScreenRect() const;
    void launchApp(const std::wstring& path);
    void focusApp(const DockItem& item);
    void openActivities();
    void drawRoundedBackground(sf::RenderTarget& target, const sf::FloatRect& rect) const;
    void drawActivitiesIcon(sf::RenderTarget& target, float cx, float cy, float scale,
                            const sf::Color& color) const;
    void drawIndicator(sf::RenderTarget& target, float cx, float bottom, bool running,
                       bool focused) const;
};

}  // namespace wingnome
