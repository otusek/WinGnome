#include "bar_config.h"

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

    if (auto a = parsed->getArray(L"modules-left")) cfg.modulesLeft = moduleNames(*a);
    if (auto a = parsed->getArray(L"modules-center")) cfg.modulesCenter = moduleNames(*a);
    if (auto a = parsed->getArray(L"modules-right")) cfg.modulesRight = moduleNames(*a);

    if (auto perf = parsed->getObject(L"performance")) {
        cfg.performance.dataTickMs = objInt(*perf, L"data_tick_ms", cfg.performance.dataTickMs);
        cfg.performance.idleSleepMs = objInt(*perf, L"idle_sleep_ms", cfg.performance.idleSleepMs);
        cfg.performance.animFps = objInt(*perf, L"anim_fps", cfg.performance.animFps);
    }

    if (cfg.modulesCenter.empty()) cfg.modulesCenter = {L"clock"};
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
