#include "dock_config.h"

#include <algorithm>

namespace wingnome {

namespace {

bool objBool(const JsonObject& obj, const std::wstring& key, bool fallback) {
    auto it = obj.find(key);
    if (it == obj.end()) return fallback;
    if (it->second.type() == JsonValue::Type::Number) return it->second.asNumber() != 0;
    return fallback;
}

int objInt(const JsonObject& obj, const std::wstring& key, int fallback) {
    auto it = obj.find(key);
    if (it == obj.end() || it->second.type() != JsonValue::Type::Number) return fallback;
    return static_cast<int>(it->second.asNumber());
}

float objFloat(const JsonObject& obj, const std::wstring& key, float fallback) {
    auto it = obj.find(key);
    if (it == obj.end() || it->second.type() != JsonValue::Type::Number) return fallback;
    return static_cast<float>(it->second.asNumber());
}

}  // namespace

std::optional<COLORREF> DockConfig::parseColor(const std::wstring& s) {
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

std::vector<std::wstring> DockConfig::stringList(const JsonArray& arr) {
    std::vector<std::wstring> out;
    for (const auto& v : arr) {
        if (v.type() == JsonValue::Type::String) out.push_back(v.asString());
    }
    return out;
}

DockConfig DockConfig::load(const std::filesystem::path& path) {
    DockConfig cfg;
    const auto parsed = JsonParser::parseFile(path);
    if (!parsed || parsed->type() != JsonValue::Type::Object) return cfg;

    if (auto v = parsed->getNumber(L"enabled")) cfg.enabled = *v != 0;
    if (auto v = parsed->getNumber(L"height")) cfg.height = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"icon_size")) cfg.iconSize = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"icon_spacing")) cfg.iconSpacing = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"padding_h")) cfg.paddingH = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"corner_radius")) cfg.cornerRadius = static_cast<int>(*v);
    if (auto bg = parsed->getString(L"background")) {
        if (auto c = parseColor(*bg)) cfg.background = *c;
    }
    if (auto v = parsed->getNumber(L"background_opacity")) cfg.backgroundOpacity = static_cast<float>(*v);
    if (auto v = parsed->getNumber(L"background_blur")) cfg.backgroundBlur = *v != 0;
    if (auto v = parsed->getNumber(L"transparent_window")) cfg.transparentWindow = *v != 0;
    if (auto v = parsed->getNumber(L"use_adwaita_icons")) cfg.useAdwaitaIcons = *v != 0;
    if (auto v = parsed->getNumber(L"autohide")) cfg.autohide = *v != 0;
    if (auto v = parsed->getNumber(L"autohide_delay_ms")) cfg.autohideDelayMs = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"show_activities")) cfg.showActivities = *v != 0;
    if (auto cmd = parsed->getString(L"activities_command")) cfg.activitiesCommand = *cmd;
    if (auto arr = parsed->getArray(L"pinned")) cfg.pinned = stringList(*arr);

    if (auto perf = parsed->getObject(L"performance")) {
        cfg.performance.dataTickMs = objInt(*perf, L"data_tick_ms", cfg.performance.dataTickMs);
        cfg.performance.idleSleepMs = objInt(*perf, L"idle_sleep_ms", cfg.performance.idleSleepMs);
        cfg.performance.animFps = objInt(*perf, L"anim_fps", cfg.performance.animFps);
    }

    cfg.backgroundOpacity = std::clamp(cfg.backgroundOpacity, 0.f, 1.f);
    return cfg;
}

}  // namespace wingnome
