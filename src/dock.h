#pragma once

#include "platform.h"
#include "bar_anim.h"
#include "dock_config.h"
#include "dock_icons.h"
#include "gfx/d2d_window.h"
#include "gfx/frame_timer.h"
#include "paths.h"
#include "window_tracker.h"

#include <filesystem>
#include <vector>

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
    enum class ItemKind { Activities, App, Separator };

    struct DockItem {
        ItemKind kind{ItemKind::App};
        std::wstring path;
        std::wstring launchPath;
        std::vector<HWND> windows;
        bool running{false};
        bool focused{false};
        bool pinned{false};
        GfxRect bounds;
    };

    bool open_{false};
    bool dirty_{true};
    D2dWindow window_;
    FrameTimer frameTimer_;
    FrameTimer animTimer_;
    FrameTimer dataTimer_;
    FrameTimer autohideTimer_;
    DockConfig config_;
    std::filesystem::path configPath_;
    ConfigChangeDetect configWatch_{};
    DockIconCache icons_;
    WindowTracker tracker_;
    std::vector<DockItem> items_;
    int hoveredIndex_{-1};
    AnimChannel visibility_;
    float deltaTime_{0.f};
    bool pointerNearDock_{false};
    bool windowLayered_{true};
    bool windowBlur_{false};

    bool usesLayeredWindow() const;
    void applyVisualStyle();
    bool recreateWindow();
    bool openWindow();
    void closeWindow();
    void reloadConfig();
    void checkConfigChanged();
    void rebuildItems();
    void applyPosition();
    void applyTopmost();
    void updateAutohide();
    void updateHover();
    bool pollEvents();
    bool render();
    int itemAt(const GfxVec2i& pos) const;
    float contentWidth() const;
    GfxRect dockScreenRect() const;
    void launchApp(const std::wstring& path);
    void focusApp(const DockItem& item);
    void openActivities();
    void drawBackground(float alpha);
    void drawActivitiesIcon(float cx, float cy, float scale, GfxColor color);
    void drawIconShadow(float cx, float iconBottom, float iconSize, float alpha);
    void drawIndicator(float cx, float bottom, bool running, bool focused, float alpha);
    void drawSeparator(const GfxRect& bounds, float alpha);
    void layoutItemBounds();
    bool anyAnimating() const;
    void markDirty();
    int screenWidth() const;
    int screenHeight() const;
};

}  // namespace wingnome
