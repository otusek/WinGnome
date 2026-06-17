#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace wingnome {

struct ConfigChangeDetect {
    std::filesystem::path path;
    uint64_t fingerprint{0};
    uint64_t pendingFingerprint{0};
    uint64_t pendingTick{0};
};

std::filesystem::path exeDir();
std::filesystem::path findConfigFile(const wchar_t* filename);
std::filesystem::path findAssetFile(const wchar_t* relativePath);
uint64_t configFingerprint(const std::filesystem::path& path);
bool configNeedsReload(ConfigChangeDetect& state, const wchar_t* filename, uint64_t debounceMs = 200);
void configMarkReloaded(ConfigChangeDetect& state, const std::filesystem::path& path);

}  // namespace wingnome
