#pragma once

#include "platform.h"
#include "audio_volume.h"
#include "bar_anim.h"
#include "bar_config.h"
#include "bar_popup.h"
#include "gfx/d2d_window.h"
#include "gfx/frame_timer.h"
#include "gfx/gfx_font.h"
#include "gfx/types.h"
#include "paths.h"
#include "svg_icons.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace wingnome {

struct IBarHost {
    virtual ~IBarHost() = default;
    virtual void openSystemMenu(const GfxRect& anchor) = 0;
    virtual void openActivities() = 0;
    virtual bool systemMenuVisible() const = 0;
    virtual void hideSystemMenu() = 0;
};

struct ModuleContext {
    const BarConfig* config{nullptr};
    GfxFont* font{nullptr};
    D2dWindow* window{nullptr};
    SvgIconCache* icons{nullptr};
    AudioVolume* audio{nullptr};
    IBarHost* host{nullptr};
    unsigned fontSize{11};
    int barHeight{32};
    float deltaTime{0.f};
};

struct ModulePaintInfo {
    D2dWindow& window;
    const ModuleContext& ctx;
    GfxRect bounds;
    GfxColor fg;
    float reveal{1.f};
    bool hovered{false};
};

struct ModuleInput {
    GfxVec2i mousePos;
    bool hovered{false};
    bool pressed{false};
    bool dragging{false};
    short wheelDelta{0};
};

class IModule {
public:
    virtual ~IModule() = default;
    virtual std::wstring id() const = 0;
    virtual std::wstring label() const { return id(); }
    virtual bool interactive() const { return false; }
    virtual void tick() {}
    virtual void update(const ModuleInput& input, const GfxRect& bounds) {
        (void)input;
        (void)bounds;
    }
    virtual GfxSize measure() const = 0;
    virtual void paint(const ModulePaintInfo& info) const = 0;
    virtual void onClick(const GfxVec2i&, const GfxRect&) {}
    virtual void onScroll(const GfxVec2i&, float delta, const GfxRect&) { (void)delta; }
    virtual bool isAnimating() const { return false; }
    virtual bool layoutAffectsMeasure() const { return false; }
};

std::unique_ptr<IModule> createModule(const std::wstring& name, ModuleContext* ctx);
GfxColor toGfxColor(COLORREF color, uint8_t alpha = 255);

class Bar : public IBarHost {
public:
    bool create();
    void destroy();
    bool isOpen() const { return open_; }
    bool processFrame();
    HWND hwnd() const;
    int height() const { return config_.height; }
    int idleSleepMs() const { return config_.performance.idleSleepMs; }

    HWND popupHwnd() const;
    void handleMouseWheel(short delta, const POINT& screenPos);

    void openSystemMenu(const GfxRect& anchor) override;
    void openActivities() override;
    bool systemMenuVisible() const override;
    void hideSystemMenu() override;

private:
    struct PlacedModule {
        IModule* module{nullptr};
        GfxRect bounds;
    };

    bool open_{false};
    bool dirty_{true};
    bool layoutDirty_{true};
    int hoveredIndex_{-1};
    D2dWindow window_;
    GfxFont font_;
    SvgIconCache icons_;
    AudioVolume audio_;
    BarPopup popup_;
    FrameTimer frameTimer_;
    FrameTimer animTimer_;
    FrameTimer dataTimer_;
    BarConfig config_;
    std::filesystem::path configPath_;
    ConfigChangeDetect configWatch_{};
    ModuleContext ctx_;
    std::vector<std::unique_ptr<IModule>> left_;
    std::vector<std::unique_ptr<IModule>> center_;
    std::vector<std::unique_ptr<IModule>> right_;
    std::vector<PlacedModule> layout_;
    AnimChannel revealAnim_;

    bool loadFont();
    void reloadConfig();
    void checkConfigChanged();
    void rebuildModules();
    void tickModules();
    void buildLayout();
    bool pollEvents();
    void updateModules();
    bool anyAnimating() const;
    bool render();
    void applyTopmost();
    void markDirty();
    int screenWidth() const;
    int moduleAt(const GfxVec2i& pos, float slideY) const;
    GfxVec2i adjustedMousePos() const;
};

}  // namespace wingnome
