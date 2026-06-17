#include "paths.h"

#include <windows.h>

#include <system_error>

namespace wingnome {

std::filesystem::path exeDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

std::filesystem::path findConfigFile(const wchar_t* filename) {
    const auto root = exeDir();
    const std::filesystem::path candidates[] = {
        root / L"config" / filename,
        root / filename,
        std::filesystem::current_path() / L"config" / filename,
        std::filesystem::current_path() / filename,
        root / L".." / L"config" / filename,
        root / L".." / L".." / L"config" / filename,
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }
    return root / L"config" / filename;
}

std::filesystem::path findAssetFile(const std::string& relativePath) {
    const auto root = exeDir();
    const std::filesystem::path rel = std::filesystem::path(relativePath);
    const std::filesystem::path candidates[] = {
        root / L"assets" / rel,
        root / rel,
        std::filesystem::current_path() / L"assets" / rel,
        std::filesystem::current_path() / rel,
        root / L".." / L"assets" / rel,
        root / L".." / L".." / L"assets" / rel,
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }
    return root / L"assets" / rel;
}

}  // namespace wingnome
