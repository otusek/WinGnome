#pragma once

#include "platform.h"
#include "audio_volume.h"
#include "bar_config.h"
#include "gfx/d2d_window.h"
#include "gfx/gfx_font.h"
#include "gfx/types.h"
#include "svg_icons.h"

#include <memory>
#include <string>

namespace wingnome {

class BarPopup;

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

}  // namespace wingnome
