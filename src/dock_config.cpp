#include "dock_config.h"

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

DockConfig DockConfig::load(const std::filesystem::path& path) {
    DockConfig cfg;
    const auto parsed = JsonParser::parseFile(path);
    if (!parsed || parsed->type() != JsonValue::Type::Object) return cfg;

    if (auto v = parsed->getNumber(L"enabled")) cfg.enabled = *v != 0;
    if (auto v = parsed->getNumber(L"height")) cfg.height = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"icon_size")) cfg.iconSize = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"icon_spacing")) cfg.iconSpacing = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"padding_h")) cfg.paddingH = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"padding_v")) cfg.paddingV = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"margin_bottom")) cfg.marginBottom = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"corner_radius")) cfg.cornerRadius = static_cast<int>(*v);
    if (auto bg = parsed->getString(L"background")) {
        if (auto c = parseColor(*bg)) cfg.background = *c;
    }
    if (auto v = parsed->getNumber(L"background_alpha")) cfg.backgroundAlpha = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"autohide")) cfg.autohide = *v != 0;
    if (auto v = parsed->getNumber(L"autohide_trigger_px")) cfg.autohideTriggerPx = static_cast<int>(*v);
    if (auto v = parsed->getNumber(L"show_activities")) cfg.showActivities = *v != 0;
    if (auto v = parsed->getNumber(L"show_running_apps")) cfg.showRunningApps = *v != 0;
    if (auto ac = parsed->getString(L"activities_command")) cfg.activitiesCommand = *ac;

    if (auto arr = parsed->getArray(L"pinned")) {
        for (const auto& item : *arr) {
            if (item.type() == JsonValue::Type::String) cfg.pinned.push_back(item.asString());
        }
    }

    if (auto perf = parsed->getObject(L"performance")) {
        cfg.performance.vsync = objBool(*perf, L"vsync", cfg.performance.vsync);
        cfg.performance.dataTickMs = objInt(*perf, L"data_tick_ms", cfg.performance.dataTickMs);
        cfg.performance.idleSleepMs = objInt(*perf, L"idle_sleep_ms", cfg.performance.idleSleepMs);
        cfg.performance.animFps = objInt(*perf, L"anim_fps", cfg.performance.animFps);
    }

    return cfg;
}

}  // namespace wingnome
