#include "bar_config.h"

namespace wingnome {

namespace {

bool objBool(const JsonObject& obj, const std::wstring& key, bool fallback) {
    auto it = obj.find(key);
    if (it == obj.end()) return fallback;
    if (it->second.type() == JsonValue::Type::Number) return it->second.asNumber() != 0;
    return fallback;
}

float objFloat(const JsonObject& obj, const std::wstring& key, float fallback) {
    auto it = obj.find(key);
    if (it == obj.end() || it->second.type() != JsonValue::Type::Number) return fallback;
    return static_cast<float>(it->second.asNumber());
}

int objInt(const JsonObject& obj, const std::wstring& key, int fallback) {
    auto it = obj.find(key);
    if (it == obj.end() || it->second.type() != JsonValue::Type::Number) return fallback;
    return static_cast<int>(it->second.asNumber());
}

}  // namespace

std::optional<COLORREF> BarConfig::parseColor(const std::wstring& s) {
    if (s.size() != 7 || s[0] != L'#') return std::nullopt;
    auto hex = [](wchar_t c) -> int {
        if (c >= L'0' && c <= L'9') return c - L'0';
        if (c >= L'a' && c <= L'f') return c - L'a' + 10;
        if (c >= L'A' && c <= L'F') return c - L'A' + 10;
        return -1;
    };
    const int r = hex(s[1]) * 16 + hex(s[2]);
    const int g = hex(s[3]) * 16 + hex(s[4]);
    const int b = hex(s[5]) * 16 + hex(s[6]);
    if (r < 0 || g < 0 || b < 0) return std::nullopt;
    return RGB(r, g, b);
}

std::vector<std::wstring> BarConfig::moduleNames(const JsonArray& arr) {
    std::vector<std::wstring> out;
    for (const auto& v : arr) {
        if (v.type() == JsonValue::Type::String) out.push_back(v.asString());
    }
    return out;
}

VolumeAnimConfig BarConfig::loadVolumeAnim(const JsonObject& obj) {
    VolumeAnimConfig cfg;
    cfg.hoverFadeMs = objFloat(obj, L"hover_fade_ms", cfg.hoverFadeMs);
    cfg.sliderExpandMs = objFloat(obj, L"slider_expand_ms", cfg.sliderExpandMs);
    cfg.pulseOnMute = objBool(obj, L"pulse_on_mute", cfg.pulseOnMute);
    cfg.pulseSpeedHz = objFloat(obj, L"pulse_speed_hz", cfg.pulseSpeedHz);
    return cfg;
}

VolumeConfig BarConfig::loadVolumeConfig(const JsonObject& root) {
    VolumeConfig cfg;
    auto it = root.find(L"volume");
    if (it == root.end() || it->second.type() != JsonValue::Type::Object) return cfg;

    const JsonObject& vol = it->second.asObject();
    cfg.step = objFloat(vol, L"step", cfg.step);
    cfg.sliderWidth = objInt(vol, L"slider_width", cfg.sliderWidth);
    cfg.sliderHeight = objInt(vol, L"slider_height", cfg.sliderHeight);
    cfg.scrollAdjust = objBool(vol, L"scroll_adjust", cfg.scrollAdjust);
    cfg.dragAdjust = objBool(vol, L"drag_adjust", cfg.dragAdjust);
    cfg.clickTogglesMute = objBool(vol, L"click_toggles_mute", cfg.clickTogglesMute);

    auto animIt = vol.find(L"animations");
    if (animIt != vol.end() && animIt->second.type() == JsonValue::Type::Object)
        cfg.animations = loadVolumeAnim(animIt->second.asObject());
    return cfg;
}

BarConfig BarConfig::load(const std::filesystem::path& path) {
    BarConfig cfg;
    const auto parsed = JsonParser::parseFile(path);
    if (!parsed || parsed->type() != JsonValue::Type::Object) return cfg;

    if (auto h = parsed->getNumber(L"height")) cfg.height = static_cast<int>(*h);
    if (auto bg = parsed->getString(L"background")) {
        if (auto c = parseColor(*bg)) cfg.background = *c;
    }
    if (auto fg = parsed->getString(L"foreground")) {
        if (auto c = parseColor(*fg)) cfg.foreground = *c;
    }
    if (auto f = parsed->getString(L"font")) cfg.font = *f;
    if (auto fs = parsed->getNumber(L"font_size")) cfg.fontSize = static_cast<int>(*fs);
    if (auto cf = parsed->getString(L"clock_format")) cfg.clockFormat = *cf;
    if (auto ac = parsed->getString(L"activities_command")) cfg.activitiesCommand = *ac;

    if (auto a = parsed->getArray(L"modules-left")) cfg.modulesLeft = moduleNames(*a);
    if (auto a = parsed->getArray(L"modules-center")) cfg.modulesCenter = moduleNames(*a);
    if (auto a = parsed->getArray(L"modules-right")) cfg.modulesRight = moduleNames(*a);

    if (auto anim = parsed->getObject(L"animations")) {
        cfg.animations.frameMs = objFloat(*anim, L"frame_ms", cfg.animations.frameMs);
        if (auto modules = anim->find(L"modules"); modules != anim->end() &&
            modules->second.type() == JsonValue::Type::Object) {
            cfg.animations.volume = loadVolumeConfig(modules->second.asObject());
        }
    }

    if (auto perf = parsed->getObject(L"performance")) {
        cfg.performance.vsync = objBool(*perf, L"vsync", cfg.performance.vsync);
        cfg.performance.dataTickMs = objInt(*perf, L"data_tick_ms", cfg.performance.dataTickMs);
        cfg.performance.idleSleepMs = objInt(*perf, L"idle_sleep_ms", cfg.performance.idleSleepMs);
        cfg.performance.animFps = objInt(*perf, L"anim_fps", cfg.performance.animFps);
    }

    if (cfg.modulesLeft.empty()) cfg.modulesLeft = {};
    if (cfg.modulesCenter.empty()) cfg.modulesCenter = {L"clock"};
    if (cfg.modulesRight.empty()) cfg.modulesRight = {L"system"};
    return cfg;
}

ShellConfig ShellConfig::load(const std::filesystem::path& path) {
    ShellConfig cfg;
    const auto parsed = JsonParser::parseFile(path);
    if (!parsed || parsed->type() != JsonValue::Type::Object) return cfg;

    if (auto wp = parsed->getString(L"wallpaper")) cfg.wallpaper = *wp;
    if (auto enabled = parsed->getNumber(L"desktop_enabled")) cfg.desktopEnabled = *enabled != 0;
    return cfg;
}

}  // namespace wingnome
